#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ctime>

namespace {

std::string g_logPath;   // empty = log to console only

std::string timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

void write(const std::string& level, const std::string& message) {
    std::string line = "[" + timestamp() + "] [" + level + "] " + message;

    // Always print to the console
    std::cout << line << "\n";

    // Also append to the log file, if one was set
    if (!g_logPath.empty()) {
        std::ofstream out(g_logPath, std::ios::app);
        if (out) out << line << "\n";
    }
}

} // namespace

namespace Logger {

void init(const std::string& logFilePath) {
    g_logPath = logFilePath;
    std::filesystem::path p(logFilePath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
}

void info(const std::string& message)  { write("INFO",  message); }
void warn(const std::string& message)  { write("WARN",  message); }
void error(const std::string& message) { write("ERROR", message); }

} // namespace Logger