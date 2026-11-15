
#include <iostream>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main(){
    int sock = socket(AF_INET,SOCK_STREAM,0);

    if(sock == -1){
        perror("socket");
        return 1;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    //connect to TCP

    if(connect(sock,(sockaddr*)&addr,sizeof(addr)) == -1){
        perror("Connect");
        close(sock);
        return 1;
    }

    //connection established

    string message = "Hello from TCP client.";
    size_t sent = 0;
    while(sent < message.size()){
        ssize_t n = send(sock,message.data() + sent,
                        message.size() - sent,MSG_NOSIGNAL);
        if(n < 0 && errno == EINTR) continue;
        if(n <= 0){
            if(n < 0) perror("send");
            else cerr << "send made no progress.\n";
            close(sock);
            return 1;
        }
        sent += static_cast<size_t>(n);
    }

    char buffer[1024];
    size_t received = 0;

    while(received < message.size()){
        ssize_t n = recv(sock,buffer,sizeof(buffer),0);

        if(n < 0 && errno == EINTR) continue;
        if(n <= 0){
            if(n < 0) perror("recv");
            else cerr << "Server disconnected \n";
            close(sock);
            return 1;
        }
        cout.write(buffer,n);
        received += static_cast<size_t>(n);
    }
    cout<<"\n";
    close(sock);
    return 0;

}


