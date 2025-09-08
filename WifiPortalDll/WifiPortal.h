#pragma once

// 导出宏定义
#ifdef WIFIPORTALDLL_EXPORTS
#define WIFIPORTAL_API __declspec(dllexport)
#else
#define WIFIPORTAL_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <memory>
#include <map>

// 禁用 STL 导出警告
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#pragma warning(disable: 4819)
#endif

// 校园网门户错误码
enum class WIFIPORTAL_API PortalErrorCode : int {
    SUCCESS = 0,

    // 网络错误 (5000-5099)
    NETWORK_CONNECTION_FAILED = 5001,
    NETWORK_TIMEOUT = 5002,
    NETWORK_HTTP_ERROR = 5003,
    NETWORK_SSL_ERROR = 5004,

    // 认证错误 (5100-5199)
    AUTH_INVALID_CREDENTIALS = 5101,
    AUTH_USER_NOT_FOUND = 5102,
    AUTH_PASSWORD_INCORRECT = 5103,
    AUTH_ACCOUNT_SUSPENDED = 5104,
    AUTH_DOMAIN_ERROR = 5105,
    AUTH_ALREADY_ONLINE = 5106,
    AUTH_MAX_SESSIONS_REACHED = 5107,

    // 解析错误 (5200-5299)
    PARSE_HTML_FAILED = 5201,
    PARSE_JSON_FAILED = 5202,
    PARSE_CONFIG_FAILED = 5203,
    PARSE_CALLBACK_FAILED = 5204,

    // 系统错误 (5300-5399)
    SYSTEM_CRYPTO_ERROR = 5301,
    SYSTEM_MEMORY_ERROR = 5302,
    SYSTEM_FILE_ERROR = 5303,

    // 其他错误
    UNKNOWN_ERROR = 5999
};

// 网络运营商类型
enum class WIFIPORTAL_API NetworkProvider {
    CAMPUS = 0,    // 校园网 (默认)
    TELECOM = 1,   // 中国电信 (@dx)
    UNICOM = 2,    // 中国联通 (@lt)
    MOBILE = 3     // 中国移动 (@yd)
};

// 在线设备信息 - 使用内联实现避免导出问题
struct OnlineDevice {
    std::string rad_online_id;
    std::string ip_address;
    std::string ipv6_address;
    std::string class_name;
    std::string os_name;

    // 内联构造函数和析构函数
    OnlineDevice() = default;

    OnlineDevice(const std::string& id, const std::string& ip, const std::string& ipv6,
        const std::string& class_type, const std::string& os)
        : rad_online_id(id), ip_address(ip), ipv6_address(ipv6),
        class_name(class_type), os_name(os) {
    }

    // 拷贝构造函数
    OnlineDevice(const OnlineDevice& other) = default;
    OnlineDevice& operator=(const OnlineDevice& other) = default;

    // 移动构造函数
    OnlineDevice(OnlineDevice&& other) noexcept = default;
    OnlineDevice& operator=(OnlineDevice&& other) noexcept = default;

    ~OnlineDevice() = default;
};

// 用户信息 - 使用内联实现
struct PortalUserInfo {
    std::string username;
    std::string real_name;
    std::string domain;
    std::string billing_name;
    std::string products_name;

    // IP 信息
    std::string online_ip;
    std::string online_ipv6;
    std::string user_mac;

    // 流量信息 (字节)
    uint64_t bytes_in = 0;           // 下行流量
    uint64_t bytes_out = 0;          // 上行流量
    uint64_t all_bytes = 0;          // 总流量
    uint64_t sum_bytes = 0;          // 历史总流量
    uint64_t remain_bytes = 0;       // 剩余流量

    // 时间信息 (秒)
    uint64_t sum_seconds = 0;        // 在线总时长
    uint64_t remain_seconds = 0;     // 剩余时长
    uint64_t add_time = 0;           // 登录时间
    uint64_t keepalive_time = 0;     // 保活时间

    // 余额信息 (元)
    double user_balance = 0.0;       // 账户余额
    double user_charge = 0.0;        // 当前费用
    double wallet_balance = 0.0;     // 钱包余额

