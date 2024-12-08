#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <asm-generic/socket.h>

// Server and buffer configurations
#define ENCODER_PORT 14357
#define FRAME_BUFFER_SIZE 536
#define BINARY_SEGMENT_SIZE 64
#define MAX_FRAME_CONTENT 536

// Semaphores for synchronization across encoding/decoding steps
sem_t sem_binaryConversion, sem_parityInsertion, sem_frameConstruction;
sem_t sem_frameExtraction, sem_parityRemoval, sem_charConversion;

// Shared buffers for various processing stages
char binaryBuffer[BINARY_SEGMENT_SIZE * 7];        // Holds binary strings for encoding
char charBuffer[BINARY_SEGMENT_SIZE];              // Holds characters before binary conversion
char parityBuffer[BINARY_SEGMENT_SIZE * 8];        // Holds binary data with parity bits inserted
char parityRemovalBuffer[BINARY_SEGMENT_SIZE * 7]; // Holds binary data after removing parity bits
char frameBuffer[FRAME_BUFFER_SIZE];               // Holds complete encoded frames
char extractedData[MAX_FRAME_CONTENT];             // Holds extracted data after decoding

// Function declarations for encoding and decoding operations
void convert_char_to_binary(char character, char *binaryRepresentation);
char convert_binary_to_char(const char *binaryRepresentation);
void insert_parity_bit(const char *binaryInput, char *parityOutput);
void remove_parity_bit(const char *parityInput, char *binaryOutput);
void build_frame(const char *inputData, char *frame);
void unpack_frame(const char *frame, char *outputData);

// Mutex for thread-safe operations
pthread_mutex_t operationLock = PTHREAD_MUTEX_INITIALIZER;

// Converts a character to its binary representation (7 bits)
void char_to_binary(char character, char *binaryOutput)
{
    for (int bitPosition = 6; bitPosition >= 0; bitPosition--)
    {
        binaryOutput[6 - bitPosition] = (character & (1 << bitPosition)) ? '1' : '0';
    }
}

// Converts a binary string (7 bits) to its corresponding character
char binary_to_char(const char *binaryInput)
{
    char character = 0;
    for (int bitPosition = 0; bitPosition < 7; bitPosition++)
    {
        character = (character << 1) | (binaryInput[bitPosition] - '0');
    }
    return character;
}

// Inserts an even parity bit to create an 8-bit binary string
void add_parity_bit(const char *binaryInput, char *parityOutput)
{
    int oneCount = 0;

    // Count the number of '1' bits in the binary input
    for (int bitPosition = 0; bitPosition < 7; bitPosition++)
    {
        if (binaryInput[bitPosition] == '1')
        {
            oneCount++;
        }
    }

    // Copy original binary and append parity bit
    memcpy(parityOutput, binaryInput, 7);
    parityOutput[7] = (oneCount % 2 == 0) ? '0' : '1'; // Even parity calculation
}

// Removes the parity bit, extracting the original 7-bit binary string
void extract_original_binary(const char *parityInput, char *binaryOutput)
{
    strncpy(binaryOutput, parityInput, 7); // Extract the original 7 bits
}

// Builds a frame by adding a header and data length field
void build_frame(const char *inputData, char *frame)
{
    // Define a fixed header
    const char header[17] = "0001011000010110"; // 16 bits + '\0'
    char dataLengthBinary[9];                   // Stores the binary representation of the data length

    // Calculate number of characters to be framed
    int dataLength = strlen(inputData) / 8;

    // Convert data length to binary (8 bits)
    for (int bitPos = 7; bitPos >= 0; bitPos--)
    {
        dataLengthBinary[7 - bitPos] = (dataLength & (1 << bitPos)) ? '1' : '0';
    }
    dataLengthBinary[8] = '\0'; // Null-terminate

    // Assemble the frame: [Header | Data Length | Actual Data]
    memcpy(frame, header, 16);
    memcpy(frame + 16, dataLengthBinary, 8);
    memcpy(frame + 24, inputData, strlen(inputData));
    frame[24 + strlen(inputData)] = '\0'; // Null-terminate the frame
}

