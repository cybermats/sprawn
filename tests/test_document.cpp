#include <doctest/doctest.h>

#include <sprawn/document.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace sprawn;

namespace {

class TempFile {
public:
    explicit TempFile(const std::string& content) {
        std::string tmpl = (std::filesystem::temp_directory_path() / "sprawn_test_XXXXXX").string();
        int fd = mkstemp(tmpl.data());
        if (fd == -1) throw std::runtime_error("mkstemp failed");
        path_ = tmpl;
        ::write(fd, content.data(), content.size());
        ::close(fd);
    }

    ~TempFile() {
        std::filesystem::remove(path_);
    }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace

TEST_CASE("Document: open and read lines") {
    TempFile file("Hello\nWorld\n");
    Document doc;
    doc.open_file(file.path());

    CHECK(doc.line_count() == 3);
    CHECK(doc.line(0) == "Hello");
    CHECK(doc.line(1) == "World");
    CHECK(doc.line(2) == "");
}

TEST_CASE("Document: single line no trailing newline") {
    TempFile file("Hello, World!");
    Document doc;
    doc.open_file(file.path());

    CHECK(doc.line_count() == 1);
    CHECK(doc.line(0) == "Hello, World!");
}

TEST_CASE("Document: empty file") {
    TempFile file("");
    Document doc;
    doc.open_file(file.path());

    CHECK(doc.line_count() == 1);
    CHECK(doc.line(0) == "");
}

TEST_CASE("Document: insert text") {
    TempFile file("Hello\nWorld");
    Document doc;
    doc.open_file(file.path());

    doc.insert(0, 5, ", dear");
    CHECK(doc.line(0) == "Hello, dear");

    doc.insert(1, 0, "Beautiful ");
    CHECK(doc.line(1) == "Beautiful World");
}

TEST_CASE("Document: insert newline creates new line") {
    TempFile file("HelloWorld");
    Document doc;
    doc.open_file(file.path());

    CHECK(doc.line_count() == 1);

    doc.insert(0, 5, "\n");
    CHECK(doc.line_count() == 2);
    CHECK(doc.line(0) == "Hello");
    CHECK(doc.line(1) == "World");
}

TEST_CASE("Document: erase text") {
    TempFile file("Hello, World!");
    Document doc;
    doc.open_file(file.path());

    doc.erase(0, 5, 7);
    CHECK(doc.line(0) == "Hello!");
}

TEST_CASE("Document: erase newline merges lines") {
    TempFile file("Hello\nWorld");
    Document doc;
    doc.open_file(file.path());

    CHECK(doc.line_count() == 2);

    // Erase the newline at position (0, 5)
    doc.erase(0, 5, 1);
    CHECK(doc.line_count() == 1);
    CHECK(doc.line(0) == "HelloWorld");
}

TEST_CASE("Document: UTF-8 BOM is skipped") {
    std::string content;
    content += '\xEF';
    content += '\xBB';
    content += '\xBF';
    content += "Hello";

    TempFile file(content);
    Document doc;
    doc.open_file(file.path());

    CHECK(doc.line_count() == 1);
    CHECK(doc.line(0) == "Hello");
}

TEST_CASE("Document: non-existent file throws") {
    Document doc;
    CHECK_THROWS(doc.open_file("/tmp/sprawn_nonexistent_file_12345"));
}
