/**
 * @file main.cpp
 * @brief MardukElevated — 提权代理子进程
 *
 * 此程序由 MardukGui 通过 ShellExecuteEx + "runas" 启动，
 * 以管理员权限执行特定的 DNS / 网络操作后立即退出。
 *
 * 命令格式：
 *   MardukElevated.exe --elevated-dns-deploy
 *   MardukElevated.exe --elevated-dns-add <ip> <hostname>
 *   MardukElevated.exe --elevated-dns-remove <hostname>
 *   MardukElevated.exe --elevated-dns-clear
 *   MardukElevated.exe --elevated-dns-restart
 *   MardukElevated.exe --elevated-pasopt
 *
 * 退出码：
 *   0 = 成功
 *   1 = 参数错误
 *   2 = 操作失败
 */

#include <string>
#include <vector>
#include <iostream>
#include <Windows.h>

#include "../DNSManagerDll/DNSManager.h"

// 退出码定义
constexpr int EXIT_SUCCESS_CODE = 0;
constexpr int EXIT_BAD_ARGS = 1;
constexpr int EXIT_OPERATION_FAILED = 2;

int handleDnsDeploy();
int handleDnsAdd(const std::wstring& ip, const std::wstring& hostname);
int handleDnsRemove(const std::wstring& hostname);
int handleDnsClear();
int handleDnsRestart();
int handlePasopt();

std::string wstr_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
        (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
        (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}


int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2) {
        return EXIT_BAD_ARGS;
    }

    std::wstring command = argv[1];

    if (command == L"--elevated-dns-deploy") {
        return handleDnsDeploy();
    }
    else if (command == L"--elevated-dns-add" && argc >= 4) {
        return handleDnsAdd(argv[2], argv[3]);
    }
    else if (command == L"--elevated-dns-remove" && argc >= 3) {
        return handleDnsRemove(argv[2]);
    }
    else if (command == L"--elevated-dns-clear") {
        return handleDnsClear();
    }
    else if (command == L"--elevated-dns-restart") {
        return handleDnsRestart();
    }
    else if (command == L"--elevated-pasopt") {
        return handlePasopt();
    }

    return EXIT_BAD_ARGS;
}

int handleDnsDeploy()
{
    DNSManager mgr;
    DnsResult initRes = mgr.initialize();
    if (!initRes.isSuccess()) return EXIT_OPERATION_FAILED;

    DnsServiceInfo info = mgr.getServiceInfo();

    if (!info.is_installed) {
        DnsResult installRes = mgr.installService();
        if (!installRes.isSuccess()) return EXIT_OPERATION_FAILED;
    }

    info = mgr.getServiceInfo();
    if (!info.is_running) {
        DnsResult startRes = mgr.startService();
        if (!startRes.isSuccess()) return EXIT_OPERATION_FAILED;
    }

    return EXIT_SUCCESS_CODE;
}


int handleDnsAdd(const std::wstring& ip, const std::wstring& hostname)
{
    DNSManager mgr;
    DnsResult initRes = mgr.initialize();
    if (!initRes.isSuccess()) return EXIT_OPERATION_FAILED;

    std::string u8ip = wstr_to_utf8(ip);
    std::string u8host = wstr_to_utf8(hostname);

    DnsResult result = mgr.addRule(u8ip, u8host);
    if (!result.isSuccess() && result.error_code != DnsErrorCode::CONFIG_RULE_EXISTS) {
        return EXIT_OPERATION_FAILED;
    }

    // 重启服务使规则生效
    mgr.restartService();
    return EXIT_SUCCESS_CODE;
}

int handleDnsRemove(const std::wstring& hostname)
{
    DNSManager mgr;
    DnsResult initRes = mgr.initialize();
    if (!initRes.isSuccess()) return EXIT_OPERATION_FAILED;

    std::string u8host = wstr_to_utf8(hostname);
    DnsResult result = mgr.removeRule(u8host);
    if (!result.isSuccess()) return EXIT_OPERATION_FAILED;

    mgr.restartService();
    return EXIT_SUCCESS_CODE;
}

int handleDnsClear()
{
    DNSManager mgr;
    DnsResult initRes = mgr.initialize();
    if (!initRes.isSuccess()) return EXIT_OPERATION_FAILED;

    DnsResult result = mgr.clearAllRules();
    if (!result.isSuccess()) return EXIT_OPERATION_FAILED;

    mgr.restartService();
    return EXIT_SUCCESS_CODE;
}

int handleDnsRestart()
{
    DNSManager mgr;
    DnsResult initRes = mgr.initialize();
    if (!initRes.isSuccess()) return EXIT_OPERATION_FAILED;

    DnsResult result = mgr.restartService();
    return result.isSuccess() ? EXIT_SUCCESS_CODE : EXIT_OPERATION_FAILED;
}


int handlePasopt()
{
    // 一键优化：部署 + 添加常用规则 + 重启
    DNSManager mgr;
    DnsResult initRes = mgr.initialize();
    if (!initRes.isSuccess()) return EXIT_OPERATION_FAILED;

    // 1. 确保服务部署
    DnsServiceInfo info = mgr.getServiceInfo();
    if (!info.is_installed) {
        DnsResult installRes = mgr.installService();
        if (!installRes.isSuccess()) return EXIT_OPERATION_FAILED;
    }
    info = mgr.getServiceInfo();
    if (!info.is_running) {
        DnsResult startRes = mgr.startService();
        if (!startRes.isSuccess()) return EXIT_OPERATION_FAILED;
    }

    // 2. 添加常用 IPv6 屏蔽规则
    std::vector<std::pair<std::string, std::string>> rules = {
        {"127.0.0.1", "www.bilibili.com"},
        {"127.0.0.1", "bilibili.com"},
        {"127.0.0.1", "www.youku.com"},
        {"127.0.0.1", "youku.com"},
        {"127.0.0.1", "v.qq.com"},
        {"127.0.0.1", "iqiyi.com"},
        {"127.0.0.1", "douyin.com"},
    };

    for (const auto& [ip, host] : rules) {
        mgr.addRule(ip, host); // 忽略已存在的错误
    }

    // 3. 重启服务
    mgr.restartService();

    return EXIT_SUCCESS_CODE;
}
