#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QTextBrowser>
#include <QGroupBox>
#include <QTableWidget>

/**
 * @brief WiFi Portal 认证页面
 *
 * 使用 WifiPortalDll 进行校园 WiFi 上网认证。
 */
class WifiPortalPage : public QWidget
{
    Q_OBJECT

public:
    explicit WifiPortalPage(QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message, int timeoutMs = 3000);

private slots:
    void onLoginClicked();
    void onLogoutClicked();
    void onRefreshInfoClicked();
    void onKickDeviceClicked();

private:
    void setupUi();

    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QComboBox* m_providerCombo = nullptr;
    QPushButton* m_btnLogin = nullptr;
    QPushButton* m_btnLogout = nullptr;
    QPushButton* m_btnRefresh = nullptr;

    // 用户信息区
    QGroupBox* m_infoGroup = nullptr;
    QLabel* m_lblName = nullptr;
    QLabel* m_lblIp = nullptr;
    QLabel* m_lblTraffic = nullptr;
    QLabel* m_lblBalance = nullptr;

    // 设备列表
    QTableWidget* m_deviceTable = nullptr;
    QPushButton* m_btnKick = nullptr;

    QTextBrowser* m_logBrowser = nullptr;
};
