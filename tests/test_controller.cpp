#include <doctest/doctest.h>

#include <sprawn/document.h>
#include <sprawn/middleware/controller.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
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

TEST_CASE("Controller: pass-through read") {
    TempFile file("Hello\nWorld\n");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    CHECK(ctrl.line_count() == 3);
    CHECK(ctrl.line(0) == "Hello");
    CHECK(ctrl.line(1) == "World");
    CHECK(ctrl.line(2) == "");
}

TEST_CASE("Controller: pass-through insert") {
    TempFile file("Hello\nWorld");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    ctrl.insert(0, 5, ", dear");
    CHECK(ctrl.line(0) == "Hello, dear");

    ctrl.insert(1, 0, "Beautiful ");
    CHECK(ctrl.line(1) == "Beautiful World");
}

TEST_CASE("Controller: pass-through erase") {
    TempFile file("Hello, World!");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    ctrl.erase(0, 5, 7);
    CHECK(ctrl.line(0) == "Hello!");
}

TEST_CASE("Controller: error propagation") {
    TempFile file("abc");
    Document doc;
    Controller ctrl(doc);
    ctrl.open_file(file.path());

    CHECK_THROWS(ctrl.line(99));
    CHECK_THROWS(ctrl.erase(0, 0, 100));
    CHECK_THROWS(ctrl.insert(99, 0, "x"));
}

TEST_CASE("Controller: non-copyable and non-movable") {
    static_assert(!std::is_copy_constructible_v<Controller>);
    static_assert(!std::is_copy_assignable_v<Controller>);
    static_assert(!std::is_move_constructible_v<Controller>);
    static_assert(!std::is_move_assignable_v<Controller>);
}
