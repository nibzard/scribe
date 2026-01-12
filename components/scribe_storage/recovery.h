#pragma once

#include <esp_err.h>
#include <string>

// Check if recovery files exist on boot for a project
// Returns true if autosave.tmp or journal files are found
bool checkRecoveryNeeded(const std::string& project_id);

// Read recovered content from autosave.tmp
// Returns empty string if no recovery file exists
std::string readRecoveredContent(const std::string& project_id);

// Get path to recovery journal
std::string getRecoveryJournalPath(const std::string& project_id);

// Get path to autosave temp file
std::string getAutosaveTempPath(const std::string& project_id);
