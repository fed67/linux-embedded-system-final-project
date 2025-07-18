#include <iostream>
#include <fstream>
#include <string>
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <syslog.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <memory>

#include "constants.h"
#include "logger.h"
#include <vector>


#define BUFFER_SIZE 256
#define PORT 1048

volatile sig_atomic_t stop = 0;

int server_fd, new_socket;
struct sockaddr_in address;




void handle_signal(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        stop = 1;

        // Close the socket
        close(new_socket);
        close(server_fd);
    }
}

void create_server() {
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << std::endl;
    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

}

void write_commands(const std::vector<char>& command, std::vector<char>& result, std::string device_name) {
    std::fstream fs(device_name);



    if( !fs.is_open()) {
        throw std::runtime_error("Error: onewire_driver is not open");
    }

    for(char c : command) {
        fs << c;
    }
    fs.flush();
    fs.close();

    sleep(1);

    fs = std::fstream{device_name};

    char c;
    while(fs.good()) {
        fs >>  c;
        std::cout << c << " "; 
    }
    std::cout << "\n";
    fs.close();
}

void demonize() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    pid_t sid = setsid();
    if (sid < 0) {
        perror("Error in setsid");
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
        perror("Error in chdir");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, char* argv[]) {
    std::cout << "Simple demon!" << std::endl;

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);


    if(argc > 1) {
        if( std::string(argv[1]).compare("-d") == 0 ){ 
            demonize();
            return 0;
        }
    } 


    std::string argument = "";
    std::stringstream ss;
    std::vector<char> char_arr;

    for(int i = 1; i < argc; i++) {
        std::string s{argv[i]};

        if(i == 1) {
            char_arr.push_back(s[0]);
            if(s.size() > 1) 
                char_arr.push_back(s[1]);
        }

        if(s[0] == '0' and s[1] == 'x') {

            if(s.length() > 4 and s.length() == 2 ) {
                throw std::runtime_error("Error input must be hex");
            }
            std::string s2(s.begin()+2, s.end());
            ss << std::hex << s2;
            std::cout << "s " << s << " s2 " << s2 << "\n";

            const long l = strtol(s.c_str()+2, NULL, 16);
            char_arr.push_back( (char) l);
            ss << std::hex << l;

        } else {
 
        }

    }

    std::cout << "ss " << ss.str()  << "\n";

    return 0;
}