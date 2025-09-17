#pragma once

#include "NetworkDiagnostic.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

// HTML模板管理器
class HtmlTemplateManager {
public:
    using TemplateVariables = std::map<std::string, std::string>;
    using TemplateFunction = std::function<std::string(const TemplateVariables&)>;

    HtmlTemplateManager();
    ~HtmlTemplateManager();

    bool loadTemplate(const std::string& template_name, const std::string& template_content);
    bool loadTemplateFromFile(const std::string& template_name, const std::string& file_path);

    std::string render(const std::string& template_name, const TemplateVariables& variables = {});

    void registerFunction(const std::string& function_name, TemplateFunction func);

    void setGlobalVariable(const std::string& key, const std::string& value);

    // 渲染循环（用于表格等重复内容）
    std::string renderLoop(const std::string& template_content,
        const std::vector<TemplateVariables>& items);

private:
    class HtmlTemplateManagerImpl;
    std::unique_ptr<HtmlTemplateManagerImpl> impl;
};

// 诊断报告专用的模板数据结构
struct DiagnosticTemplateData {
    // 基本信息
    std::string timestamp;
    std::string version;
    std::string system_info;

    // 统计数据
    struct {
        int active_interfaces = 0;
        int total_interfaces = 0;
        int successful_pings = 0;
        int total_pings = 0;
        int successful_dns = 0;
        int total_dns = 0;
        int successful_tcp = 0;
        int total_tcp = 0;
    } summary;

    // 表格数据
    std::vector<HtmlTemplateManager::TemplateVariables> network_interfaces;
    std::vector<HtmlTemplateManager::TemplateVariables> ping_results;
    std::vector<HtmlTemplateManager::TemplateVariables> dns_results;
    std::vector<HtmlTemplateManager::TemplateVariables> tcp_results;
    std::vector<HtmlTemplateManager::TemplateVariables> routing_table;

    // 代理配置
    HtmlTemplateManager::TemplateVariables proxy_config;

    // 诊断建议
    std::vector<std::string> suggestions;
    bool has_issues = false;
};

// 诊断报告生成器
class DiagnosticReportGenerator {
public:
    DiagnosticReportGenerator();
    ~DiagnosticReportGenerator();

    // 生成HTML报告
    std::string generateHtmlReport(const DiagnosticResult& diagnostic_result);

    // 生成文本报告
    std::string generateTextReport(const DiagnosticResult& diagnostic_result);

    // 设置自定义模板
    void setCustomTemplate(const std::string& template_content);

    // 添加PPPoE诊断建议
    void addPppoeErrorSuggestion(int error_code, const std::string& suggestion);

private:
    DiagnosticTemplateData convertToTemplateData(const DiagnosticResult& diagnostic_result);
    std::vector<std::string> generateSuggestions(const DiagnosticResult& diagnostic_result);

    std::unique_ptr<HtmlTemplateManager> template_manager;
    std::map<int, std::string> pppoe_error_suggestions;
};