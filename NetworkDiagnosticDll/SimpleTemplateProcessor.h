#pragma once

#include <string>
#include <map>
#include <regex>

class SimpleTemplateProcessor {
public:
    using Variables = std::map<std::string, std::string>;

    static std::string render(const std::string& template_content, const Variables& variables) {
        std::string result = template_content;

        // 替换所有 {{variable_name}} 占位符
        std::regex var_regex(R"(\{\{([^}]+)\}\})");
        std::smatch match;

        while (std::regex_search(result, match, var_regex)) {
            std::string var_name = match[1].str();
            std::string replacement = "";

            auto it = variables.find(var_name);
            if (it != variables.end()) {
                replacement = it->second;
            }

            result.replace(match.position(), match.length(), replacement);
        }

        return result;
    }

    // 转义HTML特殊字符
    static std::string escapeHtml(const std::string& input) {
        std::string result = input;

        // 替换HTML特殊字符
        std::map<std::string, std::string> replacements = {
            {"&", "&amp;"},
            {"<", "&lt;"},
            {">", "&gt;"},
            {"\"", "&quot;"},
            {"'", "&#39;"}
        };

        for (const auto& [from, to] : replacements) {
            size_t pos = 0;
            while ((pos = result.find(from, pos)) != std::string::npos) {
                result.replace(pos, from.length(), to);
                pos += to.length();
            }
        }

        return result;
    }
};