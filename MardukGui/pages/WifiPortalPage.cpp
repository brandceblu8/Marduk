#include "WifiPortalPage.h"
#include "utils/AsyncWorker.h"
#include "WifiPortal.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>

WifiPortalPage::WifiPortalPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void WifiPortalPage::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QStringLiteral("WiFi Portal 认证"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    auto* descLabel = new QLabel(
        QStringLiteral("校园 WiFi 上网认证、查看流量、管理在线设备"),
        this);
    descLabel->setStyleSheet("color: #aaa;");
    mainLayout->addWidget(descLabel);


    // ─── 登录表单 ─────────────────────────────────────────────
    auto* formLayout = new QFormLayout;
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QStringLiteral("学号"));
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItem(QStringLiteral("校园网"), static_cast<int>(NetworkProvider::CAMPUS));
    m_providerCombo->addItem(QStringLiteral("中国电信"), static_cast<int>(NetworkProvider::TELECOM));
    m_providerCombo->addItem(QStringLiteral("中国联通"), static_cast<int>(NetworkProvider::UNICOM));
    m_providerCombo->addItem(QStringLiteral("中国移动"), static_cast<int>(NetworkProvider::MOBILE));

    formLayout->addRow(QStringLiteral("学号:"), m_usernameEdit);
    formLayout->addRow(QStringLiteral("密码:"), m_passwordEdit);
    formLayout->addRow(QStringLiteral("运营商:"), m_providerCombo);
    mainLayout->addLayout(formLayout);

    // ─── 操作按钮 ─────────────────────────────────────────────
    auto* btnLayout = new QHBoxLayout;
    m_btnLogin = new QPushButton(QStringLiteral("🔑 认证上网"), this);
    m_btnLogin->setMinimumHeight(36);
    m_btnLogout = new QPushButton(QStringLiteral("🚪 注销"), this);
    m_btnRefresh = new QPushButton(QStringLiteral("🔄 刷新状态"), this);
    btnLayout->addWidget(m_btnLogin);
    btnLayout->addWidget(m_btnLogout);
    btnLayout->addWidget(m_btnRefresh);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(m_btnLogin, &QPushButton::clicked, this, &WifiPortalPage::onLoginClicked);
    connect(m_btnLogout, &QPushButton::clicked, this, &WifiPortalPage::onLogoutClicked);
    connect(m_btnRefresh, &QPushButton::clicked, this, &WifiPortalPage::onRefreshInfoClicked);


    // ─── 用户信息 ─────────────────────────────────────────────
    m_infoGroup = new QGroupBox(QStringLiteral("连接信息"), this);
    m_infoGroup->setVisible(false);
    auto* infoLayout = new QFormLayout(m_infoGroup);
    m_lblName = new QLabel(this);
    m_lblIp = new QLabel(this);
    m_lblTraffic = new QLabel(this);
    m_lblBalance = new QLabel(this);
    infoLayout->addRow(QStringLiteral("用户:"), m_lblName);
    infoLayout->addRow(QStringLiteral("IP:"), m_lblIp);
    infoLayout->addRow(QStringLiteral("流量:"), m_lblTraffic);
    infoLayout->addRow(QStringLiteral("余额:"), m_lblBalance);
    mainLayout->addWidget(m_infoGroup);

    // ─── 设备列表 ─────────────────────────────────────────────
    m_deviceTable = new QTableWidget(this);
    m_deviceTable->setColumnCount(3);
    m_deviceTable->setHorizontalHeaderLabels({
        QStringLiteral("IP"), QStringLiteral("OS"), QStringLiteral("类型")});
    m_deviceTable->horizontalHeader()->setStretchLastSection(true);
    m_deviceTable->setMaximumHeight(150);
    m_btnKick = new QPushButton(QStringLiteral("踢下线"), this);
    connect(m_btnKick, &QPushButton::clicked, this, &WifiPortalPage::onKickDeviceClicked);
    mainLayout->addWidget(m_deviceTable);
    mainLayout->addWidget(m_btnKick);

    // ─── 日志 ─────────────────────────────────────────────────
    m_logBrowser = new QTextBrowser(this);
    m_logBrowser->setMaximumHeight(100);
    mainLayout->addWidget(m_logBrowser);
}


void WifiPortalPage::onLoginClicked()
{
    QString user = m_usernameEdit->text().trimmed();
    QString pass = m_passwordEdit->text().trimmed();
    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("学号和密码不能为空"));
        return;
    }

    int providerIdx = m_providerCombo->currentData().toInt();
    auto provider = static_cast<NetworkProvider>(providerIdx);

    m_btnLogin->setEnabled(false);
    m_logBrowser->append(QStringLiteral("[*] 正在认证上网..."));

    std::string u8user = user.toStdString();
    std::string u8pass = pass.toStdString();

    AsyncWorker::run<PortalResult>(
        [u8user, u8pass, provider]() -> PortalResult {
            WifiPortal portal;
            auto loginRes = portal.login(u8user, u8pass, provider);
            // 封装为 PortalResult 方便统一处理
            PortalResult pr;
            pr.error_code = loginRes.error_code;
            pr.error_message = loginRes.error_message;
            if (loginRes.isSuccess()) {
                // 登录成功后获取用户信息
                pr = portal.getUserInfo();
            }
            return pr;
        },
        [this](const PortalResult& result) {
            m_btnLogin->setEnabled(true);
            if (result.isSuccess()) {
                m_logBrowser->append(QStringLiteral("[✓] 认证成功"));
                m_infoGroup->setVisible(true);
                m_lblName->setText(QString::fromStdString(result.user_info.real_name));
                m_lblIp->setText(QString::fromStdString(result.user_info.online_ip));
                m_lblTraffic->setText(QString::fromStdString(
                    formatBytes(result.user_info.all_bytes)));
                m_lblBalance->setText(QString::number(result.user_info.user_balance, 'f', 2)
                    + QStringLiteral(" 元"));
                emit statusMessage(QStringLiteral("WiFi 认证成功"));
            } else {
                m_logBrowser->append(QStringLiteral("[✗] 认证失败: ")
                    + QString::fromStdString(result.error_message));
                emit statusMessage(QStringLiteral("WiFi 认证失败"), 5000);
            }
        }
    );
}

void WifiPortalPage::onLogoutClicked()
{
    m_logBrowser->append(QStringLiteral("[*] 正在注销..."));
    AsyncWorker::run<PortalResult>(
        []() -> PortalResult {
            WifiPortal portal;
            return portal.logout();
        },
        [this](const PortalResult& result) {
            if (result.isSuccess()) {
                m_logBrowser->append(QStringLiteral("[✓] 已注销"));
                m_infoGroup->setVisible(false);
            } else {
                m_logBrowser->append(QStringLiteral("[✗] 注销失败: ")
                    + QString::fromStdString(result.error_message));
            }
        }
    );
}

void WifiPortalPage::onRefreshInfoClicked()
{
    m_logBrowser->append(QStringLiteral("[*] 刷新用户信息..."));
    // TODO: 实现刷新逻辑
}

void WifiPortalPage::onKickDeviceClicked()
{
    int row = m_deviceTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择设备"));
        return;
    }
    // TODO: 调用 WifiPortal::kickDevice
    m_logBrowser->append(QStringLiteral("[*] 正在踢下线..."));
}
