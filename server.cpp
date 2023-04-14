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
#include <csignal>

std::mutex clients_mutex;

std::map<int,
         std::string>
    clients;

void handle_client(int client_socket)
{
    char
        buffer[256];

    // Receive and store the client's username
    memset(buffer, 0, sizeof(buffer));

    recv(client_socket, buffer, sizeof(buffer), 0);

    std::string username(buffer);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);

        clients[client_socket] = username;

        std::cout << username << " connected" << std::endl;
    }

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received =
            recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0)
        {
            break;
        }

        std::string message = username + ": " + std::string(buffer);

        std::cout << "Message received from " << message << std::endl;

        std::lock_guard<std::mutex> lock(clients_mutex);

        for (const auto &client : clients)
        {
            if (client.first != client_socket)
            {
                send(client.first, message.c_str(), message.length(), 0);
            }
        }
    }

    std::lock_guard<std::mutex> lock(clients_mutex);

    clients.erase(client_socket);

    std::cout << username << " disconnected" << std::endl;

    // Notify other clients that a user has left the chat
    std::string exit_message = username + " has left the chat";

    for (const auto &client : clients)
    {
        send(client.first, exit_message.c_str(), exit_message.length(), 0);
    }

    close(client_socket);
}

void

handle_signal(int signal)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    for (const auto &client : clients)
    {
        close(client.first);
    }

    std::
            cout
        << "Server terminated, all clients disconnected gracefully" << std::endl;

    exit(0);
}

int

main()
{
    // Register the signal handler to handle server termination
    std::signal(SIGINT, handle_signal);

    std::signal(SIGTERM, handle_signal);

    int
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        std::cerr << "Error creating server socket" << std::endl;

        return 1;
    }

    sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_addr.sin_port = htons(8080);

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr),
             sizeof(server_addr)) < 0)
    {
        std::cerr << "Error binding server socket" << std::endl;

        return 1;
    }

    if (listen(server_socket, 10) < 0)
    {
        std::cerr << "Error listening on server socket" << std::endl;

        return 1;
    }

    std::cout << "Server is listening on port 8080" << std::endl;

    while (true)
    {
        sockaddr_in client_addr;

        socklen_t addr_len = sizeof(client_addr);

        int
            client_socket =
                accept(server_socket, reinterpret_cast<sockaddr *>(&client_addr), &addr_len);

        if (client_socket < 0)
        {
            std::cerr << "Error accepting client connection" << std::endl;
        }
        else
        {
            std::thread client_thread(handle_client, client_socket);

            client_thread.detach();
        }
    }

    close(server_socket);

    return 0;
}