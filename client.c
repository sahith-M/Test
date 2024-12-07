#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>

// Buffer and file size constraints
#define MAX_BUFFER_SIZE 100076
#define CHUNK_SIZE 64
#define ENCODE_PORT 14357
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 11122
#define INPUT_FILENAME "input.txt"
#define OUTPUT_FILENAME "receivedVowelCount.txt"
#define FRAME_SIZE 536

// Structure to hold encoding/decoding data
typedef struct {
    char chunkData[CHUNK_SIZE];
    char frameData[FRAME_SIZE];
} DataFrames;



// Function to send data for encoding
void send_for_encoding(char *chunk, char *encodedData) {
    int socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in helperAddress;
    helperAddress.sin_family = AF_INET;
    helperAddress.sin_port = htons(ENCODE_PORT);
    helperAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(socketHandle, (struct sockaddr *)&helperAddress, sizeof(helperAddress)) < 0) {
        perror("Connection to encoder failed");
        exit(1);
    }

    printf("Sending data for encoding to %s:%d...\n", SERVER_IP, ENCODE_PORT);
    send(socketHandle, "E", 1, 0);
    send(socketHandle, chunk, CHUNK_SIZE, 0);

    int receivedBytes = recv(socketHandle, encodedData, FRAME_SIZE - 1, 0);
    if (receivedBytes < 0) {
        perror("Error receiving encoded data");
    } else {
        encodedData[receivedBytes] = '\0';
        printf("Encoding completed. Encoded data received.\n");
    }
    close(socketHandle);
}

// Function to request decoding of data
void request_decoding(char *encodedPacket, char *decodedChunk) {
    int socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (socketHandle < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in helperAddress;
    helperAddress.sin_family = AF_INET;
    helperAddress.sin_port = htons(ENCODE_PORT);
    helperAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(socketHandle, (struct sockaddr *)&helperAddress, sizeof(helperAddress)) < 0) {
        perror("Connection to decoder failed");
        exit(1);
    }

    send(socketHandle, "D", 1, 0);
    send(socketHandle, encodedPacket, FRAME_SIZE, 0);

    recv(socketHandle, decodedChunk, CHUNK_SIZE, 0);
    close(socketHandle);
}

// Function to write content to a file
void write_to_output(const char *filename, const char *data) {
    FILE *fileHandle = fopen(filename, "w");
    if (!fileHandle) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    fprintf(fileHandle, "%s", data);
    fclose(fileHandle);
    printf("Results saved to %s.\n", filename);
}

// Function to load input file data into a buffer
void load_input_data(char *dataBuffer, size_t bufferSize) {
    FILE *fileHandle = fopen(INPUT_FILENAME, "r");
    if (fileHandle == NULL) {
        perror("Failed to open input file");
        exit(1);
    }
    memset(dataBuffer, 0, bufferSize);
    size_t bytesRead = fread(dataBuffer, 1, bufferSize - 1, fileHandle);
    
    if (ferror(fileHandle)) {
        perror("Error reading file");
        fclose(fileHandle);
        exit(1);
    }

    dataBuffer[bytesRead] = '\0';
    fclose(fileHandle);
    printf("Successfully loaded input data from %s.\n", INPUT_FILENAME);
}

int main() {
    char fileContent[MAX_BUFFER_SIZE];
    char serverReply[MAX_BUFFER_SIZE];
    
    load_input_data(fileContent, MAX_BUFFER_SIZE);
    
    int segmentTotal = (strlen(fileContent) + CHUNK_SIZE - 1) / CHUNK_SIZE;
    printf("Splitting data into %d segments for encoding...\n", segmentTotal);

    char encodingBuffer[segmentTotal * FRAME_SIZE];

    for (int i = 0; i < segmentTotal; i++) {
        char tempSegment[CHUNK_SIZE];
        strncpy(tempSegment, fileContent + (i * CHUNK_SIZE), CHUNK_SIZE);
        char encodedSegment[FRAME_SIZE];
        send_for_encoding(tempSegment, encodedSegment);
        memcpy(&encodingBuffer[i * FRAME_SIZE], encodedSegment, FRAME_SIZE);
    }

    int socketMain = socket(AF_INET, SOCK_STREAM, 0);
    if (socketMain < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(socketMain, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection to server failed");
        exit(1);
    }

    printf("Sending encoded data to server on port %d...\n", SERVER_PORT);
    send(socketMain, encodingBuffer, sizeof(encodingBuffer), 0);

    int receivedData = read(socketMain, serverReply, MAX_BUFFER_SIZE);
    if (receivedData > 0) {
        serverReply[receivedData] = '\0';
        printf("Server response received.\n");
    } else {
        perror("No server response");
        close(socketMain);
        exit(EXIT_FAILURE);
    }

    char decodedResponse[(receivedData - 1) / 8 - ((receivedData - 1) / FRAME_SIZE) * 3 + 1];
    decodedResponse[0] = '\0';

    int decodeSegmentCount = (receivedData - 1 + FRAME_SIZE - 1) / FRAME_SIZE;
    printf("Decoding %d segments received from the server...\n", decodeSegmentCount);

    int totalCharCount = 0;
    for (int i = 0; i < decodeSegmentCount; i++) {
        char tempDecodedChunk[CHUNK_SIZE];
        request_decoding(&serverReply[i * FRAME_SIZE], tempDecodedChunk);

        if (i == decodeSegmentCount - 1) {
            totalCharCount += (receivedData - (i * FRAME_SIZE) - 24) / 8;
            memcpy(&decodedResponse[i * CHUNK_SIZE], tempDecodedChunk, (receivedData - 1 - (i * FRAME_SIZE) - 24) / 8);
        } else {
            totalCharCount += CHUNK_SIZE;
            memcpy(&decodedResponse[i * CHUNK_SIZE], tempDecodedChunk, CHUNK_SIZE);
        }
    }

    decodedResponse[totalCharCount] = '\0';
    printf("Decoding complete. Final response:\n%s\n", decodedResponse);

    write_to_output(OUTPUT_FILENAME, decodedResponse);
    close(socketMain);

    return 0;
}
