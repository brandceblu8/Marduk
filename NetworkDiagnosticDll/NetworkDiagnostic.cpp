#include "pch.h"
#include "NetworkDiagnostic.h"
#include "HtmlTemplateManager.h"
#include "DiagnosticHTMLTemplates.h"
#include "SimpleTemplateProcessor.h"
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <wininet.h>
#include <windns.h>
#include <winhttp.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <numeric>
#include <thread>
#include <regex>
#include <iomanip>

// 定义宏以允许使用废弃的Winsock API（临时解决方案）
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
//#pragma comment(lib, "icmp.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "dnsapi.lib")

typedef HANDLE(WINAPI* IcmpCreateFileFunc)(VOID);
typedef BOOL(WINAPI* IcmpCloseHandleFunc)(HANDLE);
typedef DWORD(WINAPI* IcmpSendEchoFunc)(HANDLE, IPAddr, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD);

class ICMPHelper {
private:
    HMODULE hIcmpDll;
    IcmpCreateFileFunc pIcmpCreateFile;
    IcmpCloseHandleFunc pIcmpCloseHandle;
    IcmpSendEchoFunc pIcmpSendEcho;
    bool initialization_success;

public:
    ICMPHelper() : hIcmpDll(nullptr), pIcmpCreateFile(nullptr),
        pIcmpCloseHandle(nullptr), pIcmpSendEcho(nullptr),
        initialization_success(false) {
        try {
            // 尝试加载ICMP.DLL
            hIcmpDll = LoadLibraryA("ICMP.DLL");
            if (hIcmpDll) {
                pIcmpCreateFile = (IcmpCreateFileFunc)GetProcAddress(hIcmpDll, "IcmpCreateFile");
                pIcmpCloseHandle = (IcmpCloseHandleFunc)GetProcAddress(hIcmpDll, "IcmpCloseHandle");
                pIcmpSendEcho = (IcmpSendEchoFunc)GetProcAddress(hIcmpDll, "IcmpSendEcho");

                // 检查所有函数是否成功加载
                if (pIcmpCreateFile && pIcmpCloseHandle && pIcmpSendEcho) {
                    initialization_success = true;
                }
            }
        }
        catch (...) {
            cleanup();
        }
    }

    ~ICMPHelper() {
        cleanup();
    }

private:
    void cleanup() {
        if (hIcmpDll) {
            FreeLibrary(hIcmpDll);
            hIcmpDll = nullptr;
        }
        pIcmpCreateFile = nullptr;
        pIcmpCloseHandle = nullptr;
        pIcmpSendEcho = nullptr;
        initialization_success = false;
    }

public:
    bool IsAvailable() const {
        return initialization_success && hIcmpDll && pIcmpCreateFile && pIcmpCloseHandle && pIcmpSendEcho;
    }

    bool IsInitialized() const {
        return initialization_success;
    }

    HANDLE CreateFile() {
        if (IsAvailable() && pIcmpCreateFile) {
            return pIcmpCreateFile();
        }
        return INVALID_HANDLE_VALUE;
    }

    BOOL CloseHandle(HANDLE hIcmpFile) {
        if (IsAvailable() && pIcmpCloseHandle) {
            return pIcmpCloseHandle(hIcmpFile);
        }
        return FALSE;
    }

    DWORD SendEcho(HANDLE hIcmpFile, IPAddr DestinationAddress, LPVOID RequestData,
        WORD RequestSize, PIP_OPTION_INFORMATION RequestOptions,
        LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout) {
        if (IsAvailable() && pIcmpSendEcho) {
            return pIcmpSendEcho(hIcmpFile, DestinationAddress, RequestData, RequestSize,
                RequestOptions, ReplyBuffer, ReplySize, Timeout);
        }
        return 0;
    }
};

class NetworkDiagnostic::NetworkDiagnosticImpl {
private:
    std::unique_ptr<ICMPHelper> icmpHelper;
    std::map<int, std::string> pppoe_error_suggestions = {
        {678, u8"🔴 PPPoE错误678：远程计算机没有响应 - 检查网线连接、联系运营商或重启光猫"},
        {691, u8"🔴 PPPoE错误691：用户名或密码错误 - 请检查账号密码是否正确"},
        {619, u8"🔴 PPPoE错误619：指定的端口未连接 - 检查网络适配器状态"},
        {676, u8"🔴 PPPoE错误676：电话占线 - 可能有其他设备正在使用相同账号"},
        {720, u8"🔴 PPPoE错误720：协议配置错误 - 重新创建宽带连接或联系技术支持"},
        {769, u8"🔴 PPPoE错误769：指定的目标不可到达 - 检查网络适配器驱动程序"},
        {815, u8"🔴 PPPoE错误815：宽带网络连接配置可能不正确 - 重新配置网络连接"}
    };

public:
    NetworkDiagnosticImpl() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        try {
            icmpHelper = std::make_unique<ICMPHelper>();
        }
        catch (...) {
            icmpHelper = nullptr;
        }
    }

    ~NetworkDiagnosticImpl() {
        WSACleanup();
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        std::tm tm_buf;
        localtime_s(&tm_buf, &time_t);
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string getSystemInfo() {
        std::ostringstream oss;

        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        GetComputerNameA(computerName, &size);
        oss << "Computer Name: " << computerName << "\n";

        // 通过注册表获取Windows版本信息
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            char buffer[256];
            DWORD bufferSize = sizeof(buffer);

            // 产品名称
            if (RegQueryValueExA(hKey, "ProductName", NULL, NULL,
                (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                oss << "Windows Version: " << buffer << "\n";
            }

            // 版本号
            bufferSize = sizeof(buffer);
            if (RegQueryValueExA(hKey, "CurrentVersion", NULL, NULL,
                (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                oss << "Version Number: " << buffer << "\n";
            }

            // 构建号
            bufferSize = sizeof(buffer);
            if (RegQueryValueExA(hKey, "CurrentBuild", NULL, NULL,
                (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                oss << "Build Number: " << buffer << "\n";
            }

            RegCloseKey(hKey);
        }
        else {
            // 降级方案：使用GetVersion()
            DWORD version = 0;
            DWORD majorVersion = (DWORD)(LOBYTE(LOWORD(version)));
            DWORD minorVersion = (DWORD)(HIBYTE(LOWORD(version)));
            oss << "Windows Version: " << majorVersion << "." << minorVersion << "\n";
        }

        // 获取内存信息
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);

        oss << "Total Physical Memory: " << (memInfo.ullTotalPhys / 1024 / 1024) << " MB\n";
        oss << "Available Physical Memory: " << (memInfo.ullAvailPhys / 1024 / 1024) << " MB\n";

        return oss.str();
    }

    // 添加现代的IP地址转换辅助函数
    std::string ipv4ToString(DWORD ipAddress) {
        char buffer[INET_ADDRSTRLEN];
        struct in_addr addr;
        addr.s_addr = ipAddress;

        if (inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN) != NULL) {
            return std::string(buffer);
        }
        else {
            // 降级方案：手动转换
            unsigned char* bytes = (unsigned char*)&ipAddress;
            std::ostringstream oss;
            oss << (int)bytes[0] << "." << (int)bytes[1] << "."
                << (int)bytes[2] << "." << (int)bytes[3];
            return oss.str();
        }
    }

    DWORD stringToIPv4(const std::string& ipString) {
        struct in_addr addr;
        if (inet_pton(AF_INET, ipString.c_str(), &addr) == 1) {
            return addr.s_addr;
        }
        else {
            return INADDR_NONE;
        }
    }

    DiagnosticResult getNetworkInterfacesImpl(std::vector<NetworkInterface>& interfaces) {
        interfaces.clear();

        DWORD dwSize = 0;
        DWORD dwRetVal = 0;

        // 获取所需的缓冲区大小
        if (GetAdaptersInfo(NULL, &dwSize) != ERROR_BUFFER_OVERFLOW) {
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "Failed to get network adapter info buffer size");
        }

        PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(dwSize);
        if (pAdapterInfo == NULL) {
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "Memory allocation failed");
        }

        dwRetVal = GetAdaptersInfo(pAdapterInfo, &dwSize);
        if (dwRetVal != NO_ERROR) {
            free(pAdapterInfo);
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "GetAdaptersInfo failed with error: " + std::to_string(dwRetVal));
        }

        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            NetworkInterface iface;
            iface.name = pAdapter->AdapterName;
            iface.description = pAdapter->Description;

            // MAC地址
            std::ostringstream mac_oss;
            for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                if (i > 0) mac_oss << "-";
                mac_oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
                    << (int)pAdapter->Address[i];
            }
            iface.mac_address = mac_oss.str();

            // IP信息
            iface.ip_address = pAdapter->IpAddressList.IpAddress.String;
            iface.subnet_mask = pAdapter->IpAddressList.IpMask.String;
            iface.gateway = pAdapter->GatewayList.IpAddress.String;

            // 连接类型
            switch (pAdapter->Type) {
            case MIB_IF_TYPE_ETHERNET: iface.connection_type = "Ethernet"; break;
            case IF_TYPE_IEEE80211: iface.connection_type = "WiFi"; break;
            case MIB_IF_TYPE_PPP: iface.connection_type = "PPP"; break;
            default: iface.connection_type = "Other"; break;
            }

            iface.is_enabled = (strcmp(iface.ip_address.c_str(), "0.0.0.0") != 0);

            // 获取流量统计（需要额外的API调用）
            iface.bytes_sent = 0;
            iface.bytes_received = 0;

            interfaces.push_back(iface);
            pAdapter = pAdapter->Next;
        }

        free(pAdapterInfo);
        return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
            "Retrieved " + std::to_string(interfaces.size()) + " network interfaces");
    }

    DiagnosticResult getProxyConfigImpl(ProxyConfig& config) {
        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
        ZeroMemory(&proxyConfig, sizeof(proxyConfig));

        if (!WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig)) {
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_REGISTRY_ACCESS_FAILED,
                "Failed to get IE proxy configuration");
        }

        config.auto_detect = (proxyConfig.fAutoDetect == TRUE);

        if (proxyConfig.lpszAutoConfigUrl) {
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, proxyConfig.lpszAutoConfigUrl, -1, NULL, 0, NULL, NULL);
            if (size_needed > 0) {
                std::string result(size_needed - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, proxyConfig.lpszAutoConfigUrl, -1, &result[0], size_needed, NULL, NULL);
                config.auto_config_url = result;
            }
        }

        if (proxyConfig.lpszProxy) {
            std::wstring wstr(proxyConfig.lpszProxy);
            std::string proxy_str(wstr.begin(), wstr.end());

            // 解析代理服务器和端口
            size_t colon_pos = proxy_str.find(':');
            if (colon_pos != std::string::npos) {
                config.proxy_server = proxy_str.substr(0, colon_pos);
                config.proxy_port = proxy_str.substr(colon_pos + 1);
                config.proxy_enabled = true;
            }
        }

        if (proxyConfig.lpszProxyBypass) {
            std::wstring wstr(proxyConfig.lpszProxyBypass);
            config.proxy_bypass = std::string(wstr.begin(), wstr.end());
        }

        // 清理内存
        if (proxyConfig.lpszAutoConfigUrl) GlobalFree(proxyConfig.lpszAutoConfigUrl);
        if (proxyConfig.lpszProxy) GlobalFree(proxyConfig.lpszProxy);
        if (proxyConfig.lpszProxyBypass) GlobalFree(proxyConfig.lpszProxyBypass);

        return DiagnosticResult(DiagnosticErrorCode::SUCCESS, "Proxy configuration retrieved");
    }

    DiagnosticResult getRoutingTableImpl(std::vector<RouteInfo>& routes) {
        routes.clear();

        DWORD dwSize = 0;
        DWORD dwRetVal = GetIpForwardTable(NULL, &dwSize, 0);
        if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) {
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "Failed to get routing table size");
        }

        PMIB_IPFORWARDTABLE pIpForwardTable = (MIB_IPFORWARDTABLE*)malloc(dwSize);
        if (pIpForwardTable == NULL) {
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "Memory allocation failed for routing table");
        }

        dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 0);
        if (dwRetVal != NO_ERROR) {
            free(pIpForwardTable);
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "GetIpForwardTable failed with error: " + std::to_string(dwRetVal));
        }

        for (DWORD i = 0; i < pIpForwardTable->dwNumEntries; i++) {
            MIB_IPFORWARDROW* pRow = &pIpForwardTable->table[i];

            RouteInfo route;

            // 使用现代的IP地址转换函数
            route.destination = ipv4ToString(pRow->dwForwardDest);
            route.netmask = ipv4ToString(pRow->dwForwardMask);
            route.gateway = ipv4ToString(pRow->dwForwardNextHop);

            route.metric = pRow->dwForwardMetric1;
            route.route_interface = std::to_string(pRow->dwForwardIfIndex);

            routes.push_back(route);
        }

        free(pIpForwardTable);
        return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
            "Retrieved " + std::to_string(routes.size()) + " routing entries");
    }

    DiagnosticResult icmpApiPing(const std::string& target, PingResult& result) {
        result.target = target;
        result.success = false;

        if (!icmpHelper) {
            result.error_message = "ICMP Helper not initialized";
            return DiagnosticResult(DiagnosticErrorCode::NETWORK_PING_FAILED,
                "ICMP Helper not initialized");
        }

        if (!icmpHelper->IsAvailable()) {
            return DiagnosticResult(DiagnosticErrorCode::NETWORK_PING_FAILED,
                "ICMP API not available");
        }

        HANDLE hIcmpFile = icmpHelper->CreateFile();
        if (hIcmpFile == INVALID_HANDLE_VALUE) {
            return DiagnosticResult(DiagnosticErrorCode::NETWORK_PING_FAILED,
                "Unable to create ICMP handle");
        }

        // 解析主机名为IP
        struct addrinfo* addr_result = NULL;
        struct addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        IPAddr ipaddr = INADDR_NONE;
        if (getaddrinfo(target.c_str(), NULL, &hints, &addr_result) == 0) {
            struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)addr_result->ai_addr;
            ipaddr = sockaddr_ipv4->sin_addr.s_addr;
            freeaddrinfo(addr_result);
        }
        else {
            ipaddr = stringToIPv4(target);
        }

        if (ipaddr == INADDR_NONE) {
            icmpHelper->CloseHandle(hIcmpFile);
            result.error_message = "Could not resolve hostname";
            return DiagnosticResult(DiagnosticErrorCode::NETWORK_PING_FAILED,
                "Hostname resolution failed");
        }

        char SendData[] = "Hello World!";
        DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
        LPVOID ReplyBuffer = (VOID*)malloc(ReplySize);

        if (!ReplyBuffer) {
            icmpHelper->CloseHandle(hIcmpFile);
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_NETWORK_INFO_FAILED,
                "Memory allocation failed");
        }

        std::vector<double> times;
        int successful_pings = 0;
        const int ping_count = 4;

        for (int i = 0; i < ping_count; i++) {
            DWORD dwRetVal = icmpHelper->SendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
                NULL, ReplyBuffer, ReplySize, 3000);

            if (dwRetVal != 0) {
                PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
                if (pEchoReply->Status == IP_SUCCESS) {
                    times.push_back(pEchoReply->RoundTripTime);
                    successful_pings++;
                }
            }
            Sleep(100); // 间隔100ms
        }

        if (successful_pings > 0) {
            result.success = true;
            result.packet_loss_percent = ((ping_count - successful_pings) * 100) / ping_count;
            result.min_time_ms = *std::min_element(times.begin(), times.end());
            result.max_time_ms = *std::max_element(times.begin(), times.end());
            result.avg_time_ms = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        }
        else {
            result.success = false;
            result.packet_loss_percent = 100;
            result.error_message = "All ping packets lost";
        }

        free(ReplyBuffer);
        icmpHelper->CloseHandle(hIcmpFile);

        return DiagnosticResult(DiagnosticErrorCode::SUCCESS, "ICMP API ping completed");
    }

    DiagnosticResult pingTestImpl(const std::vector<std::string>& targets, std::vector<PingResult>& results) {
        results.clear();

        for (const auto& target : targets) {
            PingResult result;
            DiagnosticResult ping_result = icmpApiPing(target, result);

            // 即使单个ping失败，也要将结果添加到列表中
            results.push_back(result);

            // 如果有严重错误，记录但继续处理其他目标
            if (!ping_result.isSuccess() && ping_result.error_code != DiagnosticErrorCode::NETWORK_PING_FAILED) {
                // 可以在这里记录警告，但继续处理其他目标
            }
        }

        return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
            "Ping test completed for " + std::to_string(targets.size()) + " targets");
    }

    DiagnosticResult dnsTestImpl(const std::vector<std::string>& domains, std::vector<DnsQueryResult>& results) {
        results.clear();

        for (const auto& domain : domains) {
            DnsQueryResult result;
            result.hostname = domain;

            auto start_time = std::chrono::high_resolution_clock::now();

            // 使用Windows DNS API查询
            PDNS_RECORD pDnsRecord;
            DNS_STATUS status = DnsQuery_A(domain.c_str(), DNS_TYPE_A, DNS_QUERY_STANDARD,
                NULL, &pDnsRecord, NULL);

            auto end_time = std::chrono::high_resolution_clock::now();
            result.query_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

            if (status == 0 && pDnsRecord) {
                result.success = true;

                PDNS_RECORD pNext = pDnsRecord;
                while (pNext) {
                    if (pNext->wType == DNS_TYPE_A) {
                        // 使用现代的IP地址转换
                        result.ip_addresses.push_back(ipv4ToString(pNext->Data.A.IpAddress));
                    }
                    pNext = pNext->pNext;
                }

                DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
            }
            else {
                result.success = false;
                result.error_message = "DNS query failed with status: " + std::to_string(status);
            }

            // 获取使用的DNS服务器（简化版）
            result.dns_server_used = "System Default";

            results.push_back(result);
        }

        return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
            "DNS test completed for " + std::to_string(domains.size()) + " domains");
    }

    DiagnosticResult tcpTestImpl(const std::vector<std::pair<std::string, int>>& targets,
        std::vector<TcpConnectionResult>& results) {
        results.clear();

        for (const auto& target : targets) {
            TcpConnectionResult result;
            result.target_host = target.first;
            result.target_port = target.second;

            auto start_time = std::chrono::high_resolution_clock::now();

            // 创建套接字
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                result.success = false;
                result.error_message = "Failed to create socket";
                results.push_back(result);
                continue;
            }

            // 设置超时
            DWORD timeout = 5000; // 5秒
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

            // 解析主机名
            struct addrinfo* addr_result = NULL;
            struct addrinfo hints;
            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            std::string port_str = std::to_string(target.second);
            int getaddr_result = getaddrinfo(target.first.c_str(), port_str.c_str(), &hints, &addr_result);

            if (getaddr_result != 0) {
                result.success = false;
                result.error_message = "Failed to resolve hostname";
                closesocket(sock);
                results.push_back(result);
                continue;
            }

            // 尝试连接
            int connect_result = connect(sock, addr_result->ai_addr, (int)addr_result->ai_addrlen);

            auto end_time = std::chrono::high_resolution_clock::now();
            result.connection_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

            if (connect_result == 0) {
                result.success = true;
            }
            else {
                result.success = false;
                result.error_message = "Connection failed with error: " + std::to_string(WSAGetLastError());
            }

            freeaddrinfo(addr_result);
            closesocket(sock);
            results.push_back(result);
        }

        return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
            "TCP test completed for " + std::to_string(targets.size()) + " targets");
    }

    DiagnosticResult runFullDiagnosticImpl(const DiagnosticConfig& config) {
        DiagnosticResult result;
        result.timestamp = getCurrentTimestamp();
        result.system_info = getSystemInfo();

        // 获取网络配置
        auto interfaces_result = getNetworkInterfacesImpl(result.network_interfaces);
        if (!interfaces_result.isSuccess()) {
            return interfaces_result;
        }

        auto proxy_result = getProxyConfigImpl(result.proxy_config);
        // 代理配置失败不是致命错误，继续执行

        auto routing_result = getRoutingTableImpl(result.routing_table);
        // 路由表获取失败也不是致命错误

        // 执行网络测试
        auto ping_result = pingTestImpl(config.ping_targets, result.ping_results);
        //auto ping_result = rawSocketPingImpl(config.ping_targets, result.ping_results);
        //auto ping_result = icmpApiPing(config.ping_targets, result.ping_results);
        auto dns_result = dnsTestImpl(config.dns_test_domains, result.dns_results);
        auto tcp_result = tcpTestImpl(config.tcp_test_targets, result.tcp_results);

        result.error_code = DiagnosticErrorCode::SUCCESS;
        result.error_message = "Full diagnostic completed successfully";

        return result;
    }

    DiagnosticResult generateReportImpl(const DiagnosticResult& diagnostic_result, const std::string& output_path) {
        std::ofstream file(output_path);
        if (!file.is_open()) {
            return DiagnosticResult(DiagnosticErrorCode::FILE_CREATE_FAILED,
                "Failed to create report file: " + output_path);
        }

        file << "===== 网络诊断报告 =====\n";
        file << "生成时间: " << diagnostic_result.timestamp << "\n\n";

        // 系统信息
        file << "=== 系统信息 ===\n";
        file << diagnostic_result.system_info << "\n";

        // 网络接口
        file << "=== 网络接口 ===\n";
        for (const auto& iface : diagnostic_result.network_interfaces) {
            file << "接口: " << iface.description << "\n";
            file << "  名称: " << iface.name << "\n";
            file << "  MAC地址: " << iface.mac_address << "\n";
            file << "  IP地址: " << iface.ip_address << "\n";
            file << "  子网掩码: " << iface.subnet_mask << "\n";
            file << "  网关: " << iface.gateway << "\n";
            file << "  类型: " << iface.connection_type << "\n";
            file << "  状态: " << (iface.is_enabled ? "启用" : "禁用") << "\n\n";
        }

        // 代理配置
        file << "=== 代理配置 ===\n";
        file << "代理启用: " << (diagnostic_result.proxy_config.proxy_enabled ? "是" : "否") << "\n";
        if (diagnostic_result.proxy_config.proxy_enabled) {
            file << "代理服务器: " << diagnostic_result.proxy_config.proxy_server << "\n";
            file << "代理端口: " << diagnostic_result.proxy_config.proxy_port << "\n";
        }
        file << "自动检测: " << (diagnostic_result.proxy_config.auto_detect ? "是" : "否") << "\n";
        if (!diagnostic_result.proxy_config.auto_config_url.empty()) {
            file << "自动配置URL: " << diagnostic_result.proxy_config.auto_config_url << "\n";
        }
        file << "\n";

        // Ping测试结果
        file << "=== Ping 测试结果 ===\n";
        for (const auto& ping : diagnostic_result.ping_results) {
            file << "目标: " << ping.target << "\n";
            file << "  成功: " << (ping.success ? "是" : "否") << "\n";
            if (ping.success) {
                file << "  丢包率: " << ping.packet_loss_percent << "%\n";
                file << "  最小时间: " << ping.min_time_ms << "ms\n";
                file << "  最大时间: " << ping.max_time_ms << "ms\n";
                file << "  平均时间: " << ping.avg_time_ms << "ms\n";
            }
            else {
                file << "  错误: " << ping.error_message << "\n";
            }
            file << "\n";
        }

        // DNS测试结果
        file << "=== DNS 测试结果 ===\n";
        for (const auto& dns : diagnostic_result.dns_results) {
            file << "域名: " << dns.hostname << "\n";
            file << "  成功: " << (dns.success ? "是" : "否") << "\n";
            file << "  查询时间: " << dns.query_time_ms << "ms\n";
            if (dns.success) {
                file << "  IP地址:\n";
                for (const auto& ip : dns.ip_addresses) {
                    file << "    " << ip << "\n";
                }
                file << "  使用的DNS服务器: " << dns.dns_server_used << "\n";
            }
            else {
                file << "  错误: " << dns.error_message << "\n";
            }
            file << "\n";
        }

        // TCP连接测试结果
        file << "=== TCP 连接测试结果 ===\n";
        for (const auto& tcp : diagnostic_result.tcp_results) {
            file << "目标: " << tcp.target_host << ":" << tcp.target_port << "\n";
            file << "  成功: " << (tcp.success ? "是" : "否") << "\n";
            file << "  连接时间: " << tcp.connection_time_ms << "ms\n";
            if (!tcp.success) {
                file << "  错误: " << tcp.error_message << "\n";
            }
            file << "\n";
        }

        // 路由表
        file << "=== 路由表 ===\n";
        file << "目标地址\t\t子网掩码\t\t网关\t\t接口\t跃点数\n";
        for (const auto& route : diagnostic_result.routing_table) {
            file << route.destination << "\t\t"
                << route.netmask << "\t\t"
                << route.gateway << "\t\t"
                << route.route_interface << "\t"
                << route.metric << "\n";
        }

        file.close();

        return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
            "Text report generated successfully: " + output_path);
    }

    DiagnosticResult generateHTMLReportImpl(const DiagnosticResult& diagnostic_result, const std::string& output_path) {
        try {
            // 准备模板变量
            SimpleTemplateProcessor::Variables variables;

            // 基本信息
            variables["timestamp"] = diagnostic_result.timestamp;
            variables["version"] = u8"西电校园网辅助工具 v1.0";
            variables["system_info"] = SimpleTemplateProcessor::escapeHtml(diagnostic_result.system_info);

            // 计算统计数据
            int active_interfaces = 0;
            for (const auto& iface : diagnostic_result.network_interfaces) {
                if (iface.is_enabled && iface.ip_address != "0.0.0.0") {
                    active_interfaces++;
                }
            }

            int successful_pings = 0;
            for (const auto& ping : diagnostic_result.ping_results) {
                if (ping.success) successful_pings++;
            }

            int successful_dns = 0;
            for (const auto& dns : diagnostic_result.dns_results) {
                if (dns.success) successful_dns++;
            }

            int successful_tcp = 0;
            for (const auto& tcp : diagnostic_result.tcp_results) {
                if (tcp.success) successful_tcp++;
            }

            // 摘要统计
            variables["summary_active_interfaces"] = std::to_string(active_interfaces);
            variables["summary_successful_pings"] = std::to_string(successful_pings);
            variables["summary_total_pings"] = std::to_string(diagnostic_result.ping_results.size());
            variables["summary_successful_dns"] = std::to_string(successful_dns);
            variables["summary_total_dns"] = std::to_string(diagnostic_result.dns_results.size());
            variables["summary_successful_tcp"] = std::to_string(successful_tcp);
            variables["summary_total_tcp"] = std::to_string(diagnostic_result.tcp_results.size());

            // 生成网络接口表格行
            std::string network_interfaces_rows;
            for (const auto& iface : diagnostic_result.network_interfaces) {
                SimpleTemplateProcessor::Variables row_vars;
                row_vars["name"] = SimpleTemplateProcessor::escapeHtml(iface.name);
                row_vars["description"] = SimpleTemplateProcessor::escapeHtml(iface.description);
                row_vars["mac_address"] = SimpleTemplateProcessor::escapeHtml(iface.mac_address);
                row_vars["ip_address"] = SimpleTemplateProcessor::escapeHtml(iface.ip_address);
                row_vars["subnet_mask"] = SimpleTemplateProcessor::escapeHtml(iface.subnet_mask);
                row_vars["gateway"] = SimpleTemplateProcessor::escapeHtml(iface.gateway);
                row_vars["connection_type"] = SimpleTemplateProcessor::escapeHtml(iface.connection_type);

                if (iface.is_enabled) {
                    row_vars["status_html"] = u8"<span class=\"success\">启用</span>";
                }
                else {
                    row_vars["status_html"] = u8"<span class=\"error\">禁用</span>";
                }

                network_interfaces_rows += SimpleTemplateProcessor::render(
                    DiagnosticHTMLTemplates::NETWORK_INTERFACE_ROW_TEMPLATE, row_vars);
            }
            variables["network_interfaces_rows"] = network_interfaces_rows;

            // 生成代理配置内容
            std::string proxy_config_content;
            if (diagnostic_result.proxy_config.proxy_enabled) {
                proxy_config_content += u8"<p><strong>🔧 代理状态:</strong> <span class=\"success\">已启用</span></p>\n";
                proxy_config_content += u8"<p><strong>🌐 代理服务器:</strong> <code>" +
                    SimpleTemplateProcessor::escapeHtml(diagnostic_result.proxy_config.proxy_server) + u8"</code></p>\n";
                proxy_config_content += u8"<p><strong>🔌 代理端口:</strong> <code>" +
                    SimpleTemplateProcessor::escapeHtml(diagnostic_result.proxy_config.proxy_port) + u8"</code></p>\n";
            }
            else {
                proxy_config_content += u8"<p><strong>🔧 代理状态:</strong> <span class=\"error\">未启用</span></p>\n";
            }

            if (diagnostic_result.proxy_config.auto_detect) {
                proxy_config_content += u8"<p><strong>🔍 自动检测:</strong> <span class=\"success\">已启用</span></p>\n";
            }
            else {
                proxy_config_content += u8"<p><strong>🔍 自动检测:</strong> <span class=\"error\">未启用</span></p>\n";
            }

            if (!diagnostic_result.proxy_config.auto_config_url.empty()) {
                proxy_config_content += u8"<p><strong>⚙️ 自动配置URL:</strong> <code>" +
                    SimpleTemplateProcessor::escapeHtml(diagnostic_result.proxy_config.auto_config_url) + u8"</code></p>\n";
            }
            variables["proxy_config_content"] = proxy_config_content;

            // 生成Ping测试结果表格行
            std::string ping_results_rows;
            for (const auto& ping : diagnostic_result.ping_results) {
                SimpleTemplateProcessor::Variables row_vars;
                row_vars["target"] = SimpleTemplateProcessor::escapeHtml(ping.target);

                if (ping.success) {
                    row_vars["status_html"] = u8"<span class=\"success\">连接成功</span>";
                    row_vars["packet_loss"] = std::to_string(ping.packet_loss_percent) + u8"%";
                    row_vars["min_time"] = std::to_string(static_cast<int>(ping.min_time_ms)) + u8"ms";
                    row_vars["max_time"] = std::to_string(static_cast<int>(ping.max_time_ms)) + u8"ms";
                    row_vars["avg_time"] = std::to_string(static_cast<int>(ping.avg_time_ms)) + u8"ms";
                    row_vars["error_message"] = u8"-";
                }
                else {
                    row_vars["status_html"] = u8"<span class=\"error\">连接失败</span>";
                    row_vars["packet_loss"] = u8"<span class=\"error\">100%</span>";
                    row_vars["min_time"] = u8"-";
                    row_vars["max_time"] = u8"-";
                    row_vars["avg_time"] = u8"-";
                    row_vars["error_message"] = u8"<span class=\"error\">" +
                        SimpleTemplateProcessor::escapeHtml(ping.error_message) + u8"</span>";
                }

                ping_results_rows += SimpleTemplateProcessor::render(
                    DiagnosticHTMLTemplates::PING_RESULT_ROW_TEMPLATE, row_vars);
            }
            variables["ping_results_rows"] = ping_results_rows;

            // 生成DNS测试结果表格行
            std::string dns_results_rows;
            for (const auto& dns : diagnostic_result.dns_results) {
                SimpleTemplateProcessor::Variables row_vars;
                row_vars["hostname"] = SimpleTemplateProcessor::escapeHtml(dns.hostname);
                row_vars["query_time"] = std::to_string(static_cast<int>(dns.query_time_ms));

                if (dns.success) {
                    row_vars["status_html"] = u8"<span class=\"success\">解析成功</span>";

                    // 构建IP地址列表
                    std::string ip_addresses_html;
                    for (size_t i = 0; i < dns.ip_addresses.size(); ++i) {
                        if (i > 0) ip_addresses_html += u8"<br>";
                        ip_addresses_html += u8"<code>" +
                            SimpleTemplateProcessor::escapeHtml(dns.ip_addresses[i]) + u8"</code>";
                    }
                    row_vars["ip_addresses_html"] = ip_addresses_html;
                    row_vars["dns_server"] = u8"<code>" +
                        SimpleTemplateProcessor::escapeHtml(dns.dns_server_used) + u8"</code>";
                    row_vars["error_message"] = u8"-";
                }
                else {
                    row_vars["status_html"] = u8"<span class=\"error\">解析失败</span>";
                    row_vars["ip_addresses_html"] = u8"-";
                    row_vars["dns_server"] = u8"-";
                    row_vars["error_message"] = u8"<span class=\"error\">" +
                        SimpleTemplateProcessor::escapeHtml(dns.error_message) + u8"</span>";
                }

                dns_results_rows += SimpleTemplateProcessor::render(
                    DiagnosticHTMLTemplates::DNS_RESULT_ROW_TEMPLATE, row_vars);
            }
            variables["dns_results_rows"] = dns_results_rows;

            // 生成TCP连接测试结果表格行
            std::string tcp_results_rows;
            for (const auto& tcp : diagnostic_result.tcp_results) {
                SimpleTemplateProcessor::Variables row_vars;
                row_vars["target_host"] = SimpleTemplateProcessor::escapeHtml(tcp.target_host);
                row_vars["target_port"] = std::to_string(tcp.target_port);
                row_vars["connection_time"] = std::to_string(static_cast<int>(tcp.connection_time_ms));

                if (tcp.success) {
                    row_vars["status_html"] = u8"<span class=\"success\">连接成功</span>";
                    row_vars["error_message"] = u8"-";
                }
                else {
                    row_vars["status_html"] = u8"<span class=\"error\">连接失败</span>";
                    row_vars["error_message"] = u8"<span class=\"error\">" +
                        SimpleTemplateProcessor::escapeHtml(tcp.error_message) + u8"</span>";
                }

                tcp_results_rows += SimpleTemplateProcessor::render(
                    DiagnosticHTMLTemplates::TCP_RESULT_ROW_TEMPLATE, row_vars);
            }
            variables["tcp_results_rows"] = tcp_results_rows;

            // 生成路由表部分
            std::string routing_table_section;
            if (!diagnostic_result.routing_table.empty()) {
                std::string routing_table_rows;
                for (const auto& route : diagnostic_result.routing_table) {
                    SimpleTemplateProcessor::Variables row_vars;
                    row_vars["destination"] = SimpleTemplateProcessor::escapeHtml(route.destination);
                    row_vars["netmask"] = SimpleTemplateProcessor::escapeHtml(route.netmask);
                    row_vars["gateway"] = SimpleTemplateProcessor::escapeHtml(route.gateway);
                    row_vars["route_interface"] = SimpleTemplateProcessor::escapeHtml(route.route_interface);
                    row_vars["metric"] = std::to_string(route.metric);

                    routing_table_rows += SimpleTemplateProcessor::render(
                        DiagnosticHTMLTemplates::ROUTING_TABLE_ROW_TEMPLATE, row_vars);
                }

                SimpleTemplateProcessor::Variables section_vars;
                section_vars["routing_table_rows"] = routing_table_rows;
                routing_table_section = SimpleTemplateProcessor::render(
                    DiagnosticHTMLTemplates::ROUTING_TABLE_SECTION_TEMPLATE, section_vars);
            }
            variables["routing_table_section"] = routing_table_section;

            // 生成诊断建议
            std::string diagnostic_suggestions;
            std::vector<std::string> suggestions;
            bool has_network_issues = false;

            // 检查连通性问题
            if (successful_pings < static_cast<int>(diagnostic_result.ping_results.size()) / 2) {
                has_network_issues = true;
                suggestions.push_back(u8"🔴 网络连接异常：建议检查网络连接状态、防火墙设置或联系网络管理员");
            }

            // 检查DNS问题
            if (successful_dns < static_cast<int>(diagnostic_result.dns_results.size()) / 2) {
                has_network_issues = true;
                suggestions.push_back(u8"🟡 DNS解析异常：建议更换DNS服务器(如8.8.8.8)或检查DNS配置");
            }

            // 检查TCP连接问题
            if (successful_tcp < static_cast<int>(diagnostic_result.tcp_results.size()) / 2) {
                has_network_issues = true;
                suggestions.push_back(u8"🟠 TCP连接异常：建议检查目标服务器状态或端口可用性");
            }

            // 检查活跃接口
            if (active_interfaces == 0) {
                has_network_issues = true;
                suggestions.push_back(u8"🔴 无活跃网络接口：请检查网络适配器驱动或物理连接");
            }


            if (!has_network_issues) {
                diagnostic_suggestions = u8"<p style=\"color: #27ae60;\">🎉 网络状态良好！所有测试项目基本正常。</p>";
            }
            else {
                diagnostic_suggestions = u8"<ul>";
                for (const auto& suggestion : suggestions) {
                    diagnostic_suggestions += u8"<li>" + SimpleTemplateProcessor::escapeHtml(suggestion) + u8"</li>";
                }
                diagnostic_suggestions += u8"</ul>";
            }
            variables["diagnostic_suggestions"] = diagnostic_suggestions;

            // 渲染最终HTML
            std::string html_content = SimpleTemplateProcessor::render(
                DiagnosticHTMLTemplates::DIAGNOSTIC_REPORT_TEMPLATE, variables);

            // 写入文件
            std::ofstream file(output_path, std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                return DiagnosticResult(DiagnosticErrorCode::FILE_CREATE_FAILED,
                    "Failed to create HTML report file: " + output_path);
            }

            file.write(html_content.c_str(), html_content.length());
            file.close();

            return DiagnosticResult(DiagnosticErrorCode::SUCCESS,
                "Enhanced HTML report generated successfully: " + output_path);
        }
        catch (const std::exception& e) {
            return DiagnosticResult(DiagnosticErrorCode::SYSTEM_UNKNOWN_ERROR,
                "Exception occurred while generating HTML report: " + std::string(e.what()));
        }
    }
};


