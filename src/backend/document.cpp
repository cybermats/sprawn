#include <sprawn/document.h>

#include "encoding.h"
#include "file_source.h"
#include "line_index.h"
#include "piece_table.h"

#include <stdexcept>

namespace sprawn {

struct Document::Impl {
    std::unique_ptr<FileSource> source;
    PieceTable table;
    LineIndex index;
    Encoding encoding = Encoding::utf8;

    void rebuild_index() {
        index.rebuild(table);
    }
};

Document::Document() : impl_(std::make_unique<Impl>()) {}
Document::~Document() = default;
Document::Document(Document&&) noexcept = default;
Document& Document::operator=(Document&&) noexcept = default;

void Document::open_file(const std::filesystem::path& path) {
    impl_->source = std::make_unique<FileSource>(path);
    auto raw_data = impl_->source->data();

    auto [data, encoding] = skip_bom(raw_data);
    impl_->encoding = encoding;

    impl_->table = PieceTable(data);
    impl_->rebuild_index();
}

std::string Document::line(size_t line_number) const {
    auto span = impl_->index.line_span(line_number);
    return impl_->table.text(span.offset, span.length);
}

size_t Document::line_count() const {
    return impl_->index.line_count();
}

void Document::insert(size_t line, size_t col, std::string_view text) {
    size_t offset = impl_->index.to_offset(line, col);
    impl_->table.insert(offset, text);
    impl_->rebuild_index();
}

void Document::erase(size_t line, size_t col, size_t count) {
    size_t offset = impl_->index.to_offset(line, col);
    impl_->table.erase(offset, count);
    impl_->rebuild_index();
}

} // namespace sprawn
