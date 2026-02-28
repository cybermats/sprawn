#pragma once

#include <cstddef>
#include <filesystem>
#include <span>

namespace sprawn {

class MappedFile {
public:
    MappedFile() = default;
    explicit MappedFile(const std::filesystem::path& path);
    ~MappedFile();

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    std::span<const std::byte> data() const;
    size_t size() const { return size_; }
#ifdef _WIN32
    bool is_open() const { return file_handle_ != nullptr; }
#else
    bool is_open() const { return fd_ >= 0; }
#endif

private:
    void close();

    std::byte* data_ = nullptr;
    size_t size_ = 0;
#ifdef _WIN32
    void* file_handle_ = nullptr;
    void* mapping_handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

} // namespace sprawn