NetworkDiagnostic::NetworkDiagnostic()
    : impl(std::make_unique<NetworkDiagnosticImpl>()) {
}

NetworkDiagnostic::~NetworkDiagnostic() = default;

DiagnosticResult NetworkDiagnostic::runFullDiagnostic(const DiagnosticConfig& config) {
    return impl->runFullDiagnosticImpl(config);
}

DiagnosticResult NetworkDiagnostic::getNetworkInterfaces(std::vector<NetworkInterface>& interfaces) {
    return impl->getNetworkInterfacesImpl(interfaces);
}

DiagnosticResult NetworkDiagnostic::getProxyConfig(ProxyConfig& config) {
    return impl->getProxyConfigImpl(config);
}

DiagnosticResult NetworkDiagnostic::getRoutingTable(std::vector<RouteInfo>& routes) {
    return impl->getRoutingTableImpl(routes);
}

DiagnosticResult NetworkDiagnostic::pingTest(const std::vector<std::string>& targets, std::vector<PingResult>& results) {
    return impl->pingTestImpl(targets, results);
}

DiagnosticResult NetworkDiagnostic::dnsTest(const std::vector<std::string>& domains, std::vector<DnsQueryResult>& results) {
    return impl->dnsTestImpl(domains, results);
}

DiagnosticResult NetworkDiagnostic::tcpTest(const std::vector<std::pair<std::string, int>>& targets, std::vector<TcpConnectionResult>& results) {
    return impl->tcpTestImpl(targets, results);
}

DiagnosticResult NetworkDiagnostic::generateReport(const DiagnosticResult& result, const std::string& output_path) {
    return impl->generateReportImpl(result, output_path);
}

DiagnosticResult NetworkDiagnostic::generateHTMLReport(const DiagnosticResult& result, const std::string& output_path) {
    return impl->generateHTMLReportImpl(result, output_path);
}