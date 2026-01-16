#include "file.hpp"

namespace utils {

std::string read_file(const std::string& path) {

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("file not exists: " + path);
    }

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (f.fail()) {
        throw std::runtime_error("cannout open file: " + path);
    }

    size_t sz = std::filesystem::file_size(path);
    f.seekg(0, std::ios::beg);

    std::string ret(sz, 0);
    if (!f.read(&ret[0], sz)) {
        throw std::runtime_error("cannot read file: " + path);
    }
    return ret;
}
} // namespace utils
