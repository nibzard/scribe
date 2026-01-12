#pragma once

#include <esp_err.h>
#include <string>
#include <vector>

// Export document to SD card
esp_err_t exportToSD(const std::string& project_path, const std::string& content, const std::string& extension);

// List all exports in the export directory
std::vector<std::string> listExports();

// Delete an export file
esp_err_t deleteExport(const std::string& filename);
