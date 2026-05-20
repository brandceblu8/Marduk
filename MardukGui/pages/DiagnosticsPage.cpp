#include "DiagnosticsPage.h"
#include "utils/AsyncWorker.h"
#include "NetworkDiagnostic.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

DiagnosticsPage::DiagnosticsPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void DiagnosticsPage::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QStringLiteral("网络诊断"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    auto* descLabel = new QLabel(
        QStringLiteral("全面检测网络连通性、DNS 解析、TCP 连接等状况"),
        this);
    descLabel->setStyleSheet("color: #aaa;");
    mainLayout->addWidget(descLabel);


    // ─── 操作按钮 ─────────────────────────────────────────────
    auto* btnLayout = new QHBoxLayout;
    m_btnRun = new QPushButton(QStringLiteral("▶️ 开始诊断"), this);
    m_btnRun->setMinimumHeight(36);
    m_btnRun->setStyleSheet(
        "QPushButton { background-color: #2a82da; border-radius: 4px; padding: 8px 24px; }"
        "QPushButton:hover { background-color: #3a92ea; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    btnLayout->addWidget(m_btnRun);

    m_reportFormat = new QComboBox(this);
    m_reportFormat->addItem(QStringLiteral("HTML 报告"));
    m_reportFormat->addItem(QStringLiteral("文本报告"));
    m_reportFormat->addItem(QStringLiteral("两者都生成"));
    btnLayout->addWidget(m_reportFormat);

    m_btnExport = new QPushButton(QStringLiteral("📁 导出报告"), this);
    m_btnExport->setEnabled(false);
    btnLayout->addWidget(m_btnExport);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(m_btnRun, &QPushButton::clicked, this, &DiagnosticsPage::onRunDiagnostic);

    // ─── 进度条 ───────────────────────────────────────────────
    m_lblStep = new QLabel(QStringLiteral("就绪"), this);
    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 4);
    m_progress->setValue(0);
    mainLayout->addWidget(m_lblStep);
    mainLayout->addWidget(m_progress);

    // ─── 结果区域 ─────────────────────────────────────────────
    m_resultBrowser = new QTextBrowser(this);
    m_resultBrowser->setPlaceholderText(QStringLiteral("诊断结果将在此显示..."));
    mainLayout->addWidget(m_resultBrowser, 1);
}


void DiagnosticsPage::onRunDiagnostic()
{
    m_btnRun->setEnabled(false);
    m_btnRun->setText(QStringLiteral("正在诊断..."));
    m_resultBrowser->clear();
    m_progress->setValue(0);
    emit statusMessage(QStringLiteral("正在运行网络诊断..."));

    // 后台线程执行诊断
    AsyncWorker::run<DiagnosticResult>(
        []() -> DiagnosticResult {
            NetworkDiagnostic diag;
            DiagnosticConfig config; // 使用默认配置
            return diag.runFullDiagnostic(config);
        },
        [this](const DiagnosticResult& result) {
            m_progress->setValue(4);
            QString summary;
            bool success = result.isSuccess();

            if (success) {
                int pingOk = 0, dnsOk = 0, tcpOk = 0;
                for (auto& p : result.ping_results) if (p.success) pingOk++;
                for (auto& d : result.dns_results) if (d.success) dnsOk++;
                for (auto& t : result.tcp_results) if (t.success) tcpOk++;

                summary = QStringLiteral(
                    "✅ 诊断完成\n"
                    "━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
                    "网络接口: %1 个\n"
                    "Ping 测试: %2/%3 成功\n"
                    "DNS 解析: %4/%5 成功\n"
                    "TCP 连接: %6/%7 成功\n"
                    "代理: %8"
                ).arg(result.network_interfaces.size())
                 .arg(pingOk).arg(result.ping_results.size())
                 .arg(dnsOk).arg(result.dns_results.size())
                 .arg(tcpOk).arg(result.tcp_results.size())
                 .arg(result.proxy_config.proxy_enabled
                      ? QStringLiteral("已启用") : QStringLiteral("未启用"));
            } else {
                summary = QStringLiteral("❌ 诊断失败: %1")
                    .arg(QString::fromStdString(result.error_message));
            }

            onDiagnosticFinished(success, summary);
        }
    );
}

void DiagnosticsPage::onDiagnosticFinished(bool success, const QString& summary)
{
    m_btnRun->setEnabled(true);
    m_btnRun->setText(QStringLiteral("▶️ 开始诊断"));
    m_resultBrowser->setText(summary);
    m_btnExport->setEnabled(success);
    m_lblStep->setText(success ? QStringLiteral("诊断完成") : QStringLiteral("诊断失败"));
    emit statusMessage(success ? QStringLiteral("诊断完成") : QStringLiteral("诊断失败"), 5000);
}
