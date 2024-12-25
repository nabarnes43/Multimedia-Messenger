#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstring>        // For memset
#include <sys/socket.h>   // For socket, bind, listen, accept
#include <netinet/in.h>   // For sockaddr_in, INADDR_ANY
#include <unistd.h>       // For close
#include "../protocol/protocol.h" // Shared protocol
#include <algorithm>
#include <deque>

// Global variables
std::vector<int> clients; // List of client sockets
std::unordered_map<std::string, int> clientSockets; // Map of client names to sockets
std::mutex clients_mutex;

// Maximum number of messages to store in history
const int MAX_CHAT_HISTORY = 100;

// Deque to store chat history
std::deque<Packet> chatHistory;

// Function to add a message to the chat history
void addToChatHistory(const Packet& packet) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (chatHistory.size() >= MAX_CHAT_HISTORY) {
        chatHistory.pop_front(); // Remove oldest message
    }
    chatHistory.push_back(packet);
}

// Function to send chat history to a new client
void sendChatHistory(int client_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const Packet& packet : chatHistory) {
        sendPacket(client_socket, packet);
    }
}

// Function to broadcast a packet to all clients except the sender
void broadcastPacket(const Packet& packet, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client : clients) {
        if (client != sender_socket) {
            sendPacket(client, packet);
        }
    }
}

// Function to handle private messages
void sendPrivateMessage(const Packet& packet) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clientSockets.find(packet.recipient);
    if (it != clientSockets.end()) {
        sendPacket(it->second, packet); // Send to recipient's socket
    } else {
        std::cout << "Recipient not found: " << packet.recipient << std::endl;
    }
}

// Function to handle client connection
void handleClient(int client_socket) {
    std::string client_name;

    // Initial registration: Expect a "REG" packet with the client's name
    Packet registration_packet = receivePacket(client_socket);
    if (std::strcmp(registration_packet.header, "REG") == 0) {
        client_name = registration_packet.sender;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clientSockets[client_name] = client_socket;
            clients.push_back(client_socket);
        }
        std::cout << "Client connected: " << client_name << std::endl;
    } else {
        std::cerr << "Client registration failed." << std::endl;
        close(client_socket);
        return;
    }

    sendChatHistory(client_socket); // Send chat history to the new client

    // Main loop to handle client messages
    while (true) {
        Packet packet = receivePacket(client_socket);

        // If the packet header is empty, the client has disconnected
        if (std::strlen(packet.header) == 0) break;

        // Display the packet details on the server
        if (std::strcmp(packet.header, "AUDIO") == 0) {
            // Handle audio packet
            std::cout << "[AUDIO PACKET]" << std::endl;
            std::cout << "|Sender: " << packet.sender << std::endl;
            broadcastPacket(packet, client_socket); // Broadcast audio packet to all clients
        } else if (std::strcmp(packet.header, "VIDEO") == 0) {
            // Handle video packet
            std::cout << "[VIDEO PACKET]" << std::endl;
            std::cout << "|Sender: " << packet.sender << std::endl;
            broadcastPacket(packet, client_socket); // Broadcast video packet to all clients
        } else if (std::strlen(packet.recipient) > 0) {
            // Private message
            std::cout << "[PRIVATE MESSAGE]" << std::endl;
            std::cout << "|Header: " << packet.header << " |Sender: " << packet.sender
                      << " |Recipient: " << packet.recipient << " |Message: " << packet.data << std::endl;

            sendPrivateMessage(packet); // Send private message
        } else {
            // Broadcast message
            std::cout << "[BROADCAST MESSAGE]" << std::endl;
            std::cout << "|Header: " << packet.header << " |Sender: " << packet.sender
                      << " |Message: " << packet.data << std::endl;

            addToChatHistory(packet); // Add to chat history
            broadcastPacket(packet, client_socket); // Broadcast to all clients
        }
    }

    // Clean up on client disconnect
    close(client_socket);
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
        clientSockets.erase(client_name);
    }
    std::cout << "Client disconnected: " << client_name << std::endl;
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Port 8080
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        return -1;
    }

    std::cout << "Server started on port 8080" << std::endl;

    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Error accepting client connection" << std::endl;
            continue;
        }

        std::thread(handleClient, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
