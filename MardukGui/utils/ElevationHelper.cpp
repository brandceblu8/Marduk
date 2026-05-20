#include "ElevationHelper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <Windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

bool ElevationHelper::isRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &adminGroup))
    {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}


QString ElevationHelper::getElevatedHelperPath()
{
    // 提权子进程与主 GUI 在同一目录
    QString appDir = QCoreApplication::applicationDirPath();
    return QDir(appDir).filePath(QStringLiteral("MardukElevated.exe"));
}

bool ElevationHelper::runElevated(const QString& arguments, QWidget* parent)
{
    DWORD exitCode = 0;
    bool launched = runElevatedWithExitCode(arguments, exitCode, parent);
    return launched && (exitCode == 0);
}

bool ElevationHelper::runElevatedWithExitCode(const QString& arguments,
                                              DWORD& exitCode,
                                              QWidget* parent)
{
    // 如果已经是管理员，直接在当前进程上下文执行（无需 UAC 弹窗）
    // 注意：这里仍然启动子进程以保持一致的代码路径
    QString helperPath = getElevatedHelperPath();

    if (!QFileInfo::exists(helperPath)) {
        // Fallback: 如果子进程不存在，尝试用当前 exe 加 --elevated 前缀
        helperPath = QCoreApplication::applicationFilePath();
    }

    // 转换为 wchar_t
    std::wstring wPath = helperPath.toStdWString();
    std::wstring wArgs = arguments.toStdWString();

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
    sei.lpVerb = L"runas";                   // 触发 UAC 提权
    sei.lpFile = wPath.c_str();
    sei.lpParameters = wArgs.c_str();
    sei.nShow = SW_HIDE;                     // 子进程无窗口

    // 关联父窗口句柄（使 UAC 弹窗模态于我们的 GUI）
    if (parent) {
        sei.hwnd = reinterpret_cast<HWND>(parent->winId());
    }

    if (!ShellExecuteExW(&sei)) {
        DWORD error = GetLastError();
        // ERROR_CANCELLED = 1223 表示用户取消了 UAC
        exitCode = error;
        return false;
    }

    // 等待子进程完成（带超时，防止无限挂起）
    if (sei.hProcess) {
        // 最多等 60 秒
        DWORD waitResult = WaitForSingleObject(sei.hProcess, 60000);
        if (waitResult == WAIT_OBJECT_0) {
            GetExitCodeProcess(sei.hProcess, &exitCode);
        } else {
            // 超时
            TerminateProcess(sei.hProcess, 1);
            exitCode = 1;
        }
        CloseHandle(sei.hProcess);
    }

    return true;
}
