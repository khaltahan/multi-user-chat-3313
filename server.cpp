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

int server_socket;

std::mutex clients_mutex;

std::map<int,
         std::string>
    clients;

void handle_client(int client_socket)
{
    char
        buffer[256];

    // Receive and store the client's username
    memset(buffer, 0, sizeof(buffer)); // clear buffer

    recv(client_socket, buffer, sizeof(buffer), 0); // receive username

    std::string username(buffer);

    // close in braces so this is executed as a unit
    {
        std::lock_guard<std::mutex> lock(clients_mutex); // protect clients map from other threads

        clients[client_socket] = username;

        std::cout << username << " connected" << std::endl;
    }

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received =
            recv(client_socket, buffer, sizeof(buffer), 0); // executes only when client sends message

        if (bytes_received <= 0) // runs if client is disconnected
        {
            break;
        }

        std::string message = username + ": " + std::string(buffer);

        std::cout << "Message received from " << message << std::endl;

        std::lock_guard<std::mutex> lock(clients_mutex);

        for (const auto &client : clients) // send message to all clients
        {
            if (client.first != client_socket)
            {
                send(client.first, message.c_str(), message.length(), 0);
            }
        }
    }

    std::lock_guard<std::mutex> lock(clients_mutex);

    clients.erase(client_socket); // get rid of client in map

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
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    std::lock_guard<std::mutex> lock(clients_mutex);

    for (const auto &client : clients)
    {
        close(client.first);
    }

    if (close(server_socket) < 0)
    {
        std::cerr << "Error unbinding server socket" << std::endl;
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
    std::signal(SIGINT, handle_signal); // to handle interrupts

    std::signal(SIGTERM, handle_signal); // to handle termination

    server_socket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET to use IP/port, SOCK_STREAM for send/rcv data any time, 0 to let system choose protocol for socket

    if (server_socket < 0)
    {
        std::cerr << "Error creating server socket" << std::endl;

        return 1;
    }

    sockaddr_in server_addr; // sockaddr_in is built in struct

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY means that the server will bind to all available network interfaces on the machine

    server_addr.sin_port = htons(8080);

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr),
             sizeof(server_addr)) < 0)
    {
        std::cerr << "Error binding server socket" << std::endl;

        return 1;
    }

    if (listen(server_socket, 10) < 0) // also checks that queued connections is less than 10
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
                accept(server_socket, reinterpret_cast<sockaddr *>(&client_addr), &addr_len); // blocks until new connection is made

        if (client_socket < 0)
        {
            std::cerr << "Error accepting client connection" << std::endl;
        }
        else
        {
            // create new thread and detach so main and client thread run independently
            std::thread client_thread(handle_client, client_socket);

            client_thread.detach();
        }
    }

    close(server_socket);

    return 0;
}