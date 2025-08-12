#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <cstdint>
#include <format>


namespace logger {

    class Logger {

        std::string logging_path;
        std::fstream fs;

    public:
        Logger(std::string filename);
        Logger(std::string logging_path, std::string filename);

        ~Logger();

        void log(std::string msg) {
            std::time_t result = std::time(nullptr);
            if(this->fs.is_open()) {
                std::string time{std::asctime(std::localtime(&result))};
                time.pop_back();
                this->fs << "Log[" << time << "]: " << msg << "\n";
            } else {
                throw std::runtime_error("Logger Error log file is not open");
            }
        }

        template<typename... T>
        void log(std::string msg, T... t) {
            size_t p = msg.find(std::string("{}"));
            if(p == std::string::npos) {
                throw std::runtime_error("Error in logging: missing {} in msg argument\n");
            }
            
            std::string s = std::vformat(msg, std::make_format_args(t...));
            log(s);
        }    

    };

}