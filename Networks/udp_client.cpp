
#include <iostream>
#include <string>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    string message = "Hello from UDP client!";

    // Send one datagram to the server
    ssize_t n = sendto(
        sock,
        message.data(),
        message.size(),
        0,
        (sockaddr*)&server_addr,
        sizeof(server_addr)
    );

    if (n < 0) {
        perror("sendto");
        close(sock);
        return 1;
    }

    if (static_cast<size_t>(n) != message.size()) {
        cerr << "Incomplete datagram send\n";
        close(sock);
        return 1;
    }

    // Receive the echoed datagram
    char buffer[1024];

    ssize_t received;

    do {
        received = recvfrom(
            sock,
            buffer,
            sizeof(buffer),
            0,
            nullptr,
            nullptr
        );
    } while (received < 0 && errno == EINTR);

    if (received < 0) {
        perror("recvfrom");
        close(sock);
        return 1;
    }

    cout << "Echo: ";
    cout.write(buffer, received);
    cout << '\n';

    close(sock);
    return 0;
}
