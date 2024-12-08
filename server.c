#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

// Server Configuration
#define SERVER_PORT 11122
#define BUFFER_CAPACITY 100076
#define FRAME_SIZE 536
#define SEGMENT_SIZE 64
#define MAX_QUEUE_LENGTH 6

// Vowel Indices
#define VOWEL_A 0
#define VOWEL_E 1
#define VOWEL_I 2
#define VOWEL_O 3
#define VOWEL_U 4

// Global Vowel Count Tracker: [A, E, I, O, U]
int vowelCount[5] = {0};

// Structure for Message Queue
typedef struct
{
    char *queueMessages[MAX_QUEUE_LENGTH]; // Message storage
    int front, rear;                       // Queue pointers
    pthread_mutex_t queueLock;             // Lock for thread-safe access
    pthread_cond_t notEmpty, notFull;      // Conditions for empty/full queues
} MessageQueue;

// Declare separate queues for each vowel
MessageQueue vowelQueues[5];

// Declare semaphores for synchronization
sem_t taskSemaphores[6];

// Initializes a message queue by resetting pointers and initializing locks/conditions
void initialize_message_queue(MessageQueue *queue)
{
    queue->front = queue->rear = 0;
    pthread_mutex_init(&queue->queueLock, NULL);
    pthread_cond_init(&queue->notEmpty, NULL);
    pthread_cond_init(&queue->notFull, NULL);
}

// Adds a message to the queue (blocking if full)
void enqueue_message(MessageQueue *queue, const char *message)
{
    // Lock the queue for safe access
    pthread_mutex_lock(&queue->queueLock);

    // Wait if the queue is full
    while ((queue->rear + 1) % MAX_QUEUE_LENGTH == queue->front)
    {
        pthread_cond_wait(&queue->notFull, &queue->queueLock);
    }

    // Add the message to the queue
    queue->queueMessages[queue->rear] = strdup(message);
    queue->rear = (queue->rear + 1) % MAX_QUEUE_LENGTH;

    // Signal that the queue is no longer empty
    pthread_cond_signal(&queue->notEmpty);

    // Unlock the queue after insertion
    pthread_mutex_unlock(&queue->queueLock);
}

// Removes and returns a message from the queue (blocking if empty)
char *dequeue_message(MessageQueue *queue)
{
    // Lock the queue for safe access
    pthread_mutex_lock(&queue->queueLock);

    // Wait if the queue is empty
    while (queue->front == queue->rear)
    {
        pthread_cond_wait(&queue->notEmpty, &queue->queueLock);
    }

    // Remove the message from the front of the queue
    char *message = queue->queueMessages[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_LENGTH;

    // Signal that the queue is no longer full
    pthread_cond_signal(&queue->notFull);

    // Unlock the queue after removal
    pthread_mutex_unlock(&queue->queueLock);

    return message;
}

// Processes 'A' characters from the corresponding queue
void *process_char_A(void *arg)
{
    // Wait for the semaphore signal
    sem_wait(&taskSemaphores[VOWEL_A]);
    printf("Thread A: Processing 'A' characters\n");

    // Retrieve a message from the 'A' queue
    char *message = dequeue_message(&vowelQueues[VOWEL_A]);
    if (!message)
        return NULL;

    // Allocate dynamic memory for processed output
    char *processedMessage = malloc(FRAME_SIZE * sizeof(char));
    int outputIndex = 0;

    // Process each character in the message
    for (int i = 0; message[i] != '\0'; i++)
    {
        // Reallocate memory if the buffer runs out of space
        if (outputIndex >= FRAME_SIZE)
        {
            processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
        }

        // Copy the current character to the processed buffer
        processedMessage[outputIndex++] = message[i];

        // Update vowel count and append it if 'A' or 'a' is found
        if (message[i] == 'a' || message[i] == 'A')
        {
            vowelCount[VOWEL_A]++;
            char countStr[10];
            snprintf(countStr, sizeof(countStr), "%d", vowelCount[VOWEL_A]);

            // Append the updated count to the processed buffer
            for (int j = 0; countStr[j] != '\0'; j++)
            {
                if (outputIndex >= FRAME_SIZE)
                {
                    processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
                }
                processedMessage[outputIndex++] = countStr[j];
            }
        }
    }

    // Null-terminate the processed message and free original memory
    processedMessage[outputIndex] = '\0';
    free(message);

    // Enqueue the processed message into the next queue and signal the next semaphore
    enqueue_message(&vowelQueues[VOWEL_E], processedMessage);
    sem_post(&taskSemaphores[VOWEL_E]);

    return NULL;
}

void *process_char_E(void *arg)
{
    // Wait for the semaphore signal
    sem_wait(&taskSemaphores[VOWEL_E]);
    printf("Thread E: Processing 'E' characters\n");

    // Retrieve a message from the 'E' queue
    char *message = dequeue_message(&vowelQueues[VOWEL_E]);
    if (!message)
        return NULL;

    // Allocate dynamic memory for processed output
    char *processedMessage = malloc(FRAME_SIZE * sizeof(char));
    int outputIndex = 0;

    // Process each character in the message
    for (int i = 0; message[i] != '\0'; i++)
    {
        // Reallocate memory if the buffer runs out of space
        if (outputIndex >= FRAME_SIZE)
        {
            processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
        }

        // Copy the current character to the processed buffer
        processedMessage[outputIndex++] = message[i];

        // Update vowel count and append it if 'E' or 'e' is found
        if (message[i] == 'e' || message[i] == 'E')
        {
            vowelCount[VOWEL_E]++;
            char countStr[10];
            snprintf(countStr, sizeof(countStr), "%d", vowelCount[VOWEL_E]);

            // Append the updated count to the processed buffer
            for (int j = 0; countStr[j] != '\0'; j++)
            {
                if (outputIndex >= FRAME_SIZE)
                {
                    processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
                }
                processedMessage[outputIndex++] = countStr[j];
            }
        }
    }

    // Null-terminate the processed message and free original memory
    processedMessage[outputIndex] = '\0';
    free(message);

    // Enqueue the processed message into the next queue and signal the next semaphore
    enqueue_message(&vowelQueues[VOWEL_I], processedMessage);
    sem_post(&taskSemaphores[VOWEL_I]);

    return NULL;
}

// Processes 'I' characters from the corresponding queue
void *process_char_I(void *arg)
{
    // Wait for the semaphore signal
    sem_wait(&taskSemaphores[VOWEL_I]);
    printf("Thread I: Processing 'I' characters\n");

    // Retrieve a message from the 'I' queue
    char *message = dequeue_message(&vowelQueues[VOWEL_I]);
    if (!message)
        return NULL;

    // Allocate dynamic memory for processed output
    char *processedMessage = malloc(FRAME_SIZE * sizeof(char));
    int outputIndex = 0;

    // Process each character in the message
    for (int i = 0; message[i] != '\0'; i++)
    {
        // Reallocate memory if the buffer runs out of space
        if (outputIndex >= FRAME_SIZE)
        {
            processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
        }

        // Copy the current character to the processed buffer
        processedMessage[outputIndex++] = message[i];

        // Update vowel count and append it if 'I' or 'i' is found
        if (message[i] == 'i' || message[i] == 'I')
        {
            vowelCount[VOWEL_I]++;
            char countStr[10];
            snprintf(countStr, sizeof(countStr), "%d", vowelCount[VOWEL_I]);

            // Append the updated count to the processed buffer
            for (int j = 0; countStr[j] != '\0'; j++)
            {
                if (outputIndex >= FRAME_SIZE)
                {
                    processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
                }
                processedMessage[outputIndex++] = countStr[j];
            }
        }
    }

    // Null-terminate the processed message and free original memory
    processedMessage[outputIndex] = '\0';
    free(message);

    // Enqueue the processed message into the next queue and signal the next semaphore
    enqueue_message(&vowelQueues[VOWEL_O], processedMessage);
    sem_post(&taskSemaphores[VOWEL_O]);

    return NULL;
}

// Processes 'O' characters from the corresponding queue
void *process_char_O(void *arg)
{
    // Wait for the semaphore signal
    sem_wait(&taskSemaphores[VOWEL_O]);
    printf("Thread O: Processing 'O' characters\n");

    // Retrieve a message from the 'O' queue
    char *message = dequeue_message(&vowelQueues[VOWEL_O]);
    if (!message)
        return NULL;

    // Allocate dynamic memory for processed output
    char *processedMessage = malloc(FRAME_SIZE * sizeof(char));
    int outputIndex = 0;

    // Process each character in the message
    for (int i = 0; message[i] != '\0'; i++)
    {
        // Reallocate memory if the buffer runs out of space
        if (outputIndex >= FRAME_SIZE)
        {
            processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
        }

        // Copy the current character to the processed buffer
        processedMessage[outputIndex++] = message[i];

        // Update vowel count and append it if 'O' or 'o' is found
        if (message[i] == 'o' || message[i] == 'O')
        {
            vowelCount[VOWEL_O]++;
            char countStr[10];
            snprintf(countStr, sizeof(countStr), "%d", vowelCount[VOWEL_O]);

            // Append the updated count to the processed buffer
            for (int j = 0; countStr[j] != '\0'; j++)
            {
                if (outputIndex >= FRAME_SIZE)
                {
                    processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
                }
                processedMessage[outputIndex++] = countStr[j];
            }
        }
    }

    // Null-terminate the processed message and free original memory
    processedMessage[outputIndex] = '\0';
    free(message);

    // Enqueue the processed message into the next queue and signal the next semaphore
    enqueue_message(&vowelQueues[VOWEL_U], processedMessage);
    sem_post(&taskSemaphores[VOWEL_U]);

    return NULL;
}

// Processes 'U' characters from the corresponding queue
void *process_char_U(void *arg)
{
    // Wait for the semaphore signal
    sem_wait(&taskSemaphores[VOWEL_U]);
    printf("Thread U: Processing 'U' characters\n");

    // Retrieve a message from the 'U' queue
    char *message = dequeue_message(&vowelQueues[VOWEL_U]);
    if (!message)
        return NULL;

    // Allocate dynamic memory for processed output
    char *processedMessage = malloc(FRAME_SIZE * sizeof(char));
    int outputIndex = 0;

    // Process each character in the message
    for (int i = 0; message[i] != '\0'; i++)
    {
        // Reallocate memory if the buffer runs out of space
        if (outputIndex >= FRAME_SIZE)
        {
            processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
        }

        // Copy the current character to the processed buffer
        processedMessage[outputIndex++] = message[i];

        // Update vowel count and append it if 'U' or 'u' is found
        if (message[i] == 'u' || message[i] == 'U')
        {
            vowelCount[VOWEL_U]++;
            char countStr[10];
            snprintf(countStr, sizeof(countStr), "%d", vowelCount[VOWEL_U]);

            // Append the updated count to the processed buffer
            for (int j = 0; countStr[j] != '\0'; j++)
            {
                if (outputIndex >= FRAME_SIZE)
                {
                    processedMessage = realloc(processedMessage, (outputIndex + FRAME_SIZE) * sizeof(char));
                }
                processedMessage[outputIndex++] = countStr[j];
            }
        }
    }

    // Null-terminate the processed message and free original memory
    processedMessage[outputIndex] = '\0';
    free(message);

    // Enqueue the processed message into the next queue and signal the next semaphore
    enqueue_message(&vowelQueues[5], processedMessage);
    sem_post(&taskSemaphores[5]);

    return NULL;
}

// Sends encoded data to the helper for decoding and retrieves the decoded result
void request_decoding_from_helper(const char *encodedData, char *decodedData, int dataLength)
{
    int helperSocket;
    struct sockaddr_in helperAddr;

    // Create a TCP socket
    if ((helperSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the helper server address
    helperAddr.sin_family = AF_INET;
    helperAddr.sin_port = htons(14357); // Use previously defined constant

    // Convert IP address and check validity
    if (inet_pton(AF_INET, "127.0.0.1", &helperAddr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(helperSocket);
        exit(EXIT_FAILURE);
    }

    // Establish a connection to the helper node
    if (connect(helperSocket, (struct sockaddr *)&helperAddr, sizeof(helperAddr)) < 0)
    {
        perror("Connection to helper node failed");
        close(helperSocket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to helper node\n");

    // Send decoding request
    const char operation[] = "D";
    send(helperSocket, operation, sizeof(operation) - 1, 0);

    // Transmit encoded data
    send(helperSocket, encodedData, dataLength, 0);

    // Receive the decoded result
    int receivedBytes = recv(helperSocket, decodedData, SEGMENT_SIZE, 0);
    if (receivedBytes < 0)
    {
        perror("Failed to receive decoded data");
    }
    else
    {
        decodedData[receivedBytes] = '\0'; // Null-terminate the received data
    }

    // Close the socket after the operation
    close(helperSocket);
}

// Displays and saves the final vowel counts to a file
void *display_and_save_vowel_counts(void *arg)
{
    // Wait for the final semaphore signal
    sem_wait(&taskSemaphores[5]);

    // Display final counts
    printf("Thread F: Final Vowel Counts\n");
    printf("Letter aA: %d\n", vowelCount[VOWEL_A]);
    printf("Letter eE: %d\n", vowelCount[VOWEL_E]);
    printf("Letter iI: %d\n", vowelCount[VOWEL_I]);
    printf("Letter oO: %d\n", vowelCount[VOWEL_O]);
    printf("Letter uU: %d\n", vowelCount[VOWEL_U]);

    // Save the counts to a file
    FILE *file = fopen("vowelCount.txt", "w");
    if (!file)
    {
        perror("File creation failed");
        return NULL;
    }

    fprintf(file, "Final Vowel Counts:\n");
    fprintf(file, "Letter aA: %d\n", vowelCount[VOWEL_A]);
    fprintf(file, "Letter eE: %d\n", vowelCount[VOWEL_E]);
    fprintf(file, "Letter iI: %d\n", vowelCount[VOWEL_I]);
    fprintf(file, "Letter oO: %d\n", vowelCount[VOWEL_O]);
    fprintf(file, "Letter uU: %d\n", vowelCount[VOWEL_U]);

    fclose(file);
    printf("Vowel counts saved to vowelCount.txt\n");

    return NULL;
}

// Sends data to the helper for encoding and retrieves the encoded result
void request_encoding_from_helper(const char *processedData, char *encodedData)
{
    int helperSocket;
    struct sockaddr_in helperAddr;

    // Create a TCP socket
    if ((helperSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the helper server address
    helperAddr.sin_family = AF_INET;
    helperAddr.sin_port = htons(14357);

    // Convert IP address and validate
    if (inet_pton(AF_INET, "127.0.0.1", &helperAddr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(helperSocket);
        exit(EXIT_FAILURE);
    }

    // Establish a connection to the helper node
    if (connect(helperSocket, (struct sockaddr *)&helperAddr, sizeof(helperAddr)) < 0)
    {
        perror("Connection to helper node failed");
        close(helperSocket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to helper node\n");

    // Send encoding request
    const char operation[] = "E";
    send(helperSocket, operation, sizeof(operation) - 1, 0);

    // Transmit data to encode
    send(helperSocket, processedData, SEGMENT_SIZE, 0);

    // Receive the encoded result
    int receivedBytes = recv(helperSocket, encodedData, FRAME_SIZE, 0);
    if (receivedBytes < 0)
    {
        perror("Failed to receive encoded data");
    }
    else
    {
        encodedData[receivedBytes] = '\0'; // Null-terminate the received data
    }

    // Close the socket after the operation
    close(helperSocket);
}

// Handles incoming client requests, processes the data, and sends back results
void *process_client_request(void *arg)
{
    int clientSocket = *(int *)arg;
    char encodedMessage[BUFFER_CAPACITY] = {0};
    char receivedData[BUFFER_CAPACITY] = {0};
    int bytesReceived;

    // Receive content from the client
    bytesReceived = read(clientSocket, encodedMessage, BUFFER_CAPACITY);
    if (bytesReceived <= 0)
    {
        perror("Failed to read client data");
        close(clientSocket);
        pthread_exit(NULL);
    }
    printf("Received content from client\n");

    // Decode the received frames
    int framesReceived = (bytesReceived + FRAME_SIZE - 1) / FRAME_SIZE;
    for (int i = 0; i < framesReceived; i++)
    {
        char tempBuffer[SEGMENT_SIZE];
        int frameLength = (i == framesReceived - 1)
                              ? bytesReceived - (i * FRAME_SIZE)
                              : FRAME_SIZE;

        request_decoding_from_helper(&encodedMessage[i * FRAME_SIZE], tempBuffer, frameLength);
        strncat(receivedData, tempBuffer, strlen(tempBuffer));
    }

    printf("Decoded Content: %s\n", receivedData);

    // Begin vowel counting using parallel threads
    enqueue_message(&vowelQueues[VOWEL_A], receivedData);
    sem_post(&taskSemaphores[VOWEL_A]);

    pthread_t vowelThreads[6];
    pthread_create(&vowelThreads[0], NULL, process_char_A, NULL);
    pthread_create(&vowelThreads[1], NULL, process_char_E, NULL);
    pthread_create(&vowelThreads[2], NULL, process_char_I, NULL);
    pthread_create(&vowelThreads[3], NULL, process_char_O, NULL);
    pthread_create(&vowelThreads[4], NULL, process_char_U, NULL);
    pthread_create(&vowelThreads[5], NULL, display_and_save_vowel_counts, NULL);

    for (int i = 0; i < 6; i++)
    {
        pthread_join(vowelThreads[i], NULL);
    }

    // Save final content to the output file
    char *finalQueueContent = dequeue_message(&vowelQueues[5]);
    FILE *outputFile = fopen("output.txt", "w");
    fprintf(outputFile, "%s\n", finalQueueContent);
    fclose(outputFile);
    free(finalQueueContent);

    // Read vowel count summary
    FILE *fileVowel = fopen("vowelCount.txt", "r");
    char vowelSummary[BUFFER_CAPACITY] = {0};
    fread(vowelSummary, 1, BUFFER_CAPACITY, fileVowel);
    fclose(fileVowel);

    // Encode and prepare the response
    int summaryLength = strlen(vowelSummary);
    int totalChunks = (summaryLength + SEGMENT_SIZE - 1) / SEGMENT_SIZE;
    char encodedResponse[totalChunks * FRAME_SIZE];

    int bitSize = 0;
    for (int i = 0; i < totalChunks; i++)
    {
        char tempSegment[SEGMENT_SIZE] = {0};
        char encodedSegment[FRAME_SIZE] = {0};
        int segmentSize = (i == totalChunks - 1)
                              ? summaryLength - (i * SEGMENT_SIZE)
                              : SEGMENT_SIZE;

        strncpy(tempSegment, vowelSummary + (i * SEGMENT_SIZE), segmentSize);
        request_encoding_from_helper(tempSegment, encodedSegment);

        int frameBitSize = (i == totalChunks - 1)
                               ? 24 + segmentSize * 8
                               : FRAME_SIZE;

        memcpy(&encodedResponse[i * FRAME_SIZE], encodedSegment, frameBitSize);
        bitSize += frameBitSize;
    }

    // Send encoded response to the client
    send(clientSocket, encodedResponse, bitSize, 0);

    close(clientSocket);
    pthread_exit(NULL);
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;

    // Initialize semaphores and queues
    for (int i = 0; i < 6; i++)
    {
        sem_init(&taskSemaphores[i], 0, 0);
    }

    for (int i = 0; i < 5; i++)
    {
        initialize_message_queue(&vowelQueues[i]);
    }

    // Create the server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(serverSocket, 10) < 0)
    {
        perror("Listening failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", SERVER_PORT);

    // Main server loop for accepting and processing client requests
    while (1)
    {
        addrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize);
        if (clientSocket < 0)
        {
            perror("Client connection failed");
            continue;
        }

        printf("Client connected\n");

        // Create a new thread to handle the connected client
        pthread_t clientThread;
        if (pthread_create(&clientThread, NULL, process_client_request, &clientSocket) != 0)
        {
            perror("Failed to create client thread");
            close(clientSocket);
        }
        else
        {
            pthread_detach(clientThread); // Automatically manage thread resources
        }
    }

    // Close the server socket and clean up resources
    close(serverSocket);

    // Destroy semaphores and queue locks
    for (int i = 0; i < 6; i++)
    {
        sem_destroy(&taskSemaphores[i]);
    }

    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_destroy(&vowelQueues[i].queueLock);
        pthread_cond_destroy(&vowelQueues[i].notEmpty);
        pthread_cond_destroy(&vowelQueues[i].notFull);
    }

    return 0;
}
