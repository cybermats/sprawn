#include "file_source.h"

namespace sprawn {

FileSource::FileSource(const std::filesystem::path& path)
    : file_(path) {}

std::span<const std::byte> FileSource::data() const {
    return file_.data();
}

} // namespace sprawn
