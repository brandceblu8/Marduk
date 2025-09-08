#include "pch.h"
#include "WifiPortal.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>

#pragma warning(push, 0)
#pragma warning(disable : 4251)
#pragma warning(disable: 4275)
#pragma warning(disable: 4819)
#pragma warning(disable: 4305)
#pragma warning(disable: 4309)
#pragma warning(pop)

class WifiPortal::WifiPortalImpl
{
private:
	cpr::Session session;
	std::string current_ip;
	std::string current_username;
	bool logged_in = false;
	bool debug_mode = false;
	int timeout_seconds = 15;

	static constexpr const char *BASE_URL = "https://w.xidian.edu.cn";
	static constexpr const char *DEFAULT_AC_ID = "8";
	static constexpr const char *DEFAULT_THEME = "pro";

public:
	WifiPortalImpl()
	{
		session.SetHeader(cpr::Header{
			{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36"},
			{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"},
			{"Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8,ja;q=0.7"},
			{"Cache-Control", "max-age=0"},
			{"Connection", "keep-alive"}});

		session.SetVerifySsl(cpr::VerifySsl(false));
		session.SetTimeout(cpr::Timeout{timeout_seconds * 1000});
		session.SetRedirect(cpr::Redirect{5L});

		session.SetCookies(cpr::Cookies{{"lang", "zh-CN"}});
	}

	~WifiPortalImpl() = default;

	std::map<std::string, std::string> extractConfigFromHtml(const std::string &html)
	{
		std::map<std::string, std::string> config;

		std::regex config_regex(R"(var CONFIG\s*=\s*\{([^}]+)\})");
		std::smatch config_match;

		if (std::regex_search(html, config_match, config_regex))
		{
			std::string config_content = config_match[1].str();
			std::vector<std::pair<std::string, std::regex>> patterns = {
				 {"ip", std::regex(R"(ip\s*:\s*"([^"]+))")},
				 {"acid", std::regex(R"(acid\s*:\s*"([^"]+))")},
				  {"nas", std::regex(R"(nas\s*:\s*"([^"]+))")},
				   {"mac", std::regex(R"(mac\s*:\s*"([^"]+))")}
				   };

			for (const auto &[key, pattern] : patterns)
			{
				std::smatch match;
				if (std::regex_search(config_content, match, pattern))
				{
					config[key] = match[1].str();
				}
			}
		}

		return config;
	}

	// ����MD5��ϣ
	std::string generateMD5(const std::string &input)
	{
		return "placeholder_md5_" + std::to_string(std::hash<std::string>{}(input));
	}

	std::string generateChecksum(const std::string &username, const std::string &password,
								 const std::string &ip, const std::string &acid)
	{
		// ʵ��У��������߼�������ץ���������
		// ����һ���򻯰汾��ʵ����Ҫ������վ���㷨ʵ��
		std::string combined = username + password + ip + acid;
		return "chksum_" + std::to_string(std::hash<std::string>{}(combined));
	}

	std::string generateInfo(const std::string &username, const std::string &password)
	{
		return "info_placeholder";
	}

	std::string extractJsonFromJsonp(const std::string &jsonp_response)
	{
		std::regex jsonp_regex(R"([^(]*\((.+)\))");
		std::smatch match;

		if (std::regex_search(jsonp_response, match, jsonp_regex))
		{
			return match[1].str();
		}

		return "";
	}

	std::string generateCallback()
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<> dis(100000000000000, 999999999999999);

		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
					   std::chrono::system_clock::now().time_since_epoch())
					   .count();

		return "jQuery" + std::to_string(dis(gen)) + "_" + std::to_string(now);
	}

	PortalResult checkConnectionImpl()
	{
		try
		{
			session.SetUrl(cpr::Url{std::string(BASE_URL) + "/srun_portal_pc?ac_id=" + DEFAULT_AC_ID + "&theme=" + DEFAULT_THEME});

			auto response = session.Get();

			if (response.status_code != 200)
			{
				return PortalResult(PortalErrorCode::NETWORK_HTTP_ERROR,
									"HTTP error: " + std::to_string(response.status_code));
			}

			auto config = extractConfigFromHtml(response.text);
			if (config.find("ip") != config.end())
			{
				current_ip = config["ip"];
			}

			if (response.text.find(u8"网络已连接") != std::string::npos)
			{
				logged_in = true;
				return PortalResult(PortalErrorCode::SUCCESS, "Already connected");
			}
			else
			{
				logged_in = false;
				return PortalResult(PortalErrorCode::SUCCESS, "Not connected");
			}
		}
		catch (const std::exception &e)
		{
			return PortalResult(PortalErrorCode::NETWORK_CONNECTION_FAILED,
								"Network error: " + std::string(e.what()));
		}
	}

	LoginResult loginImpl(const std::string &username, const std::string &password,
						  NetworkProvider provider)
	{
		try
		{
			auto check_result = checkConnectionImpl();
			if (!check_result.isSuccess() && check_result.error_code != PortalErrorCode::SUCCESS)
			{
				return LoginResult(check_result.error_code, check_result.error_message);
			}

			if (logged_in)
			{
				return LoginResult(PortalErrorCode::AUTH_ALREADY_ONLINE, "Already logged in");
			}

			std::string full_username = username;
			switch (provider)
			{
			case NetworkProvider::TELECOM:
				full_username += "@dx";
				break;
			case NetworkProvider::UNICOM:
				full_username += "@lt";
				break;
			case NetworkProvider::MOBILE:
				full_username += "@yd";
				break;
			case NetworkProvider::CAMPUS:
			default:
				break;
			}

			std::string callback = generateCallback();
			std::string md5_password = "{MD5}" + generateMD5(password);
			std::string chksum = generateChecksum(full_username, password, current_ip, DEFAULT_AC_ID);
			std::string info = generateInfo(full_username, password);

			auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
						   std::chrono::system_clock::now().time_since_epoch())
						   .count();

			std::ostringstream login_url;
			login_url << BASE_URL << "/cgi-bin/srun_portal"
					  << "?callback=" << callback
					  << "&action=login"
					  << "&username=" << cpr::util::urlEncode(full_username)
					  << "&password=" << cpr::util::urlEncode(md5_password)
					  << "&os=Windows+10"
					  << "&name=Windows"
					  << "&double_stack=0"
					  << "&chksum=" << chksum
					  << "&info=" << cpr::util::urlEncode(info)
					  << "&ac_id=" << DEFAULT_AC_ID
					  << "&ip=" << current_ip
					  << "&n=200"
					  << "&type=1"
					  << "&_=" << now;

			session.SetHeader(cpr::Header{
				{"Accept", "text/javascript, application/javascript, application/ecmascript, application/x-ecmascript, */*; q=0.01"},
				{"X-Requested-With", "XMLHttpRequest"},
				{"Referer", std::string(BASE_URL) + "/srun_portal_pc?ac_id=" + DEFAULT_AC_ID + "&theme=" + DEFAULT_THEME}});

			session.SetUrl(cpr::Url{login_url.str()});
			auto login_response = session.Get();

			if (login_response.status_code != 200)
			{
				return LoginResult(PortalErrorCode::NETWORK_HTTP_ERROR,
								   "Login request failed: " + std::to_string(login_response.status_code));
			}

			std::string json_str = extractJsonFromJsonp(login_response.text);
			if (json_str.empty())
			{
				return LoginResult(PortalErrorCode::PARSE_CALLBACK_FAILED, "Failed to parse JSONP response");
			}

			try
			{
				auto json_response = nlohmann::json::parse(json_str);

				LoginResult result;
				result.ecode = json_response.value("ecode", -1);
				result.res_status = json_response.value("res", "");
				result.error_message = json_response.value("error_msg", "");
				result.client_ip = json_response.value("client_ip", "");
				result.online_ip = json_response.value("online_ip", "");
				result.server_version = json_response.value("srun_ver", "");

				if (result.ecode == 0 && result.res_status == "ok")
				{
					logged_in = true;
					current_username = username;
					result.error_code = PortalErrorCode::SUCCESS;
					result.error_message = "Login successful";
				}
				else
				{
					if (result.error_message.find(u8"用户名") != std::string::npos ||
						result.error_message.find(u8"密码") != std::string::npos)
					{
						result.error_code = PortalErrorCode::AUTH_INVALID_CREDENTIALS;
					}
					else if (result.error_message.find(u8"已在线") != std::string::npos)
					{
						result.error_code = PortalErrorCode::AUTH_ALREADY_ONLINE;
					}
					else
					{
						result.error_code = PortalErrorCode::AUTH_USER_NOT_FOUND;
					}
				}

				return result;
			}
			catch (const nlohmann::json::exception &e)
			{
				return LoginResult(PortalErrorCode::PARSE_JSON_FAILED,
								   "JSON parse error: " + std::string(e.what()));
			}
		}
		catch (const std::exception &e)
		{
			return LoginResult(PortalErrorCode::NETWORK_CONNECTION_FAILED,
							   "Network error: " + std::string(e.what()));
		}
	}

	PortalResult logoutImpl()
	{
		if (!logged_in)
		{
			return PortalResult(PortalErrorCode::SUCCESS, "Not logged in");
		}

		try
		{
			std::string callback = generateCallback();
			auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
						   std::chrono::system_clock::now().time_since_epoch())
						   .count();

			std::ostringstream logout_url;
			logout_url << BASE_URL << "/cgi-bin/srun_portal"
					   << "?callback=" << callback
					   << "&action=logout"
					   << "&username=" << cpr::util::urlEncode(current_username)
					   << "&ip=" << current_ip
					   << "&ac_id=" << DEFAULT_AC_ID
					   << "&_=" << now;

			session.SetUrl(cpr::Url{logout_url.str()});
			auto response = session.Get();

			if (response.status_code != 200)
			{
				return PortalResult(PortalErrorCode::NETWORK_HTTP_ERROR,
									"Logout request failed: " + std::to_string(response.status_code));
			}

			std::string json_str = extractJsonFromJsonp(response.text);
			if (!json_str.empty())
			{
				try
				{
					auto json_response = nlohmann::json::parse(json_str);
					if (json_response.value("res", "") == "ok")
					{
						logged_in = false;
						current_username.clear();
						return PortalResult(PortalErrorCode::SUCCESS, "Logout successful");
					}
				}
				catch (const nlohmann::json::exception &)
				{
					
				}
			}

			logged_in = false;
			return PortalResult(PortalErrorCode::SUCCESS, "Logout completed");
		}
		catch (const std::exception &e)
		{
			return PortalResult(PortalErrorCode::NETWORK_CONNECTION_FAILED,
								"Logout error: " + std::string(e.what()));
		}
	}

	PortalResult getUserInfoImpl()
	{
		if (!logged_in)
		{
			return PortalResult(PortalErrorCode::AUTH_USER_NOT_FOUND, "Not logged in");
		}

		try
		{
			std::string callback = generateCallback();
			auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
						   std::chrono::system_clock::now().time_since_epoch())
						   .count();

			std::ostringstream info_url;
			info_url << BASE_URL << "/cgi-bin/rad_user_info"
					 << "?callback=" << callback
					 << "&ip=" << current_ip
					 << "&_=" << now;

			session.SetUrl(cpr::Url{info_url.str()});
			auto response = session.Get();

			if (response.status_code != 200)
			{
				return PortalResult(PortalErrorCode::NETWORK_HTTP_ERROR,
									"User info request failed: " + std::to_string(response.status_code));
			}

			std::string json_str = extractJsonFromJsonp(response.text);
			if (json_str.empty())
			{
				return PortalResult(PortalErrorCode::PARSE_CALLBACK_FAILED,
									"Failed to parse user info response");
			}

			try
			{
				auto json_data = nlohmann::json::parse(json_str);

				PortalResult result(PortalErrorCode::SUCCESS, "User info retrieved");
				PortalUserInfo &info = result.user_info;

				info.username = json_data.value("user_name", "");
				info.real_name = json_data.value("real_name", "");
				info.domain = json_data.value("domain", "");
				info.billing_name = json_data.value("billing_name", "");
				info.products_name = json_data.value("products_name", "");

				info.online_ip = json_data.value("online_ip", "");
				info.online_ipv6 = json_data.value("online_ip6", "");
				info.user_mac = json_data.value("user_mac", "");

				info.bytes_in = json_data.value("bytes_in", 0ULL);
				info.bytes_out = json_data.value("bytes_out", 0ULL);
				info.all_bytes = json_data.value("all_bytes", 0ULL);
				info.sum_bytes = json_data.value("sum_bytes", 0ULL);
				info.remain_bytes = json_data.value("remain_bytes", 0ULL);

				info.sum_seconds = json_data.value("sum_seconds", 0ULL);
				info.remain_seconds = json_data.value("remain_seconds", 0ULL);
				info.add_time = json_data.value("add_time", 0ULL);
				info.keepalive_time = json_data.value("keepalive_time", 0ULL);

				info.user_balance = json_data.value("user_balance", 0.0);
				info.user_charge = json_data.value("user_charge", 0.0);
				info.wallet_balance = json_data.value("wallet_balance", 0.0);

				info.online_device_total = std::stoi(json_data.value("online_device_total", "0"));

				if (json_data.contains("online_device_detail"))
				{
					std::string device_detail_str = json_data["online_device_detail"];
					try
					{
						auto device_detail = nlohmann::json::parse(device_detail_str);
						for (auto &[key, device] : device_detail.items())
						{
							OnlineDevice dev;
							dev.rad_online_id = device.value("rad_online_id", "");
							dev.ip_address = device.value("ip", "");
							dev.ipv6_address = device.value("ip6", "");
							dev.class_name = device.value("class_name", "");
							dev.os_name = device.value("os_name", "");
							info.online_devices.push_back(dev);
						}
					}
					catch (const nlohmann::json::exception &)
					{
						// �豸��Ϣ����ʧ�ܣ�����Ӱ��������Ϣ
					}
				}
				info.group_id = json_data.value("group_id", "");
				info.package_id = json_data.value("package_id", "");
				info.products_id = json_data.value("products_id", "");
				info.server_flag = json_data.value("ServerFlag", 0ULL);

				return result;
			}
			catch (const nlohmann::json::exception &e)
			{
				return PortalResult(PortalErrorCode::PARSE_JSON_FAILED,
									"JSON parse error: " + std::string(e.what()));
			}
		}
		catch (const std::exception &e)
		{
			return PortalResult(PortalErrorCode::NETWORK_CONNECTION_FAILED,
								"Network error: " + std::string(e.what()));
		}
	}

	// ����ʵ�ַ���...
	bool isLoggedInImpl() { return logged_in; }
	std::string getCurrentIPImpl() { return current_ip; }
	void setTimeoutImpl(int seconds)
	{
		timeout_seconds = seconds;
		session.SetTimeout(cpr::Timeout{seconds * 1000});
	}
	void setUserAgentImpl(const std::string &ua)
	{
		session.SetHeader(cpr::Header{{"User-Agent", ua}});
	}
	void setDebugModeImpl(bool enabled) { debug_mode = enabled; }
};


WifiPortal::WifiPortal() : impl(std::make_unique<WifiPortalImpl>()) {}
WifiPortal::~WifiPortal() = default;

PortalResult WifiPortal::checkConnection() { return impl->checkConnectionImpl(); }
LoginResult WifiPortal::login(const std::string &username, const std::string &password, NetworkProvider provider)
{
	return impl->loginImpl(username, password, provider);
}
PortalResult WifiPortal::logout() { return impl->logoutImpl(); }
PortalResult WifiPortal::getUserInfo() { return impl->getUserInfoImpl(); }
PortalResult WifiPortal::getDeviceList() { return getUserInfo(); }
PortalResult WifiPortal::getTrafficInfo() { return getUserInfo(); }

bool WifiPortal::isLoggedIn() { return impl->isLoggedInImpl(); }
std::string WifiPortal::getCurrentIP() { return impl->getCurrentIPImpl(); }

void WifiPortal::setTimeout(int seconds) { impl->setTimeoutImpl(seconds); }
void WifiPortal::setUserAgent(const std::string &ua) { impl->setUserAgentImpl(ua); }
void WifiPortal::setDebugMode(bool enabled) { impl->setDebugModeImpl(enabled); }

PortalResult WifiPortal::kickDevice(const std::string &device_id)
{
	return PortalResult(PortalErrorCode::UNKNOWN_ERROR, "Kick device not implemented");
}

PortalResult WifiPortal::refreshSession()
{
	return checkConnection();
}

std::string portalErrorCodeToString(PortalErrorCode code)
{
	switch (code)
	{
	case PortalErrorCode::SUCCESS:
		return "Success";
	case PortalErrorCode::NETWORK_CONNECTION_FAILED:
		return "Network connection failed";
	case PortalErrorCode::NETWORK_TIMEOUT:
		return "Network timeout";
	case PortalErrorCode::NETWORK_HTTP_ERROR:
		return "HTTP error";
	case PortalErrorCode::NETWORK_SSL_ERROR:
		return "SSL error";
	case PortalErrorCode::AUTH_INVALID_CREDENTIALS:
		return "Invalid username or password";
	case PortalErrorCode::AUTH_USER_NOT_FOUND:
		return "User not found";
	case PortalErrorCode::AUTH_PASSWORD_INCORRECT:
		return "Password incorrect";
	case PortalErrorCode::AUTH_ACCOUNT_SUSPENDED:
		return "Account suspended";
	case PortalErrorCode::AUTH_DOMAIN_ERROR:
		return "Domain error";
	case PortalErrorCode::AUTH_ALREADY_ONLINE:
		return "Already online";
	case PortalErrorCode::AUTH_MAX_SESSIONS_REACHED:
		return "Maximum sessions reached";
	case PortalErrorCode::PARSE_HTML_FAILED:
		return "HTML parsing failed";
	case PortalErrorCode::PARSE_JSON_FAILED:
		return "JSON parsing failed";
	case PortalErrorCode::PARSE_CONFIG_FAILED:
		return "Configuration parsing failed";
	case PortalErrorCode::PARSE_CALLBACK_FAILED:
		return "Callback parsing failed";
	case PortalErrorCode::SYSTEM_CRYPTO_ERROR:
		return "Cryptography error";
	case PortalErrorCode::SYSTEM_MEMORY_ERROR:
		return "Memory error";
	case PortalErrorCode::SYSTEM_FILE_ERROR:
		return "File error";
	case PortalErrorCode::UNKNOWN_ERROR:
	default:
		return "Unknown error";
	}
}

std::string networkProviderToString(NetworkProvider provider) {
	switch (provider) {
	case NetworkProvider::CAMPUS:
		return u8"校园网";
	case NetworkProvider::TELECOM:
		return u8"中国电信";
	case NetworkProvider::UNICOM:
		return u8"中国联通";
	case NetworkProvider::MOBILE:
		return u8"中国移动";
	default:
		return u8"未知";
	}
}

std::string formatBytes(uint64_t bytes)
{
	const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	int unit_index = 0;
	double size = static_cast<double>(bytes);

	while (size >= 1024.0 && unit_index < 4)
	{
		size /= 1024.0;
		unit_index++;
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
	return oss.str();
}

std::string formatDuration(uint64_t seconds) {
	uint64_t days = seconds / 86400;
	uint64_t hours = (seconds % 86400) / 3600;
	uint64_t minutes = (seconds % 3600) / 60;
	uint64_t secs = seconds % 60;

	std::ostringstream oss;
	if (days > 0) {
		oss << days << u8"天 ";
	}
	if (hours > 0 || days > 0) {
		oss << hours << u8"小时 ";
	}
	if (minutes > 0 || hours > 0 || days > 0) {
		oss << minutes << u8"分钟 ";
	}
	oss << secs << u8"秒";

	return oss.str();
}