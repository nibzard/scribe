#pragma once

#include <string>

struct EnvImportResult {
    bool found = false;
    bool ai_key_set = false;
    bool github_token_set = false;
    int keys_found = 0;
    std::string error;
};

// Import API keys/tokens from a .env file without modifying the file.
EnvImportResult importEnvFile(const char* path);
