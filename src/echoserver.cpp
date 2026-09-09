#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <csignal>

#define PORT 8080




int main(){
    signal(SIGPIPE, SIG_IGN);
    int opt = 1;
    struct sockaddr_in address;

    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(server_fd < 0){
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);//Assign it to port 8080

    if(::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 3) < 0){
        perror("Listen Failed");
        exit(EXIT_FAILURE);
    }  

    while(true){

        int clientfd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if(clientfd < 0){
            perror("Accept Failure");
            exit(EXIT_FAILURE);
        }

        while(true){

            ssize_t numByte = read(clientfd, buffer, sizeof(buffer));

            if(numByte < 0){
                perror("Read Failed");
                exit(EXIT_FAILURE);
            }
            if(numByte == 0){
                break;
            }

            std::cout.write(buffer, numByte);

            if(write(clientfd, buffer, numByte) < 0){
                perror("write failed");
            }
        }
        close(clientfd);
    }
    
    
    close(server_fd);


    return 0;
}