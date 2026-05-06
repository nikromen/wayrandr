#include "backend/kanshi/config_parser.hpp"

#include <spdlog/spdlog.h>

#include <cctype>
#include <charconv>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <locale>
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

class Tokenizer {
public:
    explicit Tokenizer(std::string_view input)
        : input_(input) {}

    [[nodiscard]] auto eof() const -> bool { return pos_ >= input_.size(); }

    [[nodiscard]] auto peek() -> std::string {
        const size_t saved = pos_;
        const auto token = next();
        pos_ = saved;
        return token;
    }

    [[nodiscard]] auto next() -> std::string {
        skip_ignored();
        if (eof()) {
            return {};
        }

        const char current = input_[pos_];
        if (current == '{') {
            ++pos_;
            return "{";
        }
        if (current == '}') {
            ++pos_;
            return "}";
        }

        if (current == '"') {
            return read_quoted_string();
        }

        return read_bare_token();
    }

    [[nodiscard]] auto raw_next() -> std::string {
        skip_ignored();
        const size_t start = pos_;
        (void)next();
        return std::string(input_.substr(start, pos_ - start));
    }

    [[nodiscard]] auto remainder_of_line() -> std::string {
        while (!eof() && (input_[pos_] == ' ' || input_[pos_] == '\t')) {
            ++pos_;
        }
        const size_t start = pos_;
        while (!eof() && input_[pos_] != '\n') {
            ++pos_;
        }
        if (!eof()) {
            ++pos_;
        }
        std::string line(input_.substr(start, pos_ - start));
        while (!line.empty() && (std::isspace(static_cast<unsigned char>(line.back())) != 0)) {
            line.pop_back();
        }
        return line;
    }

private:
    std::string_view input_;
    size_t pos_ = 0;

    void skip_ignored() {
        while (!eof()) {
            if (std::isspace(static_cast<unsigned char>(input_[pos_])) != 0) {
                ++pos_;
                continue;
            }
            if (input_[pos_] == '#') {
                while (!eof() && input_[pos_] != '\n') {
                    ++pos_;
                }
                continue;
            }
            break;
        }
    }

    [[nodiscard]] auto read_quoted_string() -> std::string {
        ++pos_;
        std::string value;
        while (!eof() && input_[pos_] != '"') {
            if (input_[pos_] == '\\' && pos_ + 1 < input_.size()) {
                value.push_back(input_[pos_ + 1]);
                pos_ += 2;
                continue;
            }
            value.push_back(input_[pos_]);
            ++pos_;
        }
        if (eof()) {
            throw kanshi_config_parser::ParseError("Unterminated quoted string");
        }
        ++pos_;
        return value;
    }

    [[nodiscard]] auto read_bare_token() -> std::string {
        const size_t start = pos_;
        while (!eof()) {
            const char current = input_[pos_];
            if ((std::isspace(static_cast<unsigned char>(current)) != 0) || current == '{' ||
                current == '}' || current == '#') {
                break;
            }
            ++pos_;
        }
        return std::string(input_.substr(start, pos_ - start));
    }
};

class Parser {
public:
    explicit Parser(std::string_view input)
        : tokenizer_(input) {}

    [[nodiscard]] auto parse_document() -> KanshiConfig {
        KanshiConfig config;
        int unnamed_profile_index = 0;

        while (!tokenizer_.eof()) {
            const auto keyword = tokenizer_.peek();
            if (keyword.empty()) {
                break;
            }

            if (keyword == K_INCLUDE) {
                (void)tokenizer_.next();
                config.includes.push_back(tokenizer_.next());
                continue;
            }

            if (keyword == K_PROFILE) {
                config.profiles.push_back(parse_profile(++unnamed_profile_index));
                continue;
            }

            if (keyword == K_OUTPUT || keyword == K_MULTI_OUTPUT) {
                config.global_outputs.push_back(parse_output_setting());
                continue;
            }

            spdlog::warn("Preserving unsupported kanshi directive: {}", keyword);
            std::string preserved_line = tokenizer_.raw_next();
            const auto rest = tokenizer_.remainder_of_line();
            if (!rest.empty()) {
                preserved_line += " " + rest;
            }
            config.preserved_directives.push_back(std::move(preserved_line));
        }

        return config;
    }

private:
    Tokenizer tokenizer_;

    [[nodiscard]] auto parse_profile(int & unnamed_profile_index) -> KanshiProfile {
        expect_keyword(K_PROFILE);

        KanshiProfile profile;
        const auto maybe_name = tokenizer_.peek();
        if (!maybe_name.empty() && maybe_name != "{") {
            profile.id = tokenizer_.next();
        } else {
            profile.id = "profile_" + std::to_string(unnamed_profile_index);
        }

        expect_token("{");
        while (true) {
            const auto keyword = tokenizer_.peek();
            if (keyword == "}") {
                (void)tokenizer_.next();
                break;
            }
            if (keyword == K_OUTPUT || keyword == K_MULTI_OUTPUT) {
                profile.outputs.push_back(parse_output_setting());
                continue;
            }
            if (keyword == K_EXEC) {
                (void)tokenizer_.next();
                profile.exec.push_back(tokenizer_.remainder_of_line());
                continue;
            }
            if (keyword.empty()) {
                throw kanshi_config_parser::ParseError("Unexpected end of profile block");
            }
            spdlog::warn("Skipping unsupported profile directive: {}", keyword);
            (void)tokenizer_.next();
            (void)tokenizer_.remainder_of_line();
        }

        return profile;
    }

    [[nodiscard]] auto parse_output_setting() -> KanshiOutputSetting {
        const auto keyword = tokenizer_.next();
        if (keyword != K_OUTPUT && keyword != K_MULTI_OUTPUT) {
            throw kanshi_config_parser::ParseError("Expected output directive");
        }

        KanshiOutputSetting setting;
        setting.multi_output = keyword == K_MULTI_OUTPUT;
        setting.criteria = read_criteria();
        parse_output_directives(setting);
        return setting;
    }

    [[nodiscard]] auto read_criteria() -> std::string {
        const auto token = tokenizer_.peek();
        if (token == "{") {
            throw kanshi_config_parser::ParseError("Missing output criteria");
        }
        return tokenizer_.next();
    }

    void parse_output_directives(KanshiOutputSetting & setting) {
        if (tokenizer_.peek() == "{") {
            (void)tokenizer_.next();
            while (tokenizer_.peek() != "}") {
                parse_output_directive(setting);
            }
            expect_token("}");
            return;
        }

        while (is_output_directive(tokenizer_.peek())) {
            parse_output_directive(setting);
        }
    }

    [[nodiscard]] static auto is_output_directive(const std::string & token) -> bool {
        return token == K_ENABLE || token == K_DISABLE || token == K_MODE || token == K_POSITION ||
            token == K_SCALE || token == K_TRANSFORM || token == K_ADAPTIVE_SYNC ||
            token == "alias";
    }

    void parse_output_directive(KanshiOutputSetting & setting) {
        const auto keyword = tokenizer_.next();
        if (keyword == "alias") {
            setting.alias = tokenizer_.next();
            return;
        }
        if (keyword == K_ENABLE) {
            setting.enabled = true;
            return;
        }
        if (keyword == K_DISABLE) {
            setting.enabled = false;
            return;
        }
        if (keyword == K_MODE) {
            const auto value = tokenizer_.next();
            if (value == K_PREFERRED) {
                setting.preferred = true;
                setting.mode.reset();
                return;
            }
            setting.preferred = false;
            if (value == "--custom") {
                setting.mode = "--custom " + tokenizer_.next();
                return;
            }
            setting.mode = value;
            return;
        }
        if (keyword == K_POSITION) {
            setting.position = tokenizer_.next();
            return;
        }
        if (keyword == K_SCALE) {
            const auto token = tokenizer_.next();
            const char * begin = token.data();
            const char * const end = begin + token.size();
            if (begin != end && *begin == '+') {
                ++begin;
            }
            float scale = 0;
            const auto result = std::from_chars(begin, end, scale);
            if (result.ec != std::errc() || result.ptr != end) {
                throw kanshi_config_parser::ParseError("Invalid scale value");
            }
            setting.scale = scale;
            return;
        }
        if (keyword == K_TRANSFORM) {
            setting.transform = tokenizer_.next();
            return;
        }
        if (keyword == K_ADAPTIVE_SYNC) {
            const auto value = tokenizer_.next();
            if (value == K_ON) {
                setting.adaptive_sync = true;
                return;
            }
            if (value == K_OFF) {
                setting.adaptive_sync = false;
                return;
            }
            throw kanshi_config_parser::ParseError("Invalid adaptive_sync value: " + value);
        }

        throw kanshi_config_parser::ParseError("Unknown output directive: " + keyword);
    }

    void expect_keyword(std::string_view keyword) {
        const auto token = tokenizer_.next();
        if (token != keyword) {
            throw kanshi_config_parser::ParseError(
                "Expected '" + std::string(keyword) + "', got '" + token + "'"
            );
        }
    }

    void expect_token(const std::string & token) {
        const auto actual = tokenizer_.next();
        if (actual != token) {
            throw kanshi_config_parser::ParseError(
                "Expected '" + token + "', got '" + actual + "'"
            );
        }
    }
};

auto format_criteria(const std::string & value) -> std::string {
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
    Parser parser(content);
    return parser.parse_document();
}

auto serialize(const KanshiConfig & config) -> std::string {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10);

    for (const auto & include_path : config.includes) {
        out << "include " << format_criteria(include_path) << "\n\n";
    }

    for (const auto & preserved : config.preserved_directives) {
        out << preserved << '\n';
    }
    if (!config.preserved_directives.empty()) {
        out << '\n';
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
        out << "profile " << format_criteria(profile.id) << " {\n";
        for (const auto & output : profile.outputs) {
            write_output_setting(out, output);
        }
        for (const auto & command : profile.exec) {
            out << "\texec " << command << '\n';
        }
        out << "}\n\n";
    }

    return out.str();
}

}  // namespace kanshi_config_parser
