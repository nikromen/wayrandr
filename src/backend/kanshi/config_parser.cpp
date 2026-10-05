#include "backend/kanshi/config_parser.hpp"

#include <stdio.h>  // NOLINT: POSIX fmemopen() requires this C header.

extern "C" {
#include <scfg.h>
}

#include <charconv>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>  // NOLINT(build/c++11): C++17 from_chars requires std::errc.
#include <utility>

#include "backend/kanshi/types.hpp"

namespace {

constexpr std::string_view K_OUTPUT = "output";
constexpr std::string_view K_MULTI_OUTPUT = "...output";
constexpr std::string_view K_PROFILE = "profile";
constexpr std::string_view K_INCLUDE = "include";
constexpr std::string_view K_EXEC = "exec";
constexpr std::string_view K_ENABLE = "enable";
constexpr std::string_view K_DISABLE = "disable";
constexpr std::string_view K_MODE = "mode";
constexpr std::string_view K_PREFERRED = "preferred";
constexpr std::string_view K_POSITION = "position";
constexpr std::string_view K_SCALE = "scale";
constexpr std::string_view K_TRANSFORM = "transform";
constexpr std::string_view K_ADAPTIVE_SYNC = "adaptive_sync";
constexpr std::string_view K_ON = "on";
constexpr std::string_view K_OFF = "off";

void require_line_value(const std::string & value) {
    // libscfg 0.2 treats NUL as EOF and does not permit newlines in quoted words.
    // Other bytes (including backslashes, CR, tabs and UTF-8) remain literal data.
    if (value.find('\0') != std::string::npos || value.find('\n') != std::string::npos) {
        throw kanshi_config_parser::ParseError("Kanshi values cannot contain NUL or newlines");
    }
}

void check_nesting(std::string_view content) {
    // Supported Kanshi directives need at most two block levels. Bound the C
    // parser's recursion before handing it deeply nested, unsupported input.
    unsigned depth = 0;
    char quote = '\0';
    bool directive_start = true;
    for (size_t index = 0; index < content.size(); ++index) {
        const char character = content[index];
        if (character == '\\' && quote != '\'' && index + 1 < content.size()) {
            ++index;
            directive_start = false;
            continue;
        }
        if (quote != '\0') {
            if (character == quote) {
                quote = '\0';
            }
            continue;
        }
        if (character == '#' && directive_start) {
            while (index < content.size() && content[index] != '\n') {
                ++index;
            }
            continue;
        }
        if (character == '\n' || character == '{') {
            directive_start = true;
        } else if (character != ' ' && character != '\t') {
            directive_start = false;
        }
        if (character == '\'' || character == '"') {
            quote = character;
        } else if (character == '{') {
            if (++depth > 32) {
                throw kanshi_config_parser::ParseError("Kanshi block nesting is too deep");
            }
        } else if (character == '}' && depth > 0) {
            --depth;
        }
    }
}

void validate_exec(const std::string & command) {
    require_line_value(command);
    if (command.find_first_not_of(" \t") == std::string::npos) {
        return;
    }
    // A final parameter must remain separate: this also detects a trailing escape
    // and empty child blocks, which libscfg's tree cannot distinguish from no block.
    constexpr std::string_view boundary = "wayrandr_exec_boundary";
    auto content = "exec " + command + " " + std::string(boundary) + "\n";
    check_nesting(content);
    const auto close_input = [](FILE * stream) { fclose(stream); };
    const std::unique_ptr<FILE, decltype(close_input)> input(
        fmemopen(content.data(), content.size(), "r"), close_input
    );
    if (!input) {
        throw kanshi_config_parser::ParseError("Cannot open Kanshi exec validation buffer");
    }
    scfg_block block{};
    // The guard releases the C parser's allocations, including partial error results.
    const std::unique_ptr<scfg_block, decltype(&scfg_block_finish)> guard(
        &block, &scfg_block_finish
    );
    const int result = scfg_parse_file(&block, input.get());
    if (result != 0 || block.directives_len != 1) {
        throw kanshi_config_parser::ParseError("Kanshi exec must be one valid scfg directive");
    }
    const auto & directive = block.directives[0];
    if (directive.name != K_EXEC || directive.children.directives_len != 0 ||
        directive.params_len < 2 || directive.params[directive.params_len - 1] != boundary) {
        throw kanshi_config_parser::ParseError(
            "Kanshi exec must be one directive without a child block or incomplete escape"
        );
    }
}

auto format_criteria(const std::string & value) -> std::string;

auto format_exec_word(const std::string & value) -> std::string {
    if (value.empty() || value.find_first_of(" \t\r\v\f{}\\\"'") != std::string::npos) {
        return format_criteria(value);
    }
    return value;
}

[[noreturn]] void invalid(const scfg_directive & directive, const std::string & message) {
    throw kanshi_config_parser::ParseError(
        "Kanshi line " + std::to_string(directive.lineno) + ": " + message
    );
}

auto parse_scale(const std::string & token) -> float {
    const char * begin = token.data();
    const char * const end = begin + token.size();
    while (begin != end && std::string_view(" \t\r\v\f").find(*begin) != std::string_view::npos) {
        ++begin;
    }
    bool negative = false;
    if (begin != end && (*begin == '+' || *begin == '-')) {
        negative = *begin == '-';
        ++begin;
    }
    if (begin == end || *begin == '+' || *begin == '-') {
        throw kanshi_config_parser::ParseError("Invalid scale value: " + token);
    }
    auto format = std::chars_format::general;
    if (end - begin >= 2 && begin[0] == '0' && (begin[1] == 'x' || begin[1] == 'X')) {
        begin += 2;
        format = std::chars_format::hex;
    }
    float scale = 0;
    const auto result = std::from_chars(begin, end, scale, format);
    if (result.ec != std::errc() || result.ptr != end) {
        throw kanshi_config_parser::ParseError("Invalid scale value: " + token);
    }
    if (negative) {
        return -scale;
    }
    return scale;
}

void validate_integer(const std::string & value) {
    const char * begin = value.data();
    const char * const end = begin + value.size();
    if (begin != end && *begin == '+') {
        ++begin;
        if (begin != end && *begin == '-') {
            throw kanshi_config_parser::ParseError("Invalid integer: " + value);
        }
    }
    int number = 0;
    const auto result = std::from_chars(begin, end, number);
    if (result.ec != std::errc() || result.ptr != end) {
        throw kanshi_config_parser::ParseError("Invalid integer: " + value);
    }
}

void validate_mode(const std::string & value) {
    const auto x = value.find('x');
    const auto at = value.find('@');
    if (x == std::string::npos) {
        throw kanshi_config_parser::ParseError("Invalid mode: " + value);
    }
    validate_integer(value.substr(0, x));
    validate_integer(value.substr(x + 1, at - x - 1));
    if (at != std::string::npos) {
        auto refresh = value.substr(at + 1);
        if (refresh.size() >= 2 && refresh.substr(refresh.size() - 2) == "Hz") {
            refresh.resize(refresh.size() - 2);
        }
        (void)parse_scale(refresh);
    }
}

class Parser {
public:
    [[nodiscard]] auto parse_document(const scfg_block & block) -> KanshiConfig {
        KanshiConfig config;
        std::set<std::string> profile_ids;
        std::set<std::string> global_criteria;
        unsigned anonymous_index = 0;
        for (size_t index = 0; index < block.directives_len; ++index) {
            const auto & directive = block.directives[index];
            const std::string name = directive.name;
            if (name == K_INCLUDE) {
                if (directive.params_len != 1 || directive.children.directives_len != 0) {
                    invalid(directive, "include requires exactly one path and no child directives");
                }
                if (!config.profiles.empty() || !config.global_outputs.empty()) {
                    invalid(
                        directive,
                        "include after a profile or output default cannot be safely reordered; "
                        "edit this configuration manually"
                    );
                }
                config.includes.emplace_back(directive.params[0]);
            } else if (name == K_OUTPUT) {
                auto output = parse_output(directive);
                if (output.criteria == "*" ||
                    (!output.criteria.empty() && output.criteria[0] == '$')) {
                    invalid(directive, "global output cannot use wildcard '*' or an alias");
                }
                if (!global_criteria.insert(output.criteria).second) {
                    invalid(directive, "duplicate global output: " + output.criteria);
                }
                config.global_outputs.push_back(std::move(output));
            } else if (name == K_PROFILE) {
                if (directive.params_len > 1) {
                    invalid(directive, "profile requires zero or one name");
                }
                KanshiProfile profile;
                profile.anonymous = directive.params_len == 0;
                if (profile.anonymous) {
                    profile.id = "<anonymous profile " + std::to_string(++anonymous_index) + ">";
                } else {
                    profile.id = directive.params[0];
                }
                if (!profile_ids.insert(profile.id).second) {
                    invalid(
                        directive, "duplicate profile name cannot be edited safely: " + profile.id
                    );
                }
                std::set<std::string> criteria;
                for (size_t child_index = 0; child_index < directive.children.directives_len;
                     ++child_index) {
                    const auto & child = directive.children.directives[child_index];
                    const std::string child_name = child.name;
                    if (child_name == K_OUTPUT || child_name == K_MULTI_OUTPUT) {
                        auto output = parse_output(child);
                        if (output.alias.has_value()) {
                            invalid(child, "output aliases can only be defined in global scope");
                        }
                        if (!criteria.insert(output.criteria).second) {
                            invalid(child, "duplicate profile output: " + output.criteria);
                        }
                        profile.outputs.push_back(std::move(output));
                    } else if (child_name == K_EXEC) {
                        if (child.params_len == 0 || child.children.directives_len != 0) {
                            invalid(child, "exec requires a command and no child directives");
                        }
                        // Re-quote scfg words, not shell words. Kanshi's own re-escaping
                        // then produces the same shell command as the original directive.
                        std::string command;
                        for (size_t param = 0; param < child.params_len; ++param) {
                            if (param > 0) {
                                command += " ";
                            }
                            command += format_exec_word(child.params[param]);
                        }
                        profile.exec.push_back(std::move(command));
                    } else {
                        invalid(child, "unsupported profile directive: " + child_name);
                    }
                }
                config.profiles.push_back(std::move(profile));
            } else {
                invalid(directive, "unsupported top-level directive: " + name);
            }
        }
        return config;
    }

private:
    [[nodiscard]] static auto parse_output(const scfg_directive & directive)
        -> KanshiOutputSetting {
        if (directive.params_len == 0) {
            invalid(directive, "output requires criteria");
        }
        KanshiOutputSetting output;
        output.criteria = directive.params[0];
        output.multi_output = directive.name == K_MULTI_OUTPUT;
        std::set<std::string> options;
        size_t index = 1;
        while (index < directive.params_len) {
            const std::string name = directive.params[index++];
            index += parse_option(
                output,
                name,
                directive.params + index,
                directive.params_len - index,
                directive,
                options
            );
        }
        for (size_t child_index = 0; child_index < directive.children.directives_len;
             ++child_index) {
            const auto & child = directive.children.directives[child_index];
            if (child.children.directives_len != 0) {
                invalid(child, "nested output options are not supported");
            }
            const auto consumed =
                parse_option(output, child.name, child.params, child.params_len, child, options);
            if (consumed != child.params_len) {
                invalid(child, "expected one output option per directive");
            }
        }
        return output;
    }

