        #include <iostream>
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
        #include <unistd.h>
        #include <string.h>
        #include <errno.h>
        #include <csignal>
        #include <chrono>

        #define PORT 8080

        int main(int argc, char* argv[]){

            if(argc < 3){
                perror("Not enough arguements");
                exit(EXIT_FAILURE);
            }
            
            signal(SIGPIPE, SIG_IGN);
            struct sockaddr_in address{};

            char buffer[1024] = {0};

            int client_fd = socket(AF_INET, SOCK_STREAM, 0);
            
            if(client_fd < 0){
                perror("Socket Failed");
                exit(EXIT_FAILURE);
            }

            int inetAddr = inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);//Change IP later

            if(inetAddr != 1){
                std::cerr << "Internet Address";
                exit(EXIT_FAILURE);
            }

            address.sin_family = AF_INET;
            address.sin_port = htons(PORT);//Change this later to an argumen

            if(connect(client_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }

            std::string str = argv[2];
            str += '\n';

            const char* msg = str.data();
            
            int i = 0;
            int numIter = atoi(argv[1]);
            int msgLen = strlen(argv[2]) + 1;
            size_t msgRem;
            int recRead;

            while(i < numIter){
                if(write(client_fd, msg, msgLen) <= 0){
                    perror("write failed");
                    exit(EXIT_FAILURE);
                }
                int recLen = read(client_fd, buffer, msgLen);
                if(recLen <= 0){
                    perror("Read failed");
                    exit(EXIT_FAILURE);
                }else{
                    while(recLen < msgLen){
                        msgRem = msgLen - recLen;
                        recRead = read(client_fd, (buffer + recLen), msgRem);
                        if(recRead > 0){
                            recLen += recRead;
                        }else{
                            break;
                        }
                    }
                }
                i++;
            }


            close(client_fd);

            return 0;
        }