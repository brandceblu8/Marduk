#pragma once

#include <string>

namespace DiagnosticHTMLTemplates {
    const std::string DIAGNOSTIC_REPORT_TEMPLATE = u8R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>网络诊断报告</title>
    <style>
        body { font-family: 'Microsoft YaHei', sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }
        h2 { color: #34495e; margin-top: 30px; }
        .info-box { background-color: #ecf0f1; padding: 15px; border-left: 4px solid #3498db; margin: 10px 0; }
        .success { color: #27ae60; font-weight: bold; }
        .error { color: #e74c3c; font-weight: bold; }
        .warning { color: #f39c12; font-weight: bold; }
        table { width: 100%; border-collapse: collapse; margin: 15px 0; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #3498db; color: white; }
        tr:nth-child(even) { background-color: #f2f2f2; }
        .metric { display: inline-block; margin: 10px; padding: 10px; background-color: #ecf0f1; border-radius: 5px; }
        .timestamp { color: #7f8c8d; font-size: 0.9em; }
        .summary-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0; }
        .summary-card { background: white; border: 1px solid #e1e5e9; border-radius: 8px; padding: 20px; text-align: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .summary-number { font-size: 2em; font-weight: bold; color: #3498db; }
        .summary-label { color: #6c757d; font-weight: 500; margin-top: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔍 网络诊断报告</h1>
        <div class="info-box">
            <strong>生成时间:</strong> <span class="timestamp">{{timestamp}}</span><br>
            <strong>报告版本:</strong> <span class="timestamp">{{version}}</span>
        </div>

        <!-- 诊断摘要 -->
        <h2>📊 诊断摘要</h2>
        <div class="summary-grid">
            <div class="summary-card">
                <div class="summary-number">{{summary_active_interfaces}}</div>
                <div class="summary-label">活跃网络接口</div>
            </div>
            <div class="summary-card">
                <div class="summary-number">{{summary_successful_pings}}/{{summary_total_pings}}</div>
                <div class="summary-label">连通性测试</div>
            </div>
            <div class="summary-card">
                <div class="summary-number">{{summary_successful_dns}}/{{summary_total_dns}}</div>
                <div class="summary-label">DNS解析</div>
            </div>
            <div class="summary-card">
                <div class="summary-number">{{summary_successful_tcp}}/{{summary_total_tcp}}</div>
                <div class="summary-label">TCP连接</div>
            </div>
        </div>

        <!-- 系统信息 -->
        <h2>💻 系统信息</h2>
        <div class="info-box">
            <pre>{{system_info}}</pre>
        </div>

        <!-- 网络接口 -->
        <h2>🌐 网络接口</h2>
        <table>
            <thead>
                <tr>
                    <th>接口名称</th>
                    <th>描述</th>
                    <th>MAC地址</th>
                    <th>IP地址</th>
                    <th>子网掩码</th>
                    <th>网关</th>
                    <th>类型</th>
                    <th>状态</th>
                </tr>
            </thead>
            <tbody>
                {{network_interfaces_rows}}
            </tbody>
        </table>

        <!-- 代理配置 -->
        <h2>🔄 代理配置</h2>
        <div class="info-box">
            {{proxy_config_content}}
        </div>

        <!-- Ping测试结果 -->
        <h2>📡 连通性测试结果</h2>
        <table>
            <thead>
                <tr>
                    <th>目标地址</th>
                    <th>状态</th>
                    <th>丢包率</th>
                    <th>最小延迟</th>
                    <th>最大延迟</th>
                    <th>平均延迟</th>
                    <th>错误信息</th>
                </tr>
            </thead>
            <tbody>
                {{ping_results_rows}}
            </tbody>
        </table>

        <!-- DNS测试结果 -->
        <h2>🌍 DNS解析测试结果</h2>
        <table>
            <thead>
                <tr>
                    <th>域名</th>
                    <th>状态</th>
                    <th>查询时间</th>
                    <th>解析IP地址</th>
                    <th>DNS服务器</th>
                    <th>错误信息</th>
                </tr>
            </thead>
            <tbody>
                {{dns_results_rows}}
            </tbody>
        </table>

        <!-- TCP连接测试结果 -->
        <h2>🔌 TCP连接测试结果</h2>
        <table>
            <thead>
                <tr>
                    <th>目标主机</th>
                    <th>端口</th>
                    <th>状态</th>
                    <th>连接时间</th>
                    <th>错误信息</th>
                </tr>
            </thead>
            <tbody>
                {{tcp_results_rows}}
            </tbody>
        </table>

        <!-- 路由表 -->
        {{routing_table_section}}

        <!-- 诊断建议 -->
        <div style="margin-top: 30px; padding: 20px; background-color: #eafaf1; border-left: 5px solid #27ae60; border-radius: 5px;">
            <h3>✅ 诊断建议</h3>
            {{diagnostic_suggestions}}
        </div>

        <!-- 页脚 -->
        <div style="margin-top: 40px; padding: 20px; border-top: 2px solid #ecf0f1; text-align: center; color: #7f8c8d;">
            <p>📊 西电校园网辅助工具 - 网络诊断报告</p>
            <p>🕐 生成时间: {{timestamp}} | 🔧 诊断引擎: NetworkDiagnostic v1.0</p>
            <p>💡 如遇网络问题，请根据测试结果检查相应配置</p>
        </div>
    </div>
</body>
</html>)";

    // 表格行模板
    const std::string NETWORK_INTERFACE_ROW_TEMPLATE = u8R"(
                <tr>
                    <td>{{name}}</td>
                    <td>{{description}}</td>
                    <td>{{mac_address}}</td>
                    <td>{{ip_address}}</td>
                    <td>{{subnet_mask}}</td>
                    <td>{{gateway}}</td>
                    <td>{{connection_type}}</td>
                    <td>{{status_html}}</td>
                </tr>)";

    const std::string PING_RESULT_ROW_TEMPLATE = u8R"(
                <tr>
                    <td><code>{{target}}</code></td>
                    <td>{{status_html}}</td>
                    <td>{{packet_loss}}</td>
                    <td>{{min_time}}</td>
                    <td>{{max_time}}</td>
                    <td>{{avg_time}}</td>
                    <td>{{error_message}}</td>
                </tr>)";

    const std::string DNS_RESULT_ROW_TEMPLATE = u8R"(
                <tr>
                    <td><strong>{{hostname}}</strong></td>
                    <td>{{status_html}}</td>
                    <td>{{query_time}}ms</td>
                    <td>{{ip_addresses_html}}</td>
                    <td>{{dns_server}}</td>
                    <td>{{error_message}}</td>
                </tr>)";

    const std::string TCP_RESULT_ROW_TEMPLATE = u8R"(
                <tr>
                    <td><strong>{{target_host}}</strong></td>
                    <td>{{target_port}}</td>
                    <td>{{status_html}}</td>
                    <td>{{connection_time}}ms</td>
                    <td>{{error_message}}</td>
                </tr>)";

    const std::string ROUTING_TABLE_SECTION_TEMPLATE = u8R"(
        <h2>🛣️ 路由表信息</h2>
        <table>
            <thead>
                <tr>
                    <th>目标网络</th>
                    <th>子网掩码</th>
                    <th>网关地址</th>
                    <th>网络接口</th>
                    <th>路由跃点</th>
                </tr>
            </thead>
            <tbody>
                {{routing_table_rows}}
            </tbody>
        </table>)";

    const std::string ROUTING_TABLE_ROW_TEMPLATE = u8R"(
                <tr>
                    <td><code>{{destination}}</code></td>
                    <td><code>{{netmask}}</code></td>
                    <td><code>{{gateway}}</code></td>
                    <td>{{route_interface}}</td>
                    <td>{{metric}}</td>
                </tr>)";
}