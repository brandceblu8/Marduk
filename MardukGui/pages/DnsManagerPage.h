#pragma once

#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QTextBrowser>

/**
 * @brief DNS 管理页面
 *
 * 涉及 DNS 服务安装/启停、规则增删等操作需要管理员权限。
 * 通过 ElevationHelper 在需要时动态提权。
 */
class DnsManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit DnsManagerPage(QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message, int timeoutMs = 3000);

private slots:
    void onDeployClicked();
    void onAddRuleClicked();
    void onRemoveRuleClicked();
    void onClearRulesClicked();
    void onRefreshClicked();
    void onOneKeyOptimizeClicked();

private:
    void setupUi();
    void refreshRuleList();

    // 服务控制
    QPushButton* m_btnDeploy = nullptr;
    QPushButton* m_btnOneKey = nullptr;
    QLabel* m_lblServiceStatus = nullptr;

    // 规则管理
    QTableWidget* m_ruleTable = nullptr;
    QLineEdit* m_editIp = nullptr;
    QLineEdit* m_editHostname = nullptr;
    QPushButton* m_btnAdd = nullptr;
    QPushButton* m_btnRemove = nullptr;
    QPushButton* m_btnClear = nullptr;
    QPushButton* m_btnRefresh = nullptr;

    // 日志
    QTextBrowser* m_logBrowser = nullptr;
};
