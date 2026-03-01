#include <sprawn/middleware/controller.h>
#include <sprawn/document.h>

#include <algorithm>

namespace sprawn {

Controller::Controller(Document& doc) : doc_(doc) {}

void Controller::open_file(const std::filesystem::path& path) {
    doc_.open_file(path);
}

std::string Controller::line(size_t line_number) const {
    return doc_.line(line_number);
}

size_t Controller::line_count() const {
    return doc_.line_count();
}

void Controller::insert(size_t line, size_t col, std::string_view text) {
    doc_.insert(line, col, text);
    for (auto& src : sources_)
        src->on_edit(line, col, text, true);
}

void Controller::erase(size_t line, size_t col, size_t count) {
    doc_.erase(line, col, count);
    for (auto& src : sources_)
        src->on_edit(line, col, std::string_view{}, false);
}

void Controller::add_decoration_source(std::shared_ptr<DecorationSource> source) {
    sources_.push_back(std::move(source));
}

void Controller::remove_decoration_source(std::string_view name) {
    sources_.erase(
        std::remove_if(sources_.begin(), sources_.end(),
                        [&](const auto& s) { return s->name() == name; }),
        sources_.end());
}

LineDecoration Controller::decorations(size_t line_number) const {
    LineDecoration result;
    for (const auto& src : sources_) {
        LineDecoration d = src->decorate(line_number);
        int bp = src->base_priority();
        for (auto& span : d.spans) {
            span.priority += bp;
            result.spans.push_back(std::move(span));
        }
    }
    return result;
}

} // namespace sprawn
