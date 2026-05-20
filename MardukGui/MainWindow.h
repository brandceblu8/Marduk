#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QStatusBar>

// Forward declarations
class LoginPage;
class DnsManagerPage;
class DiagnosticsPage;
class WifiPortalPage;

/**
 * @brief 主窗口 — 左侧导航 + 右侧多页面切换
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onNavigationChanged(int currentRow);
    void onStatusMessage(const QString& message, int timeoutMs = 3000);

private:
    void setupUi();
    void setupNavigation();
    void setupPages();

    // 导航
    QListWidget* m_navList = nullptr;
    QStackedWidget* m_pageStack = nullptr;

    // 页面
    LoginPage* m_loginPage = nullptr;
    DnsManagerPage* m_dnsPage = nullptr;
    DiagnosticsPage* m_diagPage = nullptr;
    WifiPortalPage* m_wifiPage = nullptr;

    // 底部状态
    QLabel* m_elevationIndicator = nullptr;
};
