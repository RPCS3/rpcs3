#pragma once

#include "Utilities/File.h"

// Recognizes the container signature without opening a decoder.
bool is_chd_image(const fs::file& file);

// Checks the header without opening a decoder.
bool is_supported_chd_image(fs::file file, const std::string& path);

// Opens an already recognized CHD using the same host handle used for detection.
std::unique_ptr<fs::file_base> open_chd_image(fs::file file, const std::string& path);