    // 设备信息
    int online_device_total = 0;
    std::vector<OnlineDevice> online_devices;

    // 其他信息
    std::string group_id;
    std::string package_id;
    std::string products_id;
    uint64_t server_flag = 0;

    // 内联构造函数和析构函数
    PortalUserInfo() = default;
    PortalUserInfo(const PortalUserInfo& other) = default;
    PortalUserInfo& operator=(const PortalUserInfo& other) = default;
    PortalUserInfo(PortalUserInfo&& other) noexcept = default;
    PortalUserInfo& operator=(PortalUserInfo&& other) noexcept = default;
    ~PortalUserInfo() = default;
};

// 操作结果 - 使用内联实现
struct PortalResult {
    PortalErrorCode error_code;
    std::string error_message;
    PortalUserInfo user_info;

    // 内联构造函数
    PortalResult(PortalErrorCode code = PortalErrorCode::SUCCESS,
        const std::string& message = "")
        : error_code(code), error_message(message) {
    }

    // 拷贝和移动构造函数
    PortalResult(const PortalResult& other) = default;
    PortalResult& operator=(const PortalResult& other) = default;
    PortalResult(PortalResult&& other) noexcept = default;
    PortalResult& operator=(PortalResult&& other) noexcept = default;

    // 内联析构函数
    ~PortalResult() = default;

    // 内联成员函数
    bool isSuccess() const { return error_code == PortalErrorCode::SUCCESS; }
};

// 登录结果 - 使用内联实现
struct LoginResult {
    PortalErrorCode error_code;
    std::string error_message;

    // 登录响应详细信息
    std::string client_ip;
    std::string online_ip;
    std::string server_version;
    int ecode = 0;
    std::string res_status;

    // 内联构造函数
    LoginResult(PortalErrorCode code = PortalErrorCode::SUCCESS,
        const std::string& message = "")
        : error_code(code), error_message(message) {
    }

    // 拷贝和移动构造函数
    LoginResult(const LoginResult& other) = default;
    LoginResult& operator=(const LoginResult& other) = default;
    LoginResult(LoginResult&& other) noexcept = default;
    LoginResult& operator=(LoginResult&& other) noexcept = default;

    // 内联析构函数
    ~LoginResult() = default;

    // 内联成员函数
    bool isSuccess() const { return error_code == PortalErrorCode::SUCCESS; }
};

// 校园网门户管理器
class WIFIPORTAL_API WifiPortal {
public:
    WifiPortal();
    ~WifiPortal();

    // 禁用拷贝构造和赋值
    WifiPortal(const WifiPortal&) = delete;
    WifiPortal& operator=(const WifiPortal&) = delete;

    // 基础操作
    PortalResult checkConnection();                    // 检查网络连接状态
    LoginResult login(const std::string& username,
        const std::string& password,
        NetworkProvider provider = NetworkProvider::CAMPUS);
    PortalResult logout();                            // 注销登录

    // 信息获取
    PortalResult getUserInfo();                       // 获取当前用户详细信息
    PortalResult getDeviceList();                     // 获取在线设备列表
    PortalResult getTrafficInfo();                    // 获取流量使用情况

    // 状态查询
    bool isLoggedIn();                               // 检查是否已登录
    std::string getCurrentIP();                      // 获取当前IP地址

    // 高级功能
    PortalResult kickDevice(const std::string& device_id);  // 踢掉指定设备 (如果支持)
    PortalResult refreshSession();                   // 刷新会话状态

    // 配置设置
    void setTimeout(int seconds);                    // 设置超时时间
    void setUserAgent(const std::string& ua);       // 设置User-Agent
    void setDebugMode(bool enabled);                // 开启/关闭调试模式

private:
    class WifiPortalImpl;
    std::unique_ptr<WifiPortalImpl> impl;
};

WIFIPORTAL_API std::string portalErrorCodeToString(PortalErrorCode code);
WIFIPORTAL_API std::string networkProviderToString(NetworkProvider provider);
WIFIPORTAL_API std::string formatBytes(uint64_t bytes);
WIFIPORTAL_API std::string formatDuration(uint64_t seconds);

// 恢复警告
#ifdef _MSC_VER
#pragma warning(pop)
#endif