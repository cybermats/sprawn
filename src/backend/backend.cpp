#include <sprawn/backend.h>

#include "file_source.h"
#include "lorem_ipsum_source.h"
#include "mapped_file.h"
#include "mmap_document.h"
#include "piece_table.h"
#include "piece_table_document.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace sprawn {

struct Backend::Impl {
    BackendConfig config;
    std::unique_ptr<Source> source;
    std::unique_ptr<Document> document;
    SourceInfo cached_info;

    void load_from_source() {
        cached_info = source->info();

        // Read entire source into one contiguous string.
        std::string content;
        constexpr std::size_t BUF_SIZE = 64 * 1024;
        std::vector<char> buf(BUF_SIZE);

        while (!source->at_end()) {
            std::size_t n = source->read(buf.data(), BUF_SIZE);
            content.append(buf.data(), n);
        }

        // Strip \r in-place.
        content.erase(std::remove(content.begin(), content.end(), '\r'),
                      content.end());

        auto table = std::make_shared<detail::PieceTable>(std::move(content));
        auto doc = std::make_unique<detail::PieceTableDocument>(std::move(table));
        doc->mark_fully_loaded();
        document = std::move(doc);
    }
};

Backend::Backend(const BackendConfig& config)
    : impl_(std::make_unique<Impl>())
{
    impl_->config = config;
}

Backend::~Backend() = default;
Backend::Backend(Backend&&) noexcept = default;
Backend& Backend::operator=(Backend&&) noexcept = default;

void Backend::open_file(const std::string& path) {
    auto mapped = std::make_unique<detail::MappedFile>(path);
    impl_->cached_info = SourceInfo{path, mapped->size(), true};
    impl_->document = std::make_unique<detail::MmapDocument>(std::move(mapped));
    impl_->source.reset();
}

void Backend::open_lorem_ipsum(std::size_t num_lines) {
    load_from(std::make_unique<detail::LoremIpsumSource>(num_lines));
}

void Backend::load_from(std::unique_ptr<Source> source) {
    impl_->source = std::move(source);
    impl_->load_from_source();
}

const Document* Backend::document() const {
    return impl_->document.get();
}

SourceInfo Backend::source_info() const {
    return impl_->cached_info;
}

} // namespace sprawn
