#include "pch.h"
#include "HtmlTemplateManager.h"
#include <regex>
#include <fstream>
#include <sstream>

class HtmlTemplateManager::HtmlTemplateManagerImpl {
private:
    std::map<std::string, std::string> templates;
    std::map<std::string, TemplateFunction> functions;
    TemplateVariables global_variables;

public:
    bool loadTemplate(const std::string& template_name, const std::string& template_content) {
        templates[template_name] = template_content;
        return true;
    }

    bool loadTemplateFromFile(const std::string& template_name, const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        templates[template_name] = buffer.str();
        return true;
    }

    std::string render(const std::string& template_name, const TemplateVariables& variables) {
        auto it = templates.find(template_name);
        if (it == templates.end()) {
            return "Template not found: " + template_name;
        }

        return processTemplate(it->second, variables);
    }

    void registerFunction(const std::string& function_name, TemplateFunction func) {
        functions[function_name] = func;
    }

    void setGlobalVariable(const std::string& key, const std::string& value) {
        global_variables[key] = value;
    }

    std::string renderLoop(const std::string& template_content,
        const std::vector<TemplateVariables>& items) {
        std::string result;
        for (const auto& item : items) {
            result += processTemplate(template_content, item);
        }
        return result;
    }

private:
    std::string processTemplate(const std::string& template_content, const TemplateVariables& variables) {
        std::string result = template_content;

        TemplateVariables all_variables = global_variables;
        all_variables.insert(variables.begin(), variables.end());

        std::regex var_regex(R"(\{\{([^}]+)\}\})");
        std::smatch match;

        while (std::regex_search(result, match, var_regex)) {
            std::string var_name = match[1].str();
            std::string replacement = "";

            auto var_it = all_variables.find(var_name);
            if (var_it != all_variables.end()) {
                replacement = var_it->second;
            }

            result.replace(match.position(), match.length(), replacement);
        }

        result = processConditionals(result, all_variables);

        result = processFunctions(result, all_variables);

        return result;
    }

    std::string processConditionals(const std::string& content, const TemplateVariables& variables) {
        std::string result = content;

        std::regex if_regex(R"(\{\{#if\s+([^}]+)\}\}(.*?)\{\{/if\}\})");
        std::smatch match;

        while (std::regex_search(result, match, if_regex)) {
            std::string condition = match[1].str();
            std::string if_content = match[2].str();
            std::string replacement = "";

            auto var_it = variables.find(condition);
            if (var_it != variables.end() && !var_it->second.empty() && var_it->second != "0" && var_it->second != "false") {
                replacement = if_content;
            }

            result.replace(match.position(), match.length(), replacement);
        }

        // 处理 {{#unless variable}}...{{/unless}}
        std::regex unless_regex(R"(\{\{#unless\s+([^}]+)\}\}(.*?)\{\{/unless\}\})");
        while (std::regex_search(result, match, unless_regex)) {
            std::string condition = match[1].str();
            std::string unless_content = match[2].str();
            std::string replacement = "";

            // 检查反向条件
            auto var_it = variables.find(condition);
            if (var_it == variables.end() || var_it->second.empty() || var_it->second == "0" || var_it->second == "false") {
                replacement = unless_content;
            }

            result.replace(match.position(), match.length(), replacement);
        }

        return result;
    }

    std::string processFunctions(const std::string& content, const TemplateVariables& variables) {
        std::string result = content;

        // 处理函数调用 {{function_name(param)}}
        std::regex func_regex(R"(\{\{([a-zA-Z_][a-zA-Z0-9_]*)\(([^)]*)\)\}\})");
        std::smatch match;

        while (std::regex_search(result, match, func_regex)) {
            std::string func_name = match[1].str();
            std::string param = match[2].str();
            std::string replacement = "";

            auto func_it = functions.find(func_name);
            if (func_it != functions.end()) {
                TemplateVariables func_vars = variables;
                func_vars["param"] = param;
                replacement = func_it->second(func_vars);
            }

            result.replace(match.position(), match.length(), replacement);
        }

        return result;
    }
};

// HtmlTemplateManager 实现
HtmlTemplateManager::HtmlTemplateManager() : impl(std::make_unique<HtmlTemplateManagerImpl>()) {}
HtmlTemplateManager::~HtmlTemplateManager() = default;

bool HtmlTemplateManager::loadTemplate(const std::string& template_name, const std::string& template_content) {
    return impl->loadTemplate(template_name, template_content);
}

bool HtmlTemplateManager::loadTemplateFromFile(const std::string& template_name, const std::string& file_path) {
    return impl->loadTemplateFromFile(template_name, file_path);
}

std::string HtmlTemplateManager::render(const std::string& template_name, const TemplateVariables& variables) {
    return impl->render(template_name, variables);
}

void HtmlTemplateManager::registerFunction(const std::string& function_name, TemplateFunction func) {
    impl->registerFunction(function_name, func);
}

void HtmlTemplateManager::setGlobalVariable(const std::string& key, const std::string& value) {
    impl->setGlobalVariable(key, value);
}

std::string HtmlTemplateManager::renderLoop(const std::string& template_content, const std::vector<TemplateVariables>& items) {
    return impl->renderLoop(template_content, items);
}