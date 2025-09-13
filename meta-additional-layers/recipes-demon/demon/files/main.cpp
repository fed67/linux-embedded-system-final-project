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
#include <cstring>

#define LOG_FILE "onewire_log.txt"
#define DRIVER_PATH "/dev/onewire_dev"

#define PORT 1033
#define BUFFER_SIZE 512

volatile sig_atomic_t stop = 0;

int server_fd, new_socket;
struct sockaddr_in server_addr, client_addr;



void handle_signal(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        stop = 1;

        // Close the socket
        close(new_socket);
        close(server_fd);
    }
}

std::vector<char> convert_to_bvec(std::string in) {
	std::vector<char> r;
	r.reserve(in.length());
	
        for( char c : in) {
	        r.push_back( c);
        }
        return r;        
}

std::string write_commands(const std::vector<char>& command, std::string device_name) {
    std::string ret = "";
    std::fstream fs;
    fs.open(device_name, std::fstream::out );


    if( !fs.is_open()) {
        throw std::runtime_error("Error: onewire_driver is not open");
    }

    for(char c : command) {
        fs.put(c);
    }
    fs.close();

    sleep(1);

    fs.open(device_name, std::fstream::in);

    char c;
    while(fs >>  c) {
        ret.push_back(c);
    }
    fs.close(); 
    
    return ret;
}

void create_server(logger::Logger& log) {
    int opt = 1;
    
    
    socklen_t server_addr_len = sizeof(server_addr);
    socklen_t client_addr_len = sizeof(server_addr);
    
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
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Start listening for incoming connections
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    //return ;
    std::cout << "Server listening on port " << PORT << std::endl;
    log.log("Created TCP server");
}

void communicate() {

    socklen_t client_addr_len = sizeof(server_addr);

    // Accept incoming connection
    new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    
    if (new_socket < 0) {
        perror("accept");
        int err = errno;
      	std::cout <<  "Error " << std::strerror(err) << "\n";
        exit(EXIT_FAILURE);
    }
}

std::string write_commands(const std::vector<char>& command, std::vector<char>& result, std::string device_name) {
    std::string ret;
    std::fstream fs;
    fs.open(device_name, std::fstream::out );


    if( !fs.is_open()) {
        throw std::runtime_error("Error: onewire_driver is not open");
    }

    for(char c : command) {
        fs.put(c);
    }
    //fs.flush();
    fs.close();

    sleep(1);

  fs.open(device_name, std::fstream::in);

    char c;
    while(fs.good()) {
        fs >>  c;
        ret.push_back(c);
    }
    fs.close(); 
    
    return ret;
}




void demonize() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }    //return ;


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

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    logger::Logger log(".", "out.txt");

    if(argc > 1) {
        if( std::string(argv[1]).compare("-d") == 0 ){ 
            std::cout << "demonize " << std::endl;
            // demonize();
            return 0;
        } else if( std::string(argv[1]).compare("-m") == 0 ){  //measure temperature
            std::cout << "demonize " << std::endl;
            std::string s = write_commands({'C', 'T'}, DRIVER_PATH);
            std::cout << "Got " << s << '\n';
            s = write_commands({'R', 'S'}, DRIVER_PATH);
            std::cout << "Got " << s << '\n';
            std::cout << "hex ";
            for(size_t i = 0; i < s.size(); ++i) {
                std::cout << std::hex << (int) s[i] << " ";
            }
            std::cout << " end\n";

            return 0;
        }



        std::string argument = "";
        std::stringstream ss;
        std::vector<char> char_arr;

        for(int i = 2; i < argc; i++) {
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
                //ss << std::hex << s2;
                std::cout << "s " << s << " s2 " << s2 << "\n";

                const long l = strtol(s.c_str()+2, NULL, 16);
                char_arr.push_back( (char) l);
                std::cout << "l " << l << std::endl;
                ss << std::hex << l;
            }
        }

        std::cout << "ss " << ss.str()  << "\n";
        std::vector<char> result;
        write_commands(char_arr, result, "/dev/onewire_dev");
        
        for(char c : result ) {
            std::cout << c << " ";
        }
        std::cout << "\n";
    
    } else {
        std::cout << "TCP server \n";
        
        create_server(log);

        std::cout << "Accept server \n";
        communicate();


        char bufa[256];
        const int buf_size = 256;
        //buf.reserve(buf_size);

        while(ssize_t bytes_read = read(new_socket, bufa, buf_size-1)) {


            std::cout << "Reading... \n";
            // ssize_t bytes_read = read(new_socket, bufa, buf_size-1);
            if(bytes_read > 0) {
                bufa[bytes_read] = '\0';
            } else if( bytes_read == 0) {
                std:: cout << "Closing connection" << "\n";
            } else {
                // throw std::runtime_error("Error reading buffer got " + std::to_string(bytes_read) );
            }
            std::string buf;

            for(int i = 0; i < bytes_read; i++) {
                std::cout << bufa[i] << " ";
                buf.push_back(bufa[i]);
            }
            std::cout << "\n";
            
            
            std::cout << "Got " << bytes_read << " " << buf << "\n";
            std::vector<char> char_arr = convert_to_bvec(buf);
            
            
            std::vector<char> result;
            std::string s = write_commands(char_arr, result, "/dev/onewire_dev");
            
            std::cout << "send string " << s << "\n";
            write(new_socket, s.c_str(), s.length());
            //write(new_socket, bufa, bytes_read); 
        }      
        sleep(1);
        
        close(new_socket);
        close(server_fd);
    } 


    return 0;
}

