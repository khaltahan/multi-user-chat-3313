#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

const std::string PASSWORD = "3313";

void receive_messages(int client_socket) {
    char buffer[256];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            break;
        }

        std::cout << "\nMessage received: " << buffer << std::endl;
    }

    close(client_socket);
    std::cout << "\nDisconnected from server" << std::endl;
    exit(0);
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error creating client socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);

    if (connect(client_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    // Send the password
    send(client_socket, PASSWORD.c_str(), PASSWORD.length(), 0);

    // Get the user's desired username and send it to the server
    std::string username;
    std::cout << "Enter a username: ";
    std::getline(std::cin, username);
    send(client_socket, username.c_str(), username.length(), 0);

    std::thread receiver_thread(receive_messages, client_socket);
    receiver_thread.detach();

    char buffer[256];
    while (true) {
        std::cout << "\nEnter a message: ";
        std::cin.getline(buffer, sizeof(buffer));

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            std::cerr << "Error sending message" << std::endl;
        }
    }

    close(client_socket);
    return 0;
}