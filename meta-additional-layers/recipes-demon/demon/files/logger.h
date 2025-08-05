#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <cstdint>


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
                this->fs << "Log[" << std::asctime(std::localtime(&result)) << "]: " << msg << "\n";
            } else {
                throw std::runtime_error("Logger Error log file is not open");
            }
        }



        template<typename T0, typename... T>
        void log(std::string msg, T0 t0, T... t) {
            uint32_t pos = -1;
            for(uint32_t i = 0; i < msg.size()-1; i++) {
                if(msg[i] == '{' && msg[i+1] == '}' ) {
                    pos = i;
                    break;
                }
            }
            if(pos > -1) {
                msg.replace(pos, 2, std::to_string(t0));
            }
            log(msg, t...);
        }

    };

}