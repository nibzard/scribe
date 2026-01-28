#pragma once

#include <string>

// Initialize RTC time (call early)
void initTimeSync();

// Start SNTP (call after TCP/IP is ready)
void startSNTP();

// Check if time has been synced
bool isTimeSynced();

// Get current time as ISO 8601 string
std::string getCurrentTimeISO();
