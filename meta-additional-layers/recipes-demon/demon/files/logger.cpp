
#include "logger.h"

using namespace logger;

Logger::Logger(std::string filename) {
    this->fs = std::fstream(this->logging_path + "/" + filename, std::ios::app);
}

Logger::Logger(std::string logging_path, std::string filename) {
    this->fs = std::fstream(logging_path + "/" + filename, std::ios::app);
}

Logger::~Logger() {
    if(this->fs.is_open()) {
        this->fs.close();
    }
}

