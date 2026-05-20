#pragma once

#include <QWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTextBrowser>
#include <QLabel>
#include <QComboBox>

/**
 * @brief 网络诊断页面
 *
 * 运行 NetworkDiagnosticDll 的完整诊断流程。
 * 诊断本身不需要管理员权限（仅读取信息）。
 */
class DiagnosticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticsPage(QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message, int timeoutMs = 3000);

private slots:
    void onRunDiagnostic();
    void onDiagnosticFinished(bool success, const QString& summary);

private:
    void setupUi();

    QPushButton* m_btnRun = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_lblStep = nullptr;
    QTextBrowser* m_resultBrowser = nullptr;
    QComboBox* m_reportFormat = nullptr;
    QPushButton* m_btnExport = nullptr;
};
