#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main(){
    int server_fd = socket(AF_INET,SOCK_DGRAM,0);

    if(server_fd == -1){
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if(bind(server_fd,(sockaddr*)&addr,sizeof(addr))){
        perror("bind");
        close(server_fd);
        return 1;
    }

    cout << "UDP server listening on port 9000...\n";

    char buffer[1024];

    while(true){
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        ssize_t n = recvfrom(server_fd, buffer,sizeof(buffer),0,
                    (sockaddr*)&client_addr,&client_len);
        
        if(n < 0){
            if(errno == EINTR) continue;
            perror("recvfrom");
            break;
        }

        cout<<"Received"<< n << "bytes" << endl;

        ssize_t sent = sendto(server_fd,buffer,static_cast<size_t>(n),0,
                            (sockaddr*)&client_addr,client_len);
        
        if(sent < 0){
            if(errno == EINTR) continue;
            perror("sendto");
            break;
        }
    }
    close(server_fd);
    return 0;

}

