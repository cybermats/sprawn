#include <doctest/doctest.h>

#include <sprawn/document.h>
#include <sprawn/middleware/controller.h>
#include <sprawn/middleware/syntax_highlighter.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <unistd.h>

using namespace sprawn;

namespace {

class TempFile {
public:
    explicit TempFile(const std::string& content,
                      const std::string& suffix = "") {
        std::string tmpl =
            (std::filesystem::temp_directory_path() / "sprawn_syn_XXXXXX").string();
        int fd = mkstemp(tmpl.data());
        if (fd == -1) throw std::runtime_error("mkstemp failed");
        if (!suffix.empty()) {
            std::string newpath = tmpl + suffix;
            std::rename(tmpl.c_str(), newpath.c_str());
            tmpl = newpath;
        }
        path_ = tmpl;
        ::write(fd, content.data(), content.size());
        ::close(fd);
    }
    ~TempFile() { std::filesystem::remove(path_); }
    const std::filesystem::path& path() const { return path_; }
private:
    std::filesystem::path path_;
};

// Helper: create a highlighter with C++ language on inline content
struct TestFixture {
    Document    doc;
    Controller  ctrl{doc};
    SyntaxHighlighter hl{ctrl};

    TestFixture() {
        hl.set_language(LanguageDef::cpp());
    }
};

// Convenience: scan a single line with Normal entry state
ScanResult scan(const SyntaxHighlighter& hl, std::string_view text,
                LineState entry = LineState::Normal) {
    return hl.scan_line(text, entry);
}

bool has_token(const std::vector<Token>& tokens, TokenType type,
               int start, int end) {
    for (const auto& t : tokens) {
        if (t.type == type && t.byte_start == start && t.byte_end == end)
            return true;
    }
    return false;
}

} // namespace

// ===================================================================
// Scanner Basics
// ===================================================================

TEST_CASE("Scanner: keyword identification") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "if (x) return;");
    CHECK(has_token(tokens, TokenType::Keyword, 0, 2));   // "if"
    CHECK(has_token(tokens, TokenType::Keyword, 7, 13));   // "return"
    CHECK(exit == LineState::Normal);
}

TEST_CASE("Scanner: type identification") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "int x = 0;");
    CHECK(has_token(tokens, TokenType::Type, 0, 3));  // "int"
}

TEST_CASE("Scanner: line comment") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "x = 1; // comment");
    CHECK(has_token(tokens, TokenType::Comment, 7, 17));
    CHECK(exit == LineState::Normal);
}

TEST_CASE("Scanner: block comment single line") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "x /* comment */ y");
    CHECK(has_token(tokens, TokenType::Comment, 2, 15));
    CHECK(exit == LineState::Normal);
}

TEST_CASE("Scanner: string literal") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, R"(auto s = "hello world";)");
    CHECK(has_token(tokens, TokenType::Keyword, 0, 4));       // "auto"
    CHECK(has_token(tokens, TokenType::StringLiteral, 9, 22)); // "hello world"
}

TEST_CASE("Scanner: string with escape") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, R"(char* s = "he\"llo";)");
    CHECK(has_token(tokens, TokenType::StringLiteral, 10, 19));
}

TEST_CASE("Scanner: char literal") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "char c = 'x';");
    CHECK(has_token(tokens, TokenType::CharLiteral, 9, 12));
}

TEST_CASE("Scanner: char literal with escape") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, R"(char c = '\'';)");
    CHECK(has_token(tokens, TokenType::CharLiteral, 9, 13));
}

TEST_CASE("Scanner: integer number") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "int x = 42;");
    CHECK(has_token(tokens, TokenType::Number, 8, 10));
}

TEST_CASE("Scanner: hex number") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "int x = 0xFF;");
    CHECK(has_token(tokens, TokenType::Number, 8, 12));
}

TEST_CASE("Scanner: binary number") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "int x = 0b1010;");
    CHECK(has_token(tokens, TokenType::Number, 8, 14));
}

TEST_CASE("Scanner: float number") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "double d = 3.14f;");
    CHECK(has_token(tokens, TokenType::Number, 11, 16));
}

TEST_CASE("Scanner: preprocessor directive") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "#include <stdio.h>");
    CHECK(has_token(tokens, TokenType::Preprocessor, 0, 18));
}

TEST_CASE("Scanner: indented preprocessor") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "  #define FOO");
    CHECK(has_token(tokens, TokenType::Preprocessor, 2, 13));
}

// ===================================================================
// Multi-line state
// ===================================================================

TEST_CASE("Scanner: block comment opens across lines") {
    TestFixture f;
    auto [t1, exit1] = scan(f.hl, "x = 1; /* start");
    CHECK(exit1 == LineState::InBlockComment);
    CHECK(has_token(t1, TokenType::Comment, 7, 15));

    auto [t2, exit2] = scan(f.hl, "still comment", LineState::InBlockComment);
    CHECK(exit2 == LineState::InBlockComment);
    CHECK(has_token(t2, TokenType::Comment, 0, 13));

    auto [t3, exit3] = scan(f.hl, "end */ x = 2;", LineState::InBlockComment);
    CHECK(exit3 == LineState::Normal);
    CHECK(has_token(t3, TokenType::Comment, 0, 6));
}

TEST_CASE("Multi-line state management via decorate()") {
    TempFile file("int x; /* start\ncomment line\nend */ int y;\n", ".cpp");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    SyntaxHighlighter hl(ctrl);
    hl.set_language(LanguageDef::cpp());

    auto d0 = hl.decorate(0);
    // Line 0 should have Type(int), Comment(/* start)
    bool found_type = false, found_comment = false;
    for (const auto& s : d0.spans) {
        if (s.byte_start == 0 && s.byte_end == 3) found_type = true;
        if (s.style.fg.r == 106) found_comment = true;  // comment color
    }
    CHECK(found_type);
    CHECK(found_comment);

    auto d1 = hl.decorate(1);
    CHECK(d1.spans.size() == 1);  // entire line is comment
    CHECK(d1.spans[0].byte_start == 0);

    auto d2 = hl.decorate(2);
    // Should have comment (end */) and type (int)
    CHECK(d2.spans.size() >= 2);
}

// ===================================================================
// State invalidation
// ===================================================================

TEST_CASE("State invalidation on edit") {
    TempFile file("int a;\nint b;\nint c;\n", ".cpp");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    auto hl = std::make_shared<SyntaxHighlighter>(ctrl);
    hl->set_language(LanguageDef::cpp());
    ctrl.add_decoration_source(hl);

    // Force states to be computed
    auto d2 = ctrl.decorations(2);
    CHECK(!d2.spans.empty());

    // Now insert /* on line 0 to open a block comment
    // Line 0 is "int a;" — insert "/*" at end
    ctrl.insert(0, 6, " /*");

    // Line 1 should now be in a block comment
    auto d1 = ctrl.decorations(1);
    bool all_comment = !d1.spans.empty();
    for (const auto& s : d1.spans) {
        if (s.style.fg.r != 106) all_comment = false;  // comment gray
    }
    CHECK(all_comment);
}

// ===================================================================
// Integration: Controller + SyntaxHighlighter
// ===================================================================

TEST_CASE("Integration: controller.decorations() returns styled spans") {
    TempFile file("int main() { return 0; }\n", ".cpp");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    auto hl = std::make_shared<SyntaxHighlighter>(ctrl);
    hl->set_language(LanguageDef::cpp());
    ctrl.add_decoration_source(hl);

    auto deco = ctrl.decorations(0);
    CHECK(!deco.spans.empty());

    // Check "int" is styled as Type (teal: r=86)
    bool found_int = false;
    for (const auto& s : deco.spans) {
        if (s.byte_start == 0 && s.byte_end == 3 && s.style.fg.r == 86) {
            found_int = true;
        }
    }
    CHECK(found_int);

    // Check "return" is styled as Keyword (purple: r=198)
    bool found_return = false;
    for (const auto& s : deco.spans) {
        if (s.byte_start == 13 && s.byte_end == 19 && s.style.fg.r == 198) {
            found_return = true;
        }
    }
    CHECK(found_return);

    // Check "0" is styled as Number (orange: r=209)
    bool found_number = false;
    for (const auto& s : deco.spans) {
        if (s.byte_start == 20 && s.byte_end == 21 && s.style.fg.r == 209) {
            found_number = true;
        }
    }
    CHECK(found_number);
}

// ===================================================================
// Language detection
// ===================================================================

TEST_CASE("Language detection: C++ extensions") {
    Document doc;
    Controller ctrl(doc);
    SyntaxHighlighter hl(ctrl);

    hl.detect_language("test.cpp");
    auto r1 = hl.scan_line("int x;", LineState::Normal);
    CHECK(!r1.tokens.empty());  // active

    SyntaxHighlighter hl2(ctrl);
    hl2.detect_language("test.h");
    auto r2 = hl2.scan_line("int x;", LineState::Normal);
    CHECK(!r2.tokens.empty());  // active
}

TEST_CASE("Language detection: unknown extension") {
    Document doc;
    Controller ctrl(doc);
    SyntaxHighlighter hl(ctrl);

    hl.detect_language("readme.txt");
    auto deco = hl.decorate(0);
    CHECK(deco.spans.empty());  // inactive
}

TEST_CASE("Language detection: no extension") {
    Document doc;
    Controller ctrl(doc);
    SyntaxHighlighter hl(ctrl);

    hl.detect_language("Makefile");
    auto deco = hl.decorate(0);
    CHECK(deco.spans.empty());
}

// ===================================================================
// Edge cases
// ===================================================================

TEST_CASE("Edge: empty line") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "");
    CHECK(tokens.empty());
    CHECK(exit == LineState::Normal);
}

TEST_CASE("Edge: whitespace-only line") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "   \t  ");
    CHECK(tokens.empty());
}

TEST_CASE("Edge: unterminated string at EOL") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, R"(char* s = "unterminated)");
    CHECK(has_token(tokens, TokenType::StringLiteral, 10, 23));
    CHECK(exit == LineState::Normal);
}

TEST_CASE("Edge: very long identifier") {
    TestFixture f;
    std::string long_id(500, 'a');
    auto [tokens, exit] = scan(f.hl, long_id);
    // Not a keyword/type, so no tokens
    CHECK(tokens.empty());
}

TEST_CASE("Edge: dot-started float") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, ".5f");
    CHECK(has_token(tokens, TokenType::Number, 0, 3));
}

TEST_CASE("Edge: multiple block comments on one line") {
    TestFixture f;
    auto [tokens, exit] = scan(f.hl, "a /* x */ b /* y */ c");
    int comment_count = 0;
    for (const auto& t : tokens)
        if (t.type == TokenType::Comment) ++comment_count;
    CHECK(comment_count == 2);
    CHECK(exit == LineState::Normal);
}
