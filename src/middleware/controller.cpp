#include <sprawn/middleware/controller.h>
#include <sprawn/document.h>

namespace sprawn {

Controller::Controller(Document& doc) : doc_(doc) {}

std::string Controller::line(size_t line_number) const {
    return doc_.line(line_number);
}

size_t Controller::line_count() const {
    return doc_.line_count();
}

void Controller::insert(size_t line, size_t col, std::string_view text) {
    doc_.insert(line, col, text);
}

void Controller::erase(size_t line, size_t col, size_t count) {
    doc_.erase(line, col, count);
}

} // namespace sprawn
