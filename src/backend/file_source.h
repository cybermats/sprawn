#pragma once

#include <sprawn/source.h>

#include "mapped_file.h"

#include <filesystem>

namespace sprawn {

class FileSource : public Source {
public:
    explicit FileSource(const std::filesystem::path& path);

    std::span<const std::byte> data() const override;

private:
    MappedFile file_;
};

} // namespace sprawn
