#include "MainWindow.h"
#include "pages/LoginPage.h"
#include "pages/DnsManagerPage.h"
#include "pages/DiagnosticsPage.h"
#include "pages/WifiPortalPage.h"
#include "utils/ElevationHelper.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QIcon>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupNavigation();
    setupPages();

    // 在状态栏显示当前权限状态
    bool isAdmin = ElevationHelper::isRunningAsAdmin();
    m_elevationIndicator->setText(isAdmin
        ? QStringLiteral("🔐 管理员模式")
        : QStringLiteral("👤 普通用户模式（需要时自动提权）"));

    statusBar()->addPermanentWidget(m_elevationIndicator);
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Marduk — 西电校园网辅助工具"));
    resize(960, 640);
    setMinimumSize(800, 500);

    m_elevationIndicator = new QLabel(this);
}

void MainWindow::setupNavigation()
{
    // 左侧导航列表
    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(180);
    m_navList->setSpacing(4);
    m_navList->addItem(QStringLiteral("🌐 统一认证登录"));
    m_navList->addItem(QStringLiteral("📡 WiFi Portal"));
    m_navList->addItem(QStringLiteral("🔧 DNS 管理"));
    m_navList->addItem(QStringLiteral("🔍 网络诊断"));
    m_navList->setCurrentRow(0);

    connect(m_navList, &QListWidget::currentRowChanged,
            this, &MainWindow::onNavigationChanged);

    // 右侧页面栈
    m_pageStack = new QStackedWidget(this);

    // 布局：QSplitter 分割左右
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_navList);
    splitter->addWidget(m_pageStack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setHandleWidth(1);

    setCentralWidget(splitter);
}

void MainWindow::setupPages()
{
    m_loginPage = new LoginPage(this);
    m_wifiPage  = new WifiPortalPage(this);
    m_dnsPage   = new DnsManagerPage(this);
    m_diagPage  = new DiagnosticsPage(this);

    m_pageStack->addWidget(m_loginPage);
    m_pageStack->addWidget(m_wifiPage);
    m_pageStack->addWidget(m_dnsPage);
    m_pageStack->addWidget(m_diagPage);

    // 连接各页面的状态信号
    connect(m_loginPage, &LoginPage::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_dnsPage, &DnsManagerPage::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_diagPage, &DiagnosticsPage::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_wifiPage, &WifiPortalPage::statusMessage, this, &MainWindow::onStatusMessage);
}

void MainWindow::onNavigationChanged(int currentRow)
{
    if (currentRow >= 0 && currentRow < m_pageStack->count()) {
        m_pageStack->setCurrentIndex(currentRow);
    }
}

void MainWindow::onStatusMessage(const QString& message, int timeoutMs)
{
    statusBar()->showMessage(message, timeoutMs);
}
