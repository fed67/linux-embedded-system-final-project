#pragma once

#include <string>
#include <fstream>

namespace logger {

    class Logger {

        std::string logging_path;
        std::fstream fs;

    public:
        Logger(std::string filename);
        Logger(std::string logging_path, std::string filename);

        ~Logger();

        void log(std::string msg);
    };

}