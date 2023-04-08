#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <map>

const std::string PASSWORD = "3313";

std::mutex clients_mutex;
std::map<int, std::string> clients;

void notify_all_clients(const std::string &message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &client : clients) {
        send(client.first, message.c_str(), message.length(), 0);
    }
}

void handle_client(int client_socket) {
    char buffer[256];

    // Verify password
    memset(buffer, 0, sizeof(buffer));
    recv(client_socket, buffer, sizeof(buffer), 0);
    if (buffer != PASSWORD) {
        std::string error_msg = "Invalid password";
        send(client_socket, error_msg.c_str(), error_msg.length(), 0);
        close(client_socket);
        return;
    }

    // Receive and store the client's username
    memset(buffer, 0, sizeof(buffer));
    recv(client_socket, buffer, sizeof(buffer), 0);
    std::string username(buffer);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_socket] = username;
        std::cout << username << " connected" << std::endl;
    }

    std::string welcome_msg = "Welcome " + username + "!";
    send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);

    std::string user_joined_msg = "\n" + username + " has joined the chat\n";
    notify_all_clients(user_joined_msg);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            break;
        }

        std::string message = "\n" + username + ": " + std::string(buffer) + "\n";
        std::cout << "Message received: " << message << std::endl;

        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto &client : clients) {
            if (client.first != client_socket) {
                send(client.first, message.c_str(), message.length(), 0);
            }
        }
    }

    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(client_socket);
    std::cout << username << " disconnected" << std::endl;
    std::string user_left_msg = "\n" + username + " has left the chat\n";
    notify_all_clients(user_left_msg);
    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating server socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Error binding server socket" << std::endl;
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        std::cerr << "Error listening on server socket" << std::endl;
        return 1;
    }

    std::cout << "Server started" << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);

        if (client_socket < 0) {
            std::cerr << "Error accepting client connection" << std::endl;
            continue;
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(server_socket);
    return 0;
}