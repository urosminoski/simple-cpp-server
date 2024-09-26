#include <iostream>
#include <thread>
#include <cstring>    // For memset, strerror
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>   // For close
#include <mutex>
#include <cerrno>   // For errno

#define PORT            ( 8080 )    // Port number for communication
#define BUFFER_SIZE     ( 1024 )    // Buffer size for data for communication

std::mutex cout_mutex;  // Mutex to protect shared resource (cout)

void handle_client(const int client_socket);    // Function to handle communication with a client

int main()
{
    int socket_fd, new_socket;      // Socket file descriptors
    struct sockaddr_in address;     // Stuct that stores host's IP and port addresses
    int addrlen = sizeof(address);

    // Create a socket (IPv4, TCP)
    socket_fd =  socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == 0)
    {
        std::cerr << "Socket creation failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the IP and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(socket_fd, (struct sockaddr*)&address, sizeof(address));

    // Listen for incoming connections (maximum backlog of 3)
    listen(socket_fd, 3);
    std::cout << "Server listening on port " << PORT << std::endl;

    // Accept incoming connections in a loop
    while(true)
    {
        new_socket = accept(socket_fd, (struct sockaddr*)&address, (socklen_t*)&(addrlen));
        if(new_socket < 0)
        {
            std::cerr << "Accept failed!" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Create new thread to handle the client communication
        std::thread client_thred(handle_client, new_socket);
        client_thred.detach();  // Let the thred tun independently
    }

    // Close the server socket
    close(socket_fd);
}

// Function to handle communication with a client
void handle_client(const int client_socket)
{
    char buffer[BUFFER_SIZE] = {0};

    while(true)
    {
        // Blocking call to receive data from the client
        int valread = recv(client_socket, buffer, sizeof(buffer), 0);
        // Check the state of received data
        std::lock_guard<std::mutex> guard(cout_mutex);      // Locking mutex, to protect cout
        if(valread > 0)
        {
            // Null-terminate the buffer to safely print the received data
            buffer[valread] = '\0';
            // Data received
            std::cout << "Received:\n\r" << buffer << std::endl;
        }
        else if(valread == 0)
        {
            // Connection closed by client
            std::cout << "Client disconnected!" << std::endl;
            close(client_socket);
            break;
        }
        else
        {
            // Error occured, check errno
            std::cout << "Error receiving data: " << strerror(errno) << std::endl;
            close(client_socket);
            break;
        }

        // Echo back the received data
        send(client_socket, buffer, valread, 0);
    }
}