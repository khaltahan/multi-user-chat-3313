#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

void receive_messages(int client_socket)
{
    char buffer[256];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0)
        {
            std::cout << "It look slike the server is down, we are now disconnecting you from the server!" << std::endl;
            break;
        }

        std::cout << "Message received: " << buffer << std::endl;
    }

    close(client_socket);
    std::cout << "Disconnected from server" << std::endl;
    exit(0);
}

int main()
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        std::cerr << "Error creating client socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);

    if (connect(client_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
    {
        std::cerr << "Oops, it looks like the server you are trying to connect to is ot live!" << std::endl;
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    // Get the user's desired username and send it to the server
    std::string username;
    std::cout << "Enter a username: ";
    std::getline(std::cin, username);
    send(client_socket, username.c_str(), username.length(), 0);

    std::thread receiver_thread(receive_messages, client_socket);
    receiver_thread.detach();

    char buffer[256];
    while (true)
    {
        // std::cout << "Enter a message: ";
        std::cin.getline(buffer, sizeof(buffer));

        if (send(client_socket, buffer, strlen(buffer), 0) < 0)
        {
            std::cerr << "Error sending message" << std::endl;
        }
    }

    close(client_socket);
    return 0;
}