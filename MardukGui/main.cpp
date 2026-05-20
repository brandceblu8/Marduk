/**
 * @file main.cpp
 * @brief MardukGui 入口点
 *
 * 以普通用户权限启动 GUI。需要管理员权限的操作通过
 * ElevationHelper 动态提权（启动一个提权子进程执行具体任务）。
 */

#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // ─── 基本应用信息 ─────────────────────────────────────────
    app.setApplicationName(QStringLiteral("Marduk"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setOrganizationName(QStringLiteral("Marduk"));

    // ─── Fusion 风格 + 暗色主题 ──────────────────────────────
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    // ─── 字体：优先使用微软雅黑 ──────────────────────────────
    QFont defaultFont(QStringLiteral("Microsoft YaHei UI"), 9);
    app.setFont(defaultFont);

    // ─── 主窗口 ──────────────────────────────────────────────
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
