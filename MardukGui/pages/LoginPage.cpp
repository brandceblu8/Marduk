#include "LoginPage.h"
#include "utils/AsyncWorker.h"
#include "ZfwManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QtConcurrent>

LoginPage::LoginPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void LoginPage::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // ─── 标题 ─────────────────────────────────────────────────
    auto* titleLabel = new QLabel(QStringLiteral("统一认证平台登录"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(QStringLiteral("登录 zfw.xidian.edu.cn 管理校园网账户"), this);
    subtitleLabel->setStyleSheet("color: #aaa;");
    mainLayout->addWidget(subtitleLabel);

    // ─── 登录表单 ─────────────────────────────────────────────
    auto* formLayout = new QFormLayout;
    formLayout->setSpacing(12);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入学号"));
    m_usernameEdit->setMinimumHeight(32);
    formLayout->addRow(QStringLiteral("学号:"), m_usernameEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumHeight(32);
    formLayout->addRow(QStringLiteral("密码:"), m_passwordEdit);

    mainLayout->addLayout(formLayout);

    // ─── 登录按钮 ─────────────────────────────────────────────
    m_loginBtn = new QPushButton(QStringLiteral("🔑 登录"), this);
    m_loginBtn->setMinimumHeight(36);
    m_loginBtn->setStyleSheet(
        "QPushButton { background-color: #2a82da; border-radius: 4px; padding: 8px 24px; }"
        "QPushButton:hover { background-color: #3a92ea; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginPage::onLoginClicked);
    mainLayout->addWidget(m_loginBtn);

    // ─── 用户信息面板（登录成功后显示）─────────────────────────
    m_userInfoGroup = new QGroupBox(QStringLiteral("用户信息"), this);
    m_userInfoGroup->setVisible(false);
    auto* infoLayout = new QFormLayout(m_userInfoGroup);
    m_lblRealname = new QLabel(this);
    m_lblStatus   = new QLabel(this);
    m_lblWallet   = new QLabel(this);
    m_lblPlans    = new QLabel(this);
    infoLayout->addRow(QStringLiteral("姓名:"), m_lblRealname);
    infoLayout->addRow(QStringLiteral("状态:"), m_lblStatus);
    infoLayout->addRow(QStringLiteral("余额:"), m_lblWallet);
    infoLayout->addRow(QStringLiteral("套餐:"), m_lblPlans);
    mainLayout->addWidget(m_userInfoGroup);

    // ─── 日志区域 ─────────────────────────────────────────────
    m_resultBrowser = new QTextBrowser(this);
    m_resultBrowser->setMaximumHeight(150);
    m_resultBrowser->setPlaceholderText(QStringLiteral("登录日志将在此显示..."));
    mainLayout->addWidget(m_resultBrowser);

    mainLayout->addStretch();
}

void LoginPage::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("学号和密码不能为空"));
        return;
    }

    m_loginBtn->setEnabled(false);
    m_loginBtn->setText(QStringLiteral("正在登录..."));
    m_resultBrowser->append(QStringLiteral("[*] 正在连接统一认证平台..."));
    emit statusMessage(QStringLiteral("正在登录..."));

    // ─── 后台线程执行登录（避免阻塞 UI）─────────────────────
    std::string u8user = username.toStdString();
    std::string u8pass = password.toStdString();

    AsyncWorker::run<LoginResult>(
        [u8user, u8pass]() -> LoginResult {
            // 获取模型路径（与 exe 同目录）
            QString modelPath = QCoreApplication::applicationDirPath() + "/crnn_model.onnx";
            ZfwManager mgr(modelPath.toStdWString());
            return mgr.login(u8user, u8pass);
        },
        [this](const LoginResult& result) {
            bool success = result.isSuccess();
            QString msg = QString::fromStdString(result.error_message);
            onLoginFinished(success, msg);

            if (success) {
                // 填充用户信息
                m_lblRealname->setText(QString::fromStdString(result.user_info.realname));
                m_lblStatus->setText(QString::fromStdString(result.user_info.user_status));
                m_lblWallet->setText(QString::number(result.user_info.wallet, 'f', 2) + QStringLiteral(" 元"));

                QStringList plans;
                if (result.user_info.telecom_plan) plans << QStringLiteral("电信");
                if (result.user_info.unicom_plan) plans << QStringLiteral("联通");
                if (result.user_info.mobile_plan) plans << QStringLiteral("移动");
                m_lblPlans->setText(plans.isEmpty() ? QStringLiteral("无") : plans.join(", "));

                m_userInfoGroup->setVisible(true);
            }
        }
    );
}

void LoginPage::onLoginFinished(bool success, const QString& message)
{
    m_loginBtn->setEnabled(true);
    m_loginBtn->setText(QStringLiteral("🔑 登录"));

    if (success) {
        m_resultBrowser->append(QStringLiteral("[✓] 登录成功: ") + message);
        emit statusMessage(QStringLiteral("登录成功"));
    } else {
        m_resultBrowser->append(QStringLiteral("[✗] 登录失败: ") + message);
        emit statusMessage(QStringLiteral("登录失败: ") + message, 5000);
    }
}
