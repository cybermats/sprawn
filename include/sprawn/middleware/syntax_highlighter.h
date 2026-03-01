#pragma once

#include <sprawn/decoration.h>
#include <sprawn/middleware/decoration_source.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sprawn {

class Controller;

enum class TokenType {
    Plain,
    Keyword,
    Type,
    StringLiteral,
    CharLiteral,
    Number,
    Comment,
    Preprocessor,
};

struct Token {
    int       byte_start;
    int       byte_end;
    TokenType type;
};

enum class LineState {
    Normal,
    InBlockComment,
};

struct SyntaxTheme {
    TextStyle plain;
    TextStyle keyword;
    TextStyle type;
    TextStyle string_literal;
    TextStyle char_literal;
    TextStyle number;
    TextStyle comment;
    TextStyle preprocessor;

    const TextStyle& style_for(TokenType t) const;
    static SyntaxTheme dark_default();
};

struct LanguageDef {
    std::vector<std::string> keywords;       // sorted
    std::vector<std::string> types;          // sorted
    std::string              line_comment;   // e.g. "//"
    std::string              block_open;     // e.g. "/*"
    std::string              block_close;    // e.g. "*/"
    std::vector<std::string> extensions;     // e.g. ".cpp", ".h"

    static LanguageDef cpp();
};

struct ScanResult {
    std::vector<Token> tokens;
    LineState          exit_state;
};

class SyntaxHighlighter : public DecorationSource {
public:
    explicit SyntaxHighlighter(Controller& ctrl);

    void set_language(const LanguageDef& lang);
    void detect_language(const std::filesystem::path& filepath);

    // DecorationSource interface
    LineDecoration   decorate(size_t line_number) const override;
    std::string_view name() const override;
    int              base_priority() const override;
    void             on_edit(size_t line, size_t col,
                             std::string_view text, bool is_insert) override;

    // Exposed for testing
    ScanResult scan_line(std::string_view text, LineState entry) const;

private:
    void ensure_states(size_t line_number) const;

    Controller&   ctrl_;
    LanguageDef   lang_;
    SyntaxTheme   theme_;
    bool          active_{false};

    // Mutable for lazy computation in const decorate()
    mutable std::vector<LineState> entry_state_;
    mutable size_t                 states_valid_up_to_{0};
};

} // namespace sprawn
