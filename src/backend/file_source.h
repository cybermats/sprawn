#pragma once

#include <sprawn/source.h>

#include <cstdio>
#include <string>

namespace sprawn::detail {

class FileSource : public Source {
public:
    explicit FileSource(const std::string& path);
    ~FileSource() override;

    FileSource(const FileSource&) = delete;
    FileSource& operator=(const FileSource&) = delete;

    std::size_t read(char* buf, std::size_t max) override;
    bool at_end() const override;
    SourceInfo info() const override;

private:
    std::FILE* file_ = nullptr;
    std::string path_;
    std::size_t size_ = 0;
    bool eof_ = false;
};

} // namespace sprawn::detail
