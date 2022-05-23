#pragma once

void decrypt_sprx_libraries(std::vector<std::string> modules, std::function<std::string(std::string old_path, std::string path, bool tried)> input_cb);
