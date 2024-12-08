// C libraries needed
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

// Defining constants for file and network configurations
#define BUFFER_LIMIT 100076
#define DATA_SEGMENT_SIZE 64
#define ENCODE_SERVER_PORT 14357
#define MAIN_SERVER_PORT 11122
#define LOCALHOST_IP "127.0.0.1"
#define INPUT_FILE "input.txt"
#define OUTPUT_FILE "result.txt"
#define PACKET_SIZE 536

typedef struct
{
    char segmentData[DATA_SEGMENT_SIZE]; // Holds individual data segments
    char encodedFrame[PACKET_SIZE];      // Stores the encoded data frame
} TransmissionPacket;

void transmit_for_encoding(const char *dataSegment, char *processedData)
{
    // Create a TCP socket
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Define the encoding server's address
    struct sockaddr_in encoderAddr;
    encoderAddr.sin_family = AF_INET;
    encoderAddr.sin_port = htons(ENCODE_SERVER_PORT);
    encoderAddr.sin_addr.s_addr = inet_addr(LOCALHOST_IP);

    // Establish a connection to the encoding server
    if (connect(socketFD, (struct sockaddr *)&encoderAddr, sizeof(encoderAddr)) < 0)
    {
        perror("Connection to encoding server failed");
        exit(EXIT_FAILURE);
    }

    printf("Transmitting data for encoding to %s:%d...\n", LOCALHOST_IP, ENCODE_SERVER_PORT);

    // Send encoding request and data segment
    send(socketFD, "E", 1, 0);
    send(socketFD, dataSegment, DATA_SEGMENT_SIZE, 0);

    // Receive the encoded result
    int bytesReceived = recv(socketFD, processedData, PACKET_SIZE - 1, 0);
    if (bytesReceived < 0)
    {
        perror("Failed to receive encoded data");
    }
    else
    {
        processedData[bytesReceived] = '\0'; // Null-terminate the received string
        printf("Data encoding successful. Received encoded data.\n");
    }

    // Close the socket
    close(socketFD);
}

void initiate_decoding(const char *encodedFrame, char *decodedSegment)
{
    // Create a TCP socket
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Define the decoding server's address
    struct sockaddr_in decoderAddr;
    decoderAddr.sin_family = AF_INET;
    decoderAddr.sin_port = htons(ENCODE_SERVER_PORT);
    decoderAddr.sin_addr.s_addr = inet_addr(LOCALHOST_IP);

    // Establish a connection to the decoding server
    if (connect(socketFD, (struct sockaddr *)&decoderAddr, sizeof(decoderAddr)) < 0)
    {
        perror("Connection to decoding server failed");
        exit(EXIT_FAILURE);
    }

    printf("Requesting data decoding from %s:%d...\n", LOCALHOST_IP, ENCODE_SERVER_PORT);

    // Send decoding request and encoded data frame
    send(socketFD, "D", 1, 0);
    send(socketFD, encodedFrame, PACKET_SIZE, 0);

    // Receive decoded result
    int bytesReceived = recv(socketFD, decodedSegment, DATA_SEGMENT_SIZE, 0);
    if (bytesReceived < 0)
    {
        perror("Failed to receive decoded data");
    }
    else
    {
        decodedSegment[bytesReceived] = '\0'; // Null-terminate the decoded segment
        printf("Decoding completed successfully.\n");
    }

    // Close the socket
    close(socketFD);
}

void save_results_to_file(const char *outputFile, const char *content)
{
    // Open the file in write mode
    FILE *outputStream = fopen(outputFile, "w");
    if (!outputStream)
    {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    // Write the content to the file
    fprintf(outputStream, "%s", content);
    fclose(outputStream);

    printf("Data successfully saved to %s.\n", outputFile);
}

void read_input_file(char *buffer, size_t bufferCapacity)
{
    // Open the input file for reading
    FILE *inputStream = fopen(INPUT_FILE, "r");
    if (inputStream == NULL)
    {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Clear the buffer before reading data
    memset(buffer, 0, bufferCapacity);

    // Read data from the file into the buffer
    size_t bytesRead = fread(buffer, sizeof(char), bufferCapacity - 1, inputStream);

    // Check if file reading encountered an error
    if (ferror(inputStream))
    {
        perror("File read error");
        fclose(inputStream);
        exit(EXIT_FAILURE);
    }

    // Null-terminate the buffer to prevent overflow
    buffer[bytesRead] = '\0';
    fclose(inputStream);

    printf("Input data successfully loaded from %s.\n", INPUT_FILE);
}

int main()
{
    char inputData[BUFFER_LIMIT];      // Buffer to store input file content
    char serverResponse[BUFFER_LIMIT]; // Buffer to store server's reply

    read_input_file(inputData, BUFFER_LIMIT);

    int totalSegments = (strlen(inputData) + DATA_SEGMENT_SIZE - 1) / DATA_SEGMENT_SIZE;
    printf("Preparing %d segments for encoding...\n", totalSegments);

    char encodedDataBuffer[totalSegments * PACKET_SIZE];

    for (int i = 0; i < totalSegments; i++)
    {
        char currentSegment[DATA_SEGMENT_SIZE];
        strncpy(currentSegment, inputData + (i * DATA_SEGMENT_SIZE), DATA_SEGMENT_SIZE);

        char encodedSegment[PACKET_SIZE];
        transmit_for_encoding(currentSegment, encodedSegment);
        memcpy(&encodedDataBuffer[i * PACKET_SIZE], encodedSegment, PACKET_SIZE);
    }

    // Create a socket to connect to the main server
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define the server's address and port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(MAIN_SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(LOCALHOST_IP);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    printf("Transmitting encoded data to server on port %d...\n", MAIN_SERVER_PORT);
    send(clientSocket, encodedDataBuffer, sizeof(encodedDataBuffer), 0);

    // Receive the server's response
    int bytesRead = read(clientSocket, serverResponse, BUFFER_LIMIT);
    if (bytesRead > 0)
    {
        serverResponse[bytesRead] = '\0';
        printf("Received server response.\n");
    }
    else
    {
        perror("No response from server");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    int decodedSize = (bytesRead - 1) / 8 - ((bytesRead - 1) / PACKET_SIZE) * 3 + 1;
    char decodedOutput[decodedSize];
    decodedOutput[0] = '\0';

    int segmentsToDecode = (bytesRead - 1 + PACKET_SIZE - 1) / PACKET_SIZE;
    printf("Decoding %d segments from server response...\n", segmentsToDecode);

    int decodedCharCount = 0;

    // Decode each segment and accumulate the result
    for (int i = 0; i < segmentsToDecode; i++)
    {
        char decodedSegment[DATA_SEGMENT_SIZE];
        initiate_decoding(&serverResponse[i * PACKET_SIZE], decodedSegment);

        if (i == segmentsToDecode - 1)
        {
            int remainingBytes = (bytesRead - 1 - (i * PACKET_SIZE) - 24) / 8;
            decodedCharCount += remainingBytes;
            memcpy(&decodedOutput[i * DATA_SEGMENT_SIZE], decodedSegment, remainingBytes);
        }
        else
        {
            decodedCharCount += DATA_SEGMENT_SIZE;
            memcpy(&decodedOutput[i * DATA_SEGMENT_SIZE], decodedSegment, DATA_SEGMENT_SIZE);
        }
    }

    decodedOutput[decodedCharCount] = '\0';
    printf("Decoding completed. Final output:\n%s\n", decodedOutput);

    // Save decoded results to the output file
    save_results_to_file(OUTPUT_FILE, decodedOutput);
    close(clientSocket);

    return 0;
}
