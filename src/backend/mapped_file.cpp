#include "mapped_file.h"

#include <stdexcept>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace sprawn {

MappedFile::MappedFile(const std::filesystem::path& path) {
#ifdef _WIN32
    file_handle_ = CreateFileW(
        path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        file_handle_ = nullptr;
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        throw std::runtime_error("Failed to get file size: " + path.string());
    }
    size_ = static_cast<size_t>(file_size.QuadPart);

    if (size_ == 0) {
        // Empty file — valid but nothing to map
        return;
    }

    mapping_handle_ = CreateFileMappingW(
        file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping_handle_) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        throw std::runtime_error("Failed to create file mapping: " + path.string());
    }

    data_ = static_cast<std::byte*>(
        MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0));
    if (!data_) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        throw std::runtime_error("Failed to map file: " + path.string());
    }
#else
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    struct stat st{};
    if (fstat(fd_, &st) < 0) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("Failed to stat file: " + path.string());
    }
    size_ = static_cast<size_t>(st.st_size);

    if (size_ == 0) {
        ::close(fd_);
        fd_ = -1;
        return;
    }

    void* mapped = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapped == MAP_FAILED) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("Failed to mmap file: " + path.string());
    }
    data_ = static_cast<std::byte*>(mapped);
    ::close(fd_);
    fd_ = -1;
#endif
}

MappedFile::~MappedFile() {
    close();
}

MappedFile::MappedFile(MappedFile&& other) noexcept
    : data_(std::exchange(other.data_, nullptr))
    , size_(std::exchange(other.size_, 0))
#ifdef _WIN32
    , file_handle_(std::exchange(other.file_handle_, nullptr))
    , mapping_handle_(std::exchange(other.mapping_handle_, nullptr))
#else
    , fd_(std::exchange(other.fd_, -1))
#endif
{
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        close();
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
#ifdef _WIN32
        file_handle_ = std::exchange(other.file_handle_, nullptr);
        mapping_handle_ = std::exchange(other.mapping_handle_, nullptr);
#else
        fd_ = std::exchange(other.fd_, -1);
#endif
    }
    return *this;
}

std::span<const std::byte> MappedFile::data() const {
    return {data_, size_};
}

void MappedFile::close() {
#ifdef _WIN32
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
    }
    if (file_handle_) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
    }
#else
    if (data_) {
        ::munmap(data_, size_);
        data_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    size_ = 0;
}

} // namespace sprawn
