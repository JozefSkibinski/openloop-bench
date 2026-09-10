#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <csignal>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>




#define PORT 8080

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/utsname.h>
#endif

std::string get_timestamp() {
    std::time_t now = std::time(nullptr);

    std::tm* tm = std::localtime(&now);

    std::ostringstream oss;

    oss << std::put_time(tm, "%Y-%m-%d|%H:%M:%S");

    return oss.str();
}

std::string get_machine_name() {
        char name[256];

    #if defined(_WIN32) || defined(_WIN64)
        DWORD size = sizeof(name);
        GetComputerNameA(name, &size);

    #elif defined(__linux__) || defined(__APPLE__)
        struct utsname u;
        uname(&u);
        snprintf(name, sizeof(name), "%s", u.nodename);

    #endif
        return name;
}

std::string get_os() {
    #if defined(_WIN32) || defined(_WIN64)
        return "Windows";
    #elif defined(__linux__)
        return "Linux";
    #elif defined(__APPLE__)
        return "macOS";
    #elif defined(__FreeBSD__)
        return "FreeBSD";
    #else
        return "Unknown";
    #endif
}


int main(int argc, char* argv[]){
    std::vector<long long> latency;

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
    if (numIter == 0){exit(EXIT_FAILURE);}
    int msgLen = strlen(argv[2]) + 1;

    ssize_t recRead;
    size_t msgRem;
    ssize_t writeRem;
    ssize_t writeRead;
    latency.reserve(numIter);
    auto startDur = std::chrono::steady_clock::now();
    while(i < numIter){
        auto start = std::chrono::steady_clock::now();
        ssize_t writeLen = write(client_fd, msg, msgLen);
        if(writeLen <= 0){
            perror("write failed");
            exit(EXIT_FAILURE);
        }else{
            while(writeLen < msgLen){
                writeRem = msgLen - writeLen;
                writeRead= write(client_fd, msg + writeLen, writeRem);
                if(writeRead > 0){
                    writeLen += writeRead;
                }else{
                    break;
                }
            }
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
        auto  end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);   

        latency.push_back(ms.count());

        i++;
    }
    auto endDur = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = endDur - startDur;
    
    long long max = 0;
    long long avg = 0;
    long long low = 0;
    long long count = 0;

    for(const auto& time: latency){
        avg += time;
        count++;
    }

    avg = avg/count;
    std::sort(latency.begin(), latency.end());
    low = latency.front();
    max = latency.back();

    int size = latency.size();
    double throughput = size / elapsed.count();

    int p50 = size / 2;
    int p99 = size * 0.99;
    int p999 = size * 0.999;
    int phigh = size * 0.9999;
    int phighest = size * 0.99999;
    
    std::cout << "Average: " << avg << " microseconds" << std::endl 
    << "Lowest: " << low << " microseconds" << std::endl 
    << "Highest: " << max  << " microseconds" << std::endl
    << "Median: " << latency[p50] << " microseconds" << std::endl << std::endl
    << "99th Percentile: " << latency[p99] << " microseconds" << std::endl
    << "99.9th Percentile: " << latency[p999] << " microseconds" << std::endl
    << "99.99th Percentile: " << latency[phigh] << " microseconds" << std::endl
    << "99.999th Percentile: " << latency[phighest] << " microseconds" << std::endl;

    std::ofstream csvFile("results/results.csv", std::ios::app);

    if(csvFile.tellp() == 0){
        if(csvFile.is_open()){
            csvFile <<"timestamp,machine,os,count,msg_bytes,mode,target_rate,stall_ms,stall_every,duration_s,throughput_rps,errors,min_us,p50_us,p99_us,p999_us,p9999_us,p99999_us,max_us,avg_us\n";
        }else{
            std::cerr << "Unable to open file\n";
        }
    }

    if(csvFile.is_open()){
        csvFile << get_timestamp() << "," << get_machine_name() << "," << get_os() << "," << size << "," 
        << msgLen << "," << "" << "," << "" <<  "," << "" <<  "," << "" << "," << elapsed.count() << ","
        << throughput <<  "," << "" << "," << low << "," << latency[p50] << "," << latency[p99] << "," << latency[p999] << ","
        << latency[phigh] << "," << latency[phighest] << "," << max << "," << avg << "\n";
        csvFile.close();
    }else{
        std::cerr << "Unable to open file\n";
    }
    
    

    close(client_fd);

    return 0;
}