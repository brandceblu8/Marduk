#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextBrowser>
#include <QGroupBox>

/**
 * @brief 统一认证平台 (zfw.xidian.edu.cn) 登录页面
 *
 * 使用 ZfwInteractionDll 进行登录（含 ONNX 验证码识别）。
 * 登录操作在后台线程执行，避免阻塞 UI。
 */
class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message, int timeoutMs = 3000);

private slots:
    void onLoginClicked();
    void onLoginFinished(bool success, const QString& message);

private:
    void setupUi();

    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QTextBrowser* m_resultBrowser = nullptr;
    QGroupBox* m_userInfoGroup = nullptr;

    // 用户信息标签
    QLabel* m_lblRealname = nullptr;
    QLabel* m_lblStatus = nullptr;
    QLabel* m_lblWallet = nullptr;
    QLabel* m_lblPlans = nullptr;
};
