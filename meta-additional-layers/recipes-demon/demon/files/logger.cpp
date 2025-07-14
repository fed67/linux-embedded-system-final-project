#include <ctime>

#include "logger.h"

using namespace logger;

Logger::Logger(std::string filename) {
    this->fs = std::fstream(this->logging_path + "/" + filename, std::ios::out);
}

Logger::Logger(std::string logging_path, std::string filename) {
    this->fs = std::fstream(logging_path + "/" + filename, std::ios::out);
}

Logger::~Logger() {
    if(this->fs.is_open()) {
        this->fs.close();
    }
}

void Logger::log(std::string msg) {
    std::time_t result = std::time(nullptr);
    if(this->fs.is_open()) {
        this->fs << "Log[" << std::asctime(std::localtime(&result)) << "]: " << msg << "\n";
    } else {
        throw std::runtime_error("Logger Error log file is not open");
    }
}