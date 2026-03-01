#include <sprawn/middleware/syntax_highlighter.h>
#include <sprawn/middleware/controller.h>

#include <algorithm>
#include <cctype>

namespace sprawn {

// ---------------------------------------------------------------------------
// SyntaxTheme
// ---------------------------------------------------------------------------

const TextStyle& SyntaxTheme::style_for(TokenType t) const {
    switch (t) {
        case TokenType::Keyword:       return keyword;
        case TokenType::Type:          return type;
        case TokenType::StringLiteral: return string_literal;
        case TokenType::CharLiteral:   return char_literal;
        case TokenType::Number:        return number;
        case TokenType::Comment:       return comment;
        case TokenType::Preprocessor:  return preprocessor;
        default:                       return plain;
    }
}

SyntaxTheme SyntaxTheme::dark_default() {
    SyntaxTheme t;
    t.plain          = {{220, 220, 220, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.keyword        = {{198, 120, 221, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.type           = {{86,  182, 194, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.string_literal = {{152, 195, 121, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.char_literal   = {{152, 195, 121, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.number         = {{209, 154, 102, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.comment        = {{106, 115, 125, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    t.preprocessor   = {{224, 108, 117, 255}, {0,0,0,0}, false, false, false, {0,0,0,0}};
    return t;
}

// ---------------------------------------------------------------------------
// LanguageDef
// ---------------------------------------------------------------------------

LanguageDef LanguageDef::cpp() {
    LanguageDef d;
    d.keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto",
        "bitand", "bitor", "break",
        "case", "catch", "class", "co_await", "co_return", "co_yield",
        "compl", "concept", "const", "const_cast", "consteval",
        "constexpr", "constinit", "continue",
        "decltype", "default", "delete", "do", "dynamic_cast",
        "else", "enum", "explicit", "export", "extern",
        "false", "for", "friend",
        "goto",
        "if", "inline",
        "module", "mutable",
        "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
        "operator", "or", "or_eq",
        "private", "protected", "public",
        "register", "reinterpret_cast", "requires", "return",
        "sizeof", "static", "static_assert", "static_cast",
        "struct", "switch",
        "template", "this", "throw", "true", "try", "typedef",
        "typeid", "typename",
        "union", "using",
        "virtual", "volatile",
        "while",
        "xor", "xor_eq",
    };
    d.types = {
        "bool", "char", "char8_t", "char16_t", "char32_t",
        "double", "float",
        "int", "int8_t", "int16_t", "int32_t", "int64_t",
        "long",
        "short", "signed",
        "size_t", "ssize_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "unsigned",
        "void", "wchar_t",
        "string", "string_view", "vector", "map", "set",
        "unordered_map", "unordered_set", "array", "pair", "tuple",
        "shared_ptr", "unique_ptr", "weak_ptr",
        "optional", "variant", "any",
        "FILE",
    };
    std::sort(d.keywords.begin(), d.keywords.end());
    std::sort(d.types.begin(), d.types.end());
    d.line_comment = "//";
    d.block_open   = "/*";
    d.block_close  = "*/";
    d.extensions   = {".cpp", ".cxx", ".cc", ".c", ".h", ".hpp", ".hxx", ".inl"};
    return d;
}

// ---------------------------------------------------------------------------
// SyntaxHighlighter
// ---------------------------------------------------------------------------

SyntaxHighlighter::SyntaxHighlighter(Controller& ctrl)
    : ctrl_(ctrl)
    , theme_(SyntaxTheme::dark_default())
{}

void SyntaxHighlighter::set_language(const LanguageDef& lang) {
    lang_   = lang;
    active_ = true;
    entry_state_.clear();
    states_valid_up_to_ = 0;
}

void SyntaxHighlighter::detect_language(const std::filesystem::path& filepath) {
    std::string ext = filepath.extension().string();
    if (ext.empty()) {
        active_ = false;
        return;
    }
    auto cpp_lang = LanguageDef::cpp();
    for (const auto& e : cpp_lang.extensions) {
        if (e == ext) {
            set_language(cpp_lang);
            return;
        }
    }
    active_ = false;
}

std::string_view SyntaxHighlighter::name() const {
    return "syntax";
}

int SyntaxHighlighter::base_priority() const {
    return 0;
}

void SyntaxHighlighter::on_edit(size_t line, size_t /*col*/,
                                 std::string_view /*text*/, bool /*is_insert*/) {
    if (!active_) return;
    if (line < states_valid_up_to_) {
        states_valid_up_to_ = line;
    }
    // Resize to match document — the vector will be grown lazily in ensure_states
    size_t lc = ctrl_.line_count();
    if (entry_state_.size() > lc + 1) {
        entry_state_.resize(lc + 1);
    }
}

LineDecoration SyntaxHighlighter::decorate(size_t line_number) const {
    LineDecoration result;
    if (!active_) return result;

    ensure_states(line_number);

    LineState entry = (line_number < entry_state_.size())
                          ? entry_state_[line_number]
                          : LineState::Normal;

    std::string text = ctrl_.line(line_number);
    auto [tokens, _] = scan_line(text, entry);

    result.spans.reserve(tokens.size());
    for (const auto& tok : tokens) {
        const TextStyle& s = theme_.style_for(tok.type);
        result.spans.push_back({tok.byte_start, tok.byte_end, s, 0});
    }
    return result;
}

// ---------------------------------------------------------------------------
// Lazy multi-line state
// ---------------------------------------------------------------------------

void SyntaxHighlighter::ensure_states(size_t line_number) const {
    size_t lc = ctrl_.line_count();
    if (entry_state_.empty()) {
        entry_state_.resize(lc + 1, LineState::Normal);
        states_valid_up_to_ = 0;
    }
    if (entry_state_.size() < lc + 1) {
        entry_state_.resize(lc + 1, LineState::Normal);
    }

    // Scan forward from states_valid_up_to_ up to and including line_number
    for (size_t i = states_valid_up_to_; i <= line_number && i < lc; ++i) {
        std::string text = ctrl_.line(i);
        auto [tokens, exit_state] = scan_line(text, entry_state_[i]);
        LineState next = exit_state;
        if (i + 1 < entry_state_.size()) {
            if (entry_state_[i + 1] == next && i >= states_valid_up_to_) {
                // State matches — everything beyond is still valid
                states_valid_up_to_ = i + 1;
                if (i >= line_number) break;
                continue;
            }
            entry_state_[i + 1] = next;
        }
        states_valid_up_to_ = i + 1;
    }
}

// ---------------------------------------------------------------------------
// Hand-written scanner
// ---------------------------------------------------------------------------

static bool is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_ident_char(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

ScanResult SyntaxHighlighter::scan_line(std::string_view text, LineState entry) const {
    std::vector<Token> tokens;
    int pos = 0;
    int len = static_cast<int>(text.size());

    // 1. Continue block comment from previous line
    if (entry == LineState::InBlockComment) {
        int close = -1;
        for (int i = 0; i + 1 < len; ++i) {
            if (text[i] == '*' && text[i + 1] == '/') {
                close = i + 2;
                break;
            }
        }
        if (close < 0) {
            tokens.push_back({0, len, TokenType::Comment});
            return {std::move(tokens), LineState::InBlockComment};
        }
        tokens.push_back({0, close, TokenType::Comment});
        pos = close;
    }

    while (pos < len) {
        char c = text[pos];

        // 2. Line comment
        if (c == '/' && pos + 1 < len && text[pos + 1] == '/') {
            tokens.push_back({pos, len, TokenType::Comment});
            return {std::move(tokens), LineState::Normal};
        }

        // 3. Block comment start
        if (c == '/' && pos + 1 < len && text[pos + 1] == '*') {
            int start = pos;
            pos += 2;
            while (pos + 1 < len) {
                if (text[pos] == '*' && text[pos + 1] == '/') {
                    pos += 2;
                    tokens.push_back({start, pos, TokenType::Comment});
                    goto next;
                }
                ++pos;
            }
            // Check last char pair
            if (pos < len) ++pos;
            tokens.push_back({start, len, TokenType::Comment});
            return {std::move(tokens), LineState::InBlockComment};
        next:
            continue;
        }

        // 4. Preprocessor directive: # at first non-whitespace
        if (c == '#') {
            // Check that everything before pos is whitespace
            bool leading_ws = true;
            for (int i = 0; i < pos; ++i) {
                if (text[i] != ' ' && text[i] != '\t') {
                    leading_ws = false;
                    break;
                }
            }
            if (leading_ws) {
                tokens.push_back({pos, len, TokenType::Preprocessor});
                return {std::move(tokens), LineState::Normal};
            }
        }

        // 5. String literal
        if (c == '"') {
            int start = pos;
            ++pos;
            while (pos < len) {
                if (text[pos] == '\\') {
                    pos += 2;
                    continue;
                }
                if (text[pos] == '"') {
                    ++pos;
                    break;
                }
                ++pos;
            }
            tokens.push_back({start, pos, TokenType::StringLiteral});
            continue;
        }

        // 6. Char literal
        if (c == '\'') {
            int start = pos;
            ++pos;
            while (pos < len) {
                if (text[pos] == '\\') {
                    pos += 2;
                    continue;
                }
                if (text[pos] == '\'') {
                    ++pos;
                    break;
                }
                ++pos;
            }
            tokens.push_back({start, pos, TokenType::CharLiteral});
            continue;
        }

        // 7. Number
        if (is_digit(c) || (c == '.' && pos + 1 < len && is_digit(text[pos + 1]))) {
            int start = pos;
            if (c == '0' && pos + 1 < len && (text[pos + 1] == 'x' || text[pos + 1] == 'X')) {
                // Hex
                pos += 2;
                while (pos < len && (is_hex_digit(text[pos]) || text[pos] == '\'')) ++pos;
            } else if (c == '0' && pos + 1 < len && (text[pos + 1] == 'b' || text[pos + 1] == 'B')) {
                // Binary
                pos += 2;
                while (pos < len && (text[pos] == '0' || text[pos] == '1' || text[pos] == '\'')) ++pos;
            } else {
                // Decimal / float
                while (pos < len && (is_digit(text[pos]) || text[pos] == '\'')) ++pos;
                if (pos < len && text[pos] == '.') {
                    ++pos;
                    while (pos < len && (is_digit(text[pos]) || text[pos] == '\'')) ++pos;
                }
                if (pos < len && (text[pos] == 'e' || text[pos] == 'E')) {
                    ++pos;
                    if (pos < len && (text[pos] == '+' || text[pos] == '-')) ++pos;
                    while (pos < len && is_digit(text[pos])) ++pos;
                }
            }
            // Integer/float suffixes: u, l, ll, f, etc.
            while (pos < len && (text[pos] == 'u' || text[pos] == 'U' ||
                                 text[pos] == 'l' || text[pos] == 'L' ||
                                 text[pos] == 'f' || text[pos] == 'F')) ++pos;
            tokens.push_back({start, pos, TokenType::Number});
            continue;
        }

        // 8. Identifier / keyword / type
        if (is_ident_start(c)) {
            int start = pos;
            while (pos < len && is_ident_char(text[pos])) ++pos;
            std::string_view word = text.substr(start, pos - start);
            std::string word_str(word);

            if (std::binary_search(lang_.keywords.begin(), lang_.keywords.end(), word_str)) {
                tokens.push_back({start, pos, TokenType::Keyword});
            } else if (std::binary_search(lang_.types.begin(), lang_.types.end(), word_str)) {
                tokens.push_back({start, pos, TokenType::Type});
            }
            // Plain identifiers → no token (gaps get default style)
            continue;
        }

        // 9. Skip anything else
        ++pos;
    }

    return {std::move(tokens), LineState::Normal};
}

} // namespace sprawn