// Extracts data from a frame by removing the header and data length field
void unpack_frame(const char *frame, char *outputData)
{
    char dataLengthBinary[9] = {0};   // Binary representation of the data length
    char extractedContent[513] = {0}; // Holds extracted data before processing

    // Extract the 8-bit data length from the frame
    strncpy(dataLengthBinary, frame + 16, 8);

    // Convert the binary length to an integer
    int dataLength = 0;
    for (int bitPos = 0; bitPos < 8; bitPos++)
    {
        dataLength = (dataLength << 1) | (dataLengthBinary[bitPos] - '0');
    }

    // Extract the data from the frame after the header and data length
    strncpy(extractedContent, frame + 24, 512);

    // Copy only the relevant portion of the extracted data
    for (int i = 0; i < dataLength * 8; i++)
    {
        outputData[i] = extractedContent[i];
    }

    // Null-terminate the extracted output
    outputData[dataLength * 8] = '\0';
}

// Thread handlers renamed accordingly
// Handles character-to-binary conversion using a semaphore lock
void *binary_conversion_task(void *arg)
{
    // Wait for the binary conversion semaphore
    sem_wait(&sem_binaryConversion);

    // Cast input argument to a character array
    char *inputData = (char *)arg;
    int dataLength = strlen(inputData) > BINARY_SEGMENT_SIZE ? BINARY_SEGMENT_SIZE : strlen(inputData);

    // Convert each character to its binary representation
    for (int i = 0; i < dataLength; i++)
    {
        char_to_binary(inputData[i], &binaryBuffer[i * 7]);
    }

    // Signal that parity insertion can proceed
    sem_post(&sem_parityInsertion);
    return NULL;
}

// Handles parity bit insertion using a semaphore lock
void *parity_insertion_task(void *arg)
{
    // Wait for the parity insertion semaphore
    sem_wait(&sem_parityInsertion);

    // Calculate the number of binary blocks to process
    int blockCount = strlen(binaryBuffer) / 7;

    // Insert parity bits for each binary block
    for (int i = 0; i < blockCount; i++)
    {
        add_parity_bit(&binaryBuffer[i * 7], &parityBuffer[i * 8]);
    }

    // Signal that frame construction can proceed
    sem_post(&sem_frameConstruction);
    return NULL;
}

// Handles frame construction after parity insertion
void *frame_construction_task(void *arg)
{
    sem_wait(&sem_frameConstruction);

    // Build the frame from the parity-inserted buffer
    build_frame(parityBuffer, frameBuffer);

    // Signal that frame extraction can proceed (if needed)
    sem_post(&sem_frameExtraction);

    return NULL;
}

// Handles frame extraction from a received frame
void *frame_extraction_task(void *arg)
{
    sem_wait(&sem_frameExtraction);

    // Extract the frame content
    unpack_frame((char *)arg, extractedData);

    // Signal that parity removal can proceed
    sem_post(&sem_parityRemoval);

    return NULL;
}

// Handles parity bit removal after frame extraction
void *parity_removal_task(void *arg)
{
    sem_wait(&sem_parityRemoval);

    // Calculate the number of 8-bit binary blocks in the extracted frame
    int blockCount = strlen(extractedData) / 8;

    // Remove parity bits from each 8-bit binary block
    for (int i = 0; i < blockCount; i++)
    {
        extract_original_binary(&extractedData[i * 8], &parityRemovalBuffer[i * 7]);
    }

    // Signal that binary-to-character conversion can proceed
    sem_post(&sem_charConversion);

    return NULL;
}

// Handles binary-to-character conversion after parity removal
void *binary_to_char_task(void *arg)
{
    sem_wait(&sem_charConversion);

    // Calculate the number of 7-bit binary blocks
    int charCount = strlen(parityRemovalBuffer) / 7;

    // Convert each 7-bit binary block to its corresponding character
    for (int i = 0; i < charCount; i++)
    {
        charBuffer[i] = binary_to_char(&parityRemovalBuffer[i * 7]);
    }
    charBuffer[charCount] = '\0'; // Null-terminate the character buffer

    return NULL;
}

// Manages the encoding process by creating and synchronizing worker threads
void run_encoding_pipeline(const char *inputData, char *encodedOutput)
{
    pthread_t encodingTasks[3];

    // Start the binary conversion process
    sem_post(&sem_binaryConversion);

    // Create threads for encoding pipeline stages
    pthread_create(&encodingTasks[0], NULL, binary_conversion_task, (void *)inputData);
    pthread_create(&encodingTasks[1], NULL, parity_insertion_task, NULL);
    pthread_create(&encodingTasks[2], NULL, frame_construction_task, NULL);

    // Wait for all encoding tasks to finish
    for (int i = 0; i < 3; i++)
    {
        pthread_join(encodingTasks[i], NULL);
    }

    // Copy the final encoded frame to the output buffer
    strcpy(encodedOutput, frameBuffer);
    encodedOutput[strlen(frameBuffer)] = '\0'; // Null-terminate the result
}

