#pragma once

#include <string>

// Initialize SNTP time sync
void initTimeSync();

// Check if time has been synced
bool isTimeSynced();

// Get current time as ISO 8601 string
std::string getCurrentTimeISO();
