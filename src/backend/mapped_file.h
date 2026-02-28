#pragma once

#include <cstddef>
#include <string>

namespace sprawn::detail {

class MappedFile {
public:
    explicit MappedFile(const std::string& path);
    ~MappedFile();

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    const char* data() const { return data_; }
    std::size_t size() const { return size_; }

private:
    void unmap();

    const char* data_ = nullptr;
    std::size_t size_ = 0;

#ifdef _WIN32
    void* file_handle_ = nullptr;
    void* mapping_handle_ = nullptr;
#endif
};

} // namespace sprawn::detail