// Manages the decoding process by creating and synchronizing worker threads
void run_decoding_pipeline(const char *receivedFrame, char *decodedOutput)
{
    pthread_t decodingTasks[3];

    // Start the frame extraction process
    sem_post(&sem_frameExtraction);

    // Create threads for decoding pipeline stages
    pthread_create(&decodingTasks[0], NULL, frame_extraction_task, (void *)receivedFrame);
    pthread_create(&decodingTasks[1], NULL, parity_removal_task, NULL);
    pthread_create(&decodingTasks[2], NULL, binary_to_char_task, NULL);

    // Wait for all decoding tasks to finish
    for (int i = 0; i < 3; i++)
    {
        pthread_join(decodingTasks[i], NULL);
    }

    // Copy the final decoded result to the output buffer
    strcpy(decodedOutput, charBuffer);
    decodedOutput[strlen(charBuffer)] = '\0'; // Null-terminate the result
}

// Main helper handler function for client requests
// Handles client requests by performing encoding or decoding based on the operation signal
void *client_request_handler(void *arg)
{
    int clientSocket = *(int *)arg;

    // Buffers for handling input and responses
    char operationSignal[2]; // Operation signal buffer ("E" or "D")
    char inputBuffer[FRAME_BUFFER_SIZE];
    char responseBuffer[FRAME_BUFFER_SIZE];

    // Clear all shared buffers to avoid residual data issues
    memset(binaryBuffer, 0, sizeof(binaryBuffer));
    memset(parityBuffer, 0, sizeof(parityBuffer));
    memset(frameBuffer, 0, sizeof(frameBuffer));
    memset(extractedData, 0, sizeof(extractedData));
    memset(parityRemovalBuffer, 0, sizeof(parityRemovalBuffer));
    memset(charBuffer, 0, sizeof(charBuffer));
    memset(inputBuffer, 0, sizeof(inputBuffer));
    memset(responseBuffer, 0, sizeof(responseBuffer));

    // Read the operation signal from the client
    int bytesRead = read(clientSocket, operationSignal, 1);
    if (bytesRead <= 0)
    {
        perror("Failed to read operation signal");
        close(clientSocket);
        return NULL;
    }
    operationSignal[1] = '\0'; // Null-terminate the operation signal

    // Process the client's request
    while ((bytesRead = read(clientSocket, inputBuffer, FRAME_BUFFER_SIZE)) > 0)
    {
        if (strcmp(operationSignal, "E") == 0)
        {
            printf("Encoding initiated\n");
            run_encoding_pipeline(inputBuffer, responseBuffer); // Call encoding pipeline
        }
        else if (strcmp(operationSignal, "D") == 0)
        {
            printf("Decoding initiated\n");
            run_decoding_pipeline(inputBuffer, responseBuffer); // Call decoding pipeline
        }
        else
        {
            fprintf(stderr, "Invalid operation signal received.\n");
            break;
        }

        // Send the processed response back to the client
        send(clientSocket, responseBuffer, strlen(responseBuffer), 0);
    }

    // Close the client socket after processing the request
    close(clientSocket);
    return NULL;
}

int main()
{
    // Initialize semaphores for encoding and decoding tasks
    sem_init(&sem_binaryConversion, 0, 1);
    sem_init(&sem_parityInsertion, 0, 0);
    sem_init(&sem_frameConstruction, 0, 0);
    sem_init(&sem_frameExtraction, 0, 1);
    sem_init(&sem_parityRemoval, 0, 0);
    sem_init(&sem_charConversion, 0, 0);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr;
    int addrLen = sizeof(serverAddr);
    int enableReuse = 1;

    // Create the server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow address reuse
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enableReuse, sizeof(enableReuse)))
    {
        perror("Failed to set socket options");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(ENCODER_PORT);

    // Bind the socket to the specified IP and port
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Socket binding failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming client connections
    if (listen(serverSocket, 3) < 0)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Helper service listening on port %d\n", ENCODER_PORT);

    // Main loop to accept and handle client connections
    while (1)
    {
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&serverAddr, (socklen_t *)&addrLen)) < 0)
        {
            perror("Client connection failed");
            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the client request
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, client_request_handler, (void *)&clientSocket);
        pthread_detach(clientThread); // Automatically reclaim thread resources
    }

    return 0;
}
