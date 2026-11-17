#pragma once

#include <filesystem>

namespace je2be {

bool ConvertWorldIcon(std::filesystem::path const &javaWorld, std::filesystem::path const &bedrockWorld);

} // namespace je2be