    [[nodiscard]] static auto parse_option(
        KanshiOutputSetting & output,
        const std::string & name,
        char * const * params,
        size_t count,
        const scfg_directive & directive,
        std::set<std::string> & options
    ) -> size_t {
        std::string key = name;
        if (name == K_DISABLE) {
            key = K_ENABLE;
        }
        if (!options.insert(key).second) {
            invalid(
                directive, "repeated/conflicting output option cannot be edited safely: " + name
            );
        }
        if (name == K_ENABLE || name == K_DISABLE) {
            output.enabled = name == K_ENABLE;
            return 0;
        }
        if (count == 0) {
            invalid(directive, "missing value for output option: " + name);
        }
        const std::string value = params[0];
        if (name == K_MODE) {
            if (value == K_PREFERRED) {
                output.preferred = true;
            } else if (value == "--custom") {
                if (count < 2) {
                    invalid(directive, "mode --custom requires a mode");
                }
                validate_mode(params[1]);
                output.mode = "--custom " + std::string(params[1]);
                return 2;
            } else {
                validate_mode(value);
                output.mode = value;
            }
        } else if (name == K_POSITION) {
            const auto comma = value.find(',');
            if (comma == std::string::npos) {
                invalid(directive, "position requires x,y");
            }
            validate_integer(value.substr(0, comma));
            validate_integer(value.substr(comma + 1));
            output.position = value;
        } else if (name == K_SCALE) {
            output.scale = parse_scale(value);
        } else if (name == K_TRANSFORM) {
            if (value != "normal" && value != "90" && value != "180" && value != "270" &&
                value != "flipped" && value != "flipped-90" && value != "flipped-180" &&
                value != "flipped-270") {
                invalid(directive, "invalid transform: " + value);
            }
            output.transform = value;
        } else if (name == K_ADAPTIVE_SYNC) {
            if (value != K_ON && value != K_OFF) {
                invalid(directive, "invalid adaptive_sync: " + value);
            }
            output.adaptive_sync = value == K_ON;
        } else if (name == "alias") {
            if (value.empty() || value[0] != '$') {
                invalid(directive, "alias must start with '$'");
            }
            output.alias = value;
        } else {
            invalid(directive, "unsupported output option: " + name);
        }
        return 1;
    }
};

auto format_criteria(const std::string & value) -> std::string {
    require_line_value(value);
    std::string quoted = "\"";
    for (const char character : value) {
        if (character == '\\' || character == '\"') {
            quoted.push_back('\\');
        }
        quoted.push_back(character);
    }
    quoted.push_back('\"');
    return quoted;
}

void write_output_directives(std::ostringstream & out, const KanshiOutputSetting & setting) {
    if (setting.alias.has_value()) {
        out << "\t\talias " << format_criteria(*setting.alias) << '\n';
    }
    if (setting.enabled.has_value()) {
        if (setting.enabled.value()) {
            out << "\t\tenable\n";
        } else {
            out << "\t\tdisable\n";
        }
    }
    if (setting.preferred) {
        out << "\t\tmode preferred\n";
    } else if (setting.mode.has_value()) {
        if (setting.mode->rfind("--custom ", 0) == 0) {
            out << "\t\tmode --custom " << format_criteria(setting.mode->substr(9)) << '\n';
        } else {
            out << "\t\tmode " << format_criteria(*setting.mode) << '\n';
        }
    }
    if (setting.position.has_value()) {
        out << "\t\tposition " << format_criteria(setting.position.value()) << '\n';
    }
    if (setting.scale.has_value()) {
        out << "\t\tscale " << setting.scale.value() << '\n';
    }
    if (setting.transform.has_value()) {
        out << "\t\ttransform " << format_criteria(setting.transform.value()) << '\n';
    }
    if (setting.adaptive_sync.has_value()) {
        if (setting.adaptive_sync.value()) {
            out << "\t\tadaptive_sync on\n";
        } else {
            out << "\t\tadaptive_sync off\n";
        }
    }
}

auto count_output_directives(const KanshiOutputSetting & setting) -> int {
    int count = 0;
    if (setting.alias.has_value()) {
        ++count;
    }
    if (setting.enabled.has_value()) {
        ++count;
    }
    if (setting.preferred || setting.mode.has_value()) {
        ++count;
    }
    if (setting.position.has_value()) {
        ++count;
    }
    if (setting.scale.has_value()) {
        ++count;
    }
    if (setting.transform.has_value()) {
        ++count;
    }
    if (setting.adaptive_sync.has_value()) {
        ++count;
    }
    return count;
}

void write_output_setting(std::ostringstream & out, const KanshiOutputSetting & setting) {
    if (setting.multi_output) {
        out << "\t...output ";
    } else {
        out << "\toutput ";
    }
    out << format_criteria(setting.criteria);
    const int directive_count = count_output_directives(setting);
    if (directive_count == 0) {
        out << '\n';
        return;
    }
    if (directive_count == 1 && setting.enabled.has_value() && !setting.enabled.value()) {
        out << " disable\n";
        return;
    }
    if (directive_count == 1 && setting.enabled.has_value() && setting.enabled.value()) {
        out << " enable\n";
        return;
    }

    out << " {\n";
    write_output_directives(out, setting);
    out << "\t}\n";
}

}  // namespace

namespace kanshi_config_parser {

auto parse(const std::string & content) -> KanshiConfig {
    if (content.find('\0') != std::string::npos) {
        throw ParseError("Kanshi configuration cannot contain NUL bytes");
    }
    check_nesting(content);
    // Use the same syntax parser as Kanshi 1.9.0, including directive boundaries.
    auto buffer = content;
    const auto close_input = [](FILE * stream) { fclose(stream); };
    const std::unique_ptr<FILE, decltype(close_input)> input(
        fmemopen(buffer.data(), buffer.size(), "r"), close_input
    );
    if (!input) {
        throw ParseError("Cannot open Kanshi parsing buffer");
    }
    scfg_block block{};
    const std::unique_ptr<scfg_block, decltype(&scfg_block_finish)> guard(
        &block, &scfg_block_finish
    );
    if (scfg_parse_file(&block, input.get()) != 0) {
        throw ParseError("Invalid Kanshi/scfg syntax; configuration was not loaded");
    }
    Parser parser;
    auto config = parser.parse_document(block);
    return config;
}

auto serialize(const KanshiConfig & config) -> std::string {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10);

    for (const auto & include_path : config.includes) {
        out << "include " << format_criteria(include_path) << "\n\n";
    }

    if (!config.preserved_directives.empty()) {
        throw ParseError("Unsupported Kanshi directives cannot be saved safely");
    }

    for (const auto & global_output : config.global_outputs) {
        if (global_output.multi_output) {
            out << "...output ";
        } else {
            out << "output ";
        }
        out << format_criteria(global_output.criteria);
        if (count_output_directives(global_output) == 0) {
            out << '\n';
        } else if (count_output_directives(global_output) == 1 &&
                   global_output.enabled.has_value()) {
            if (global_output.enabled.value()) {
                out << " enable\n";
            } else {
                out << " disable\n";
            }
        } else {
            out << " {\n";
            write_output_directives(out, global_output);
            out << "}\n";
        }
        out << '\n';
    }

    for (const auto & profile : config.profiles) {
        out << "profile";
        if (!profile.anonymous) {
            out << " " << format_criteria(profile.id);
        }
        out << " {\n";
        for (const auto & output : profile.outputs) {
            write_output_setting(out, output);
        }
        for (const auto & command : profile.exec) {
            validate_exec(command);
            if (command.find_first_not_of(" \t") == std::string::npos) {
                continue;
            }
            out << "\texec " << command << '\n';
        }
        out << "}\n\n";
    }

    const auto content = out.str();
    (void)parse(content);
    return content;
}

}  // namespace kanshi_config_parser
