#include "mapped_file.h"

#include <stdexcept>
#include <utility>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <cstring>
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace sprawn::detail {

#ifdef _WIN32

MappedFile::MappedFile(const std::string& path) {
    // Convert to wide string for CreateFileW.
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0)
        throw std::runtime_error("MappedFile: failed to convert path: " + path);

    std::wstring wpath(static_cast<std::size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), wlen);

    file_handle_ = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle_ == INVALID_HANDLE_VALUE)
        throw std::runtime_error("MappedFile: cannot open file: " + path);

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        throw std::runtime_error("MappedFile: cannot get file size: " + path);
    }

    size_ = static_cast<std::size_t>(file_size.QuadPart);
    if (size_ == 0) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        return; // Empty file — data_ stays nullptr.
    }

    mapping_handle_ = CreateFileMappingW(file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping_handle_) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        throw std::runtime_error("MappedFile: CreateFileMapping failed: " + path);
    }

    data_ = static_cast<const char*>(MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0));
    if (!data_) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = nullptr;
        file_handle_ = nullptr;
        throw std::runtime_error("MappedFile: MapViewOfFile failed: " + path);
    }
}

void MappedFile::unmap() {
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
    size_ = 0;
}

#else // POSIX

MappedFile::MappedFile(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        throw std::runtime_error("MappedFile: cannot open file: " + path +
                                 " (" + std::strerror(errno) + ")");

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        throw std::runtime_error("MappedFile: fstat failed: " + path);
    }

    size_ = static_cast<std::size_t>(st.st_size);
    if (size_ == 0) {
        close(fd);
        return; // Empty file — data_ stays nullptr.
    }

    void* mapped = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd); // fd is no longer needed after mmap.

    if (mapped == MAP_FAILED)
        throw std::runtime_error("MappedFile: mmap failed: " + path +
                                 " (" + std::strerror(errno) + ")");

    data_ = static_cast<const char*>(mapped);
}

void MappedFile::unmap() {
    if (data_) {
        munmap(const_cast<char*>(data_), size_);
        data_ = nullptr;
    }
    size_ = 0;
}

#endif // _WIN32

MappedFile::~MappedFile() {
    unmap();
}

MappedFile::MappedFile(MappedFile&& other) noexcept
    : data_(other.data_)
    , size_(other.size_)
#ifdef _WIN32
    , file_handle_(other.file_handle_)
    , mapping_handle_(other.mapping_handle_)
#endif
{
    other.data_ = nullptr;
    other.size_ = 0;
#ifdef _WIN32
    other.file_handle_ = nullptr;
    other.mapping_handle_ = nullptr;
#endif
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        unmap();
        data_ = other.data_;
        size_ = other.size_;
#ifdef _WIN32
        file_handle_ = other.file_handle_;
        mapping_handle_ = other.mapping_handle_;
        other.file_handle_ = nullptr;
        other.mapping_handle_ = nullptr;
#endif
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

} // namespace sprawn::detail
