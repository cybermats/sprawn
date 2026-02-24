#include <sprawn/backend.h>

#include "file_source.h"
#include "lorem_ipsum_source.h"
#include "string_array_document.h"

#include <vector>

namespace sprawn {

struct Backend::Impl {
    BackendConfig config;
    std::unique_ptr<Source> source;
    std::unique_ptr<detail::StringArrayDocument> document;
    SourceInfo cached_info;

    void load() {
        document = std::make_unique<detail::StringArrayDocument>();
        cached_info = source->info();

        constexpr std::size_t BUF_SIZE = 64 * 1024;
        std::vector<char> buf(BUF_SIZE);
        std::string partial_line;

        while (!source->at_end()) {
            std::size_t n = source->read(buf.data(), BUF_SIZE);
            for (std::size_t i = 0; i < n; ++i) {
                if (buf[i] == '\n') {
                    document->add_line(std::move(partial_line));
                    partial_line.clear();
                } else if (buf[i] != '\r') {
                    partial_line += buf[i];
                }
            }
        }

        // Add any remaining content as the last line.
        if (!partial_line.empty()) {
            document->add_line(std::move(partial_line));
        }

        document->mark_fully_loaded();
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
    load_from(std::make_unique<detail::FileSource>(path));
}

void Backend::open_lorem_ipsum(std::size_t num_lines) {
    load_from(std::make_unique<detail::LoremIpsumSource>(num_lines));
}

void Backend::load_from(std::unique_ptr<Source> source) {
    impl_->source = std::move(source);
    impl_->load();
}

const Document* Backend::document() const {
    return impl_->document.get();
}

SourceInfo Backend::source_info() const {
    return impl_->source ? impl_->cached_info : SourceInfo{};
}

} // namespace sprawn
