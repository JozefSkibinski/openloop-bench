#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <csignal>
#include <cstdlib>





int main(int argc, char* argv[]){
    int PORT;
    //int stall_ms;
    //int stall_every;
    
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "--port") == 0){
            PORT = atoi(argv[i+1]);
        }
        /*if(strcmp(argv[i], "--stall-ms") == 0){
            stall_ms = atoi(argv[i+1]);
        }
        if(strcmp(argv[i], "--stall-every-sec") == 0){
            stall_every = atoi(argv[i+1]);
        }*/
    }

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

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//sets server fd q

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);//Assign it to port 8080

    if(::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 3) < 0){//listen with backlog 3
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

            std::cout.write(buffer, numByte) << std::flush;

            ssize_t wNumByte = write(clientfd, buffer, numByte);
            if(wNumByte < 0){
                perror("write failed");
            }else{
                while(wNumByte < numByte){//If not enough bytes went through, keep looping through the message and send parts of it until it all sends.
                    ssize_t remain = numByte - wNumByte;
                    ssize_t written = write(clientfd, (buffer + wNumByte), remain);
                    if(written >=0){
                        wNumByte += written;
                    }else if(written < 0){
                        break;
                    }
                } 
            }
            

        }
        close(clientfd);
    }
    close(server_fd);
    return 0;
}