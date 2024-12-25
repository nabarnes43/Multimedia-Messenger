#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <cstring>
#include <sys/socket.h>  // For send() and socket-related functions
#include <unistd.h>      // For close()

// Define the Packet structure
struct Packet {
    char header[10];    // Message type: "MSG", "AUDIO", "VIDEO"
    char sender[50];    // Name of the sender
    char recipient[50]; // Name of the recipient (optional, empty for broadcast)
    char data[1024];    // Message content or payload
};




// Function to create a packet
inline Packet createPacket(const std::string& header, const std::string& sender, const std::string& message) {
    Packet packet;
    std::memset(&packet, 0, sizeof(packet)); // Zero out the packet
    strncpy(packet.header, header.c_str(), sizeof(packet.header) - 1);
    strncpy(packet.sender, sender.c_str(), sizeof(packet.sender) - 1);
    strncpy(packet.data, message.c_str(), sizeof(packet.data) - 1);
    return packet;
}


// Function to send a packet
inline void sendPacket(int socket_fd, const Packet& packet) {
    send(socket_fd, &packet, sizeof(packet), 0);
}

// Function to receive a packet
inline Packet receivePacket(int socket_fd) {
    Packet packet;
    recv(socket_fd, &packet, sizeof(packet), 0);
    return packet;
}

#endif
