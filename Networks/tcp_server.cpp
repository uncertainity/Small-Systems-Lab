#include <iostream>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

//bind -> accept -> recv -> send

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if(bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1){
        perror("bind");
        return 1;
    }

    if(listen(server_fd,5) == -1){
        perror("socket");
        return 1;
    }

    cout << "Server listening on port 9000...\n";

    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd,reinterpret_cast<sockaddr*>(&client_addr),&addr_len);
    if(client_fd == -1){
        perror("accept");
        return 1;
    }
    
    cout<<"Connected IP:"<< client_addr.sin_port<<endl;
    char buffer[1024];

    while(true){
        ssize_t n = recv(client_fd,buffer,sizeof(buffer),0);

        if(n == 0){
            cout << "No Data is recived. \n";
            break;
        }
        if(n < 0){
            if(errno == EINTR) continue;
            perror("recv");
            break;
        }

        size_t sent = 0;
        while(sent < static_cast<size_t>(n)){
            ssize_t k = send(client_fd,buffer + sent, n - sent, 0);
            if(k < 0 && errno == EINTR) continue;
            if(k <= 0){
                perror("send");
                close(client_fd);
                close(server_fd);
                return 1;
            }
            sent += static_cast<size_t>(k);    
        }
    }

    close(client_fd);
    close(server_fd);
    
}

