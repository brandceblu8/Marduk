#include "DnsManagerPage.h"
#include "utils/ElevationHelper.h"
#include "utils/AsyncWorker.h"
#include "DNSManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>

DnsManagerPage::DnsManagerPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshRuleList();
}

void DnsManagerPage::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(12);

    // ─── 标题 ─────────────────────────────────────────────────
    auto* titleLabel = new QLabel(QStringLiteral("DNS 代理管理"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);


    // ─── 服务状态区 ───────────────────────────────────────────
    auto* serviceLayout = new QHBoxLayout;
    m_lblServiceStatus = new QLabel(QStringLiteral("服务状态: 未知"), this);
    m_btnDeploy = new QPushButton(QStringLiteral("🔧 部署/启动服务"), this);
    m_btnOneKey = new QPushButton(QStringLiteral("⚡ 一键优化"), this);
    serviceLayout->addWidget(m_lblServiceStatus);
    serviceLayout->addStretch();
    serviceLayout->addWidget(m_btnDeploy);
    serviceLayout->addWidget(m_btnOneKey);
    mainLayout->addLayout(serviceLayout);

    connect(m_btnDeploy, &QPushButton::clicked, this, &DnsManagerPage::onDeployClicked);
    connect(m_btnOneKey, &QPushButton::clicked, this, &DnsManagerPage::onOneKeyOptimizeClicked);

    // ─── 规则表格 ─────────────────────────────────────────────
    m_ruleTable = new QTableWidget(this);
    m_ruleTable->setColumnCount(2);
    m_ruleTable->setHorizontalHeaderLabels({QStringLiteral("IP 地址"), QStringLiteral("主机名")});
    m_ruleTable->horizontalHeader()->setStretchLastSection(true);
    m_ruleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ruleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_ruleTable, 1);


    // ─── 添加规则行 ───────────────────────────────────────────
    auto* addLayout = new QHBoxLayout;
    m_editIp = new QLineEdit(this);
    m_editIp->setPlaceholderText(QStringLiteral("IP 地址 (如 127.0.0.1)"));
    m_editHostname = new QLineEdit(this);
    m_editHostname->setPlaceholderText(QStringLiteral("主机名 (如 example.com)"));
    m_btnAdd = new QPushButton(QStringLiteral("➕ 添加"), this);
    m_btnRemove = new QPushButton(QStringLiteral("➖ 删除选中"), this);
    m_btnClear = new QPushButton(QStringLiteral("🗑️ 清空全部"), this);
    m_btnRefresh = new QPushButton(QStringLiteral("🔄 刷新"), this);
    addLayout->addWidget(m_editIp);
    addLayout->addWidget(m_editHostname);
    addLayout->addWidget(m_btnAdd);
    addLayout->addWidget(m_btnRemove);
    addLayout->addWidget(m_btnClear);
    addLayout->addWidget(m_btnRefresh);
    mainLayout->addLayout(addLayout);

    connect(m_btnAdd, &QPushButton::clicked, this, &DnsManagerPage::onAddRuleClicked);
    connect(m_btnRemove, &QPushButton::clicked, this, &DnsManagerPage::onRemoveRuleClicked);
    connect(m_btnClear, &QPushButton::clicked, this, &DnsManagerPage::onClearRulesClicked);
    connect(m_btnRefresh, &QPushButton::clicked, this, &DnsManagerPage::onRefreshClicked);

    // ─── 日志 ─────────────────────────────────────────────────
    m_logBrowser = new QTextBrowser(this);
    m_logBrowser->setMaximumHeight(120);
    m_logBrowser->setPlaceholderText(QStringLiteral("操作日志..."));
    mainLayout->addWidget(m_logBrowser);
}


void DnsManagerPage::refreshRuleList()
{
    // 列出规则不需要管理员权限
    DNSManager mgr;
    mgr.initialize();
    std::vector<std::pair<std::string, std::string>> rules;
    DnsResult result = mgr.listRules(rules);

    m_ruleTable->setRowCount(0);
    if (result.isSuccess()) {
        for (const auto& [ip, hostname] : rules) {
            int row = m_ruleTable->rowCount();
            m_ruleTable->insertRow(row);
            m_ruleTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(ip)));
            m_ruleTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(hostname)));
        }
    }

    // 更新服务状态
    DnsServiceInfo info = mgr.getServiceInfo();
    QString statusText = QStringLiteral("服务状态: %1 | %2 | 规则: %3 条")
        .arg(info.is_installed ? QStringLiteral("已安装") : QStringLiteral("未安装"))
        .arg(info.is_running ? QStringLiteral("运行中 ✅") : QStringLiteral("已停止 ❌"))
        .arg(info.rule_count);
    m_lblServiceStatus->setText(statusText);
}


void DnsManagerPage::onDeployClicked()
{
    m_logBrowser->append(QStringLiteral("[*] 正在部署 DNS 服务（需要管理员权限）..."));
    emit statusMessage(QStringLiteral("正在部署 DNS 服务..."));

    // 动态提权：通过提权子进程执行
    bool ok = ElevationHelper::runElevated(
        QStringLiteral("--elevated-dns-deploy"),
        this
    );

    if (ok) {
        m_logBrowser->append(QStringLiteral("[✓] DNS 服务部署成功"));
    } else {
        m_logBrowser->append(QStringLiteral("[✗] DNS 服务部署失败或已取消"));
    }
    refreshRuleList();
}

void DnsManagerPage::onAddRuleClicked()
{
    QString ip = m_editIp->text().trimmed();
    QString hostname = m_editHostname->text().trimmed();

    if (ip.isEmpty() || hostname.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("IP 和主机名不能为空"));
        return;
    }

    QString args = QStringLiteral("--elevated-dns-add %1 %2").arg(ip, hostname);
    bool ok = ElevationHelper::runElevated(args, this);

    if (ok) {
        m_logBrowser->append(QStringLiteral("[✓] 规则已添加: %1 -> %2").arg(ip, hostname));
        m_editIp->clear();
        m_editHostname->clear();
    } else {
        m_logBrowser->append(QStringLiteral("[✗] 添加规则失败或已取消"));
    }
    refreshRuleList();
}


void DnsManagerPage::onRemoveRuleClicked()
{
    int row = m_ruleTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的规则"));
        return;
    }

    QString hostname = m_ruleTable->item(row, 1)->text();
    QString args = QStringLiteral("--elevated-dns-remove %1").arg(hostname);
    bool ok = ElevationHelper::runElevated(args, this);

    if (ok) {
        m_logBrowser->append(QStringLiteral("[✓] 规则已删除: %1").arg(hostname));
    } else {
        m_logBrowser->append(QStringLiteral("[✗] 删除规则失败或已取消"));
    }
    refreshRuleList();
}

void DnsManagerPage::onClearRulesClicked()
{
    auto btn = QMessageBox::warning(this,
        QStringLiteral("确认清空"),
        QStringLiteral("确定要清空所有 DNS 规则吗？此操作无法撤销！"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (btn != QMessageBox::Yes) return;

    bool ok = ElevationHelper::runElevated(QStringLiteral("--elevated-dns-clear"), this);
    if (ok) {
        m_logBrowser->append(QStringLiteral("[✓] 所有规则已清空"));
    } else {
        m_logBrowser->append(QStringLiteral("[✗] 清空规则失败或已取消"));
    }
    refreshRuleList();
}

void DnsManagerPage::onRefreshClicked()
{
    refreshRuleList();
    m_logBrowser->append(QStringLiteral("[*] 规则列表已刷新"));
}

void DnsManagerPage::onOneKeyOptimizeClicked()
{
    m_logBrowser->append(QStringLiteral("[*] 正在执行一键网络优化..."));
    bool ok = ElevationHelper::runElevated(QStringLiteral("--elevated-pasopt"), this);
    if (ok) {
        m_logBrowser->append(QStringLiteral("[✓] 一键优化完成"));
    } else {
        m_logBrowser->append(QStringLiteral("[✗] 一键优化失败或已取消"));
    }
    refreshRuleList();
}
