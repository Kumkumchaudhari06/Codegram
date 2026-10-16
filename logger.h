#pragma once
#include <string>

namespace Logger {
    // Set the log file. Call once, for example init("testrepo/.codegram/logs/codegram.log")
    void init(const std::string& logFilePath);

    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
}