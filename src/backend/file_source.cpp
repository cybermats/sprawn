#include "file_source.h"

#include <stdexcept>

namespace sprawn::detail {

FileSource::FileSource(const std::string& path)
    : path_(path)
{
    file_ = std::fopen(path.c_str(), "rb");
    if (!file_) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::fseek(file_, 0, SEEK_END);
    size_ = static_cast<std::size_t>(std::ftell(file_));
    std::fseek(file_, 0, SEEK_SET);
}

FileSource::~FileSource() {
    if (file_) {
        std::fclose(file_);
    }
}

std::size_t FileSource::read(char* buf, std::size_t max) {
    if (!file_ || eof_) return 0;
    std::size_t n = std::fread(buf, 1, max, file_);
    if (n == 0 || std::feof(file_)) {
        eof_ = true;
    }
    return n;
}

bool FileSource::at_end() const {
    return eof_;
}

SourceInfo FileSource::info() const {
    return {path_, size_, true};
}

} // namespace sprawn::detail
