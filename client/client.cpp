#include <iostream>
#include <thread>
#include <cstring>        // For memset, strncpy
#include <unistd.h>       // For close()
#include <netinet/in.h>   // For sockaddr_in, INADDR_ANY, etc.
#include <cstdlib>        // For system()
#include "../protocol/protocol.h" // Include the shared protocol
#include <signal.h> // For kill()


// Function to handle audio recording
void recordAudio() {
    std::cout << "Recording audio... Press ctl+c to stop." << std::endl;
    // Command to record audio using FFmpeg
    system("ffmpeg -f avfoundation -i \":2\" -f s16le -ar 44100 -ac 2 recorded_audio.raw -y > /dev/null 2>&1");
}

// Function to play audio
void playAudio() {
    std::cout << "Playing audio..." << std::endl;
    // Command to convert raw audio to WAV
    system("ffmpeg -f s16le -ar 44100 -ac 2 -i recorded_audio.raw recorded_audio.wav > /dev/null 2>&1");
    // Command to play the WAV audio file
    system("afplay recorded_audio.wav");
    system("rm -f recorded_audio.wav"); // Delete the video after playback
    system("rm -f recorded_audio.raw"); // Delete the video after playback
    std::cout << "Temporary audio file deleted.\n";
}

// Function to handle video recording
void recordVideo() {
    std::cout << "Recording video...press ctr+c to stop" << std::endl;
    // Command to record video using FFmpeg
    system("ffmpeg -f avfoundation -framerate 30 -i \"0:2\" -vcodec libx264 -acodec aac combined_output.mp4 -y > /dev/null 2>&1");
}

// Function to play video
void playVideo() {
    std::cout << "Playing video and audio...close player to stop" << std::endl;

    // Launch ffplay in the foreground (blocking call)
    int result = system("ffplay combined_output.mp4 > /dev/null 2>&1");

    if (result == 0) {
        // ffplay finished, now delete the video file
        if (std::remove("combined_output.mp4") == 0) {
            std::cout << "Temporary video file deleted.\n";
        } else {
            std::cerr << "Error deleting video file or it doesn't exist.\n";
        }
    } else {
        std::cerr << "Error: ffplay encountered an issue.\n";
    }
}

// Function to receive messages from the server
void receiveMessages(int socket_fd) {
    while (true) {
        Packet packet = receivePacket(socket_fd);
        if (std::strlen(packet.header) == 0) break; // If header is empty, server closed the connection

        if (std::strcmp(packet.header, "MSG") == 0) {
            if (std::strlen(packet.recipient) > 0) {
                // Private message
                std::cout << "[Private] " << packet.sender << " to " << packet.recipient << ": " << packet.data << std::endl;
            } else {
                // Broadcast message
                std::cout << packet.sender << ": " << packet.data << std::endl;
            }
        } else if (std::strcmp(packet.header, "AUDIO") == 0) {
            // Play received audio
            playAudio();
        } else if (std::strcmp(packet.header, "VIDEO") == 0) {
            // Play received video
            playVideo();
        }
    }
}

int main() {
    // Prompt for client name
    std::cout << "Enter your name: ";
    std::string client_name;
    std::getline(std::cin, client_name);

    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Configure server address
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to the server" << std::endl;
        close(client_socket);
        return -1;
    }

    std::cout << "Connected to server as " << client_name << std::endl;

    // Send client name to the server for registration
    Packet registration_packet = createPacket("REG", client_name, "");
    sendPacket(client_socket, registration_packet);

    // Start a thread to receive messages
    std::thread(receiveMessages, client_socket).detach();

    // Main loop to handle user input
    while (true) {
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            std::cout << "Message cannot be empty." << std::endl;
            continue;
        }

        if (input == "/audio") {
            // Record and send audio message
            recordAudio();
            Packet audio_packet = createPacket("AUDIO", client_name, "Audio message recorded.");
            sendPacket(client_socket, audio_packet);
            std::cout << "Audio message sent." << std::endl;
        } else if (input == "/video") {
            // Record and send video message
            recordVideo();
            Packet video_packet = createPacket("VIDEO", client_name, "Video message recorded.");
            sendPacket(client_socket, video_packet);
            std::cout << "Video message sent." << std::endl;
        } else if (input.substr(0, 4) == "/msg") {
            // Private message command
            size_t first_space = input.find(' ');
            size_t second_space = input.find(' ', first_space + 1);
            if (first_space != std::string::npos && second_space != std::string::npos) {
                std::string recipient = input.substr(first_space + 1, second_space - first_space - 1);
                std::string message = input.substr(second_space + 1);

                if (recipient.empty() || message.empty()) {
                    std::cout << "Usage: /msg <recipient> <message>" << std::endl;
                    continue;
                }

                Packet packet = createPacket("MSG", client_name, message);
                strncpy(packet.recipient, recipient.c_str(), sizeof(packet.recipient) - 1);
                sendPacket(client_socket, packet);
            } else {
                std::cout << "Usage: /msg <recipient> <message>" << std::endl;
            }
        } else {
            // Broadcast message
            Packet packet = createPacket("MSG", client_name, input);
            sendPacket(client_socket, packet);
        }
    }

    close(client_socket);
    return 0;
}
