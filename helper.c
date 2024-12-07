#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define SERVER_PORT 14357
#define BUFFER_SIZE 536
#define MAX_ENCODED_LENGTH 64
#define MAX_DECODED_LENGTH 536

sem_t sem_convertCharToBin, sem_insertParity, sem_constructFrame, sem_extractFrame, sem_removeParity, sem_convertBinToChar;

char binConversionBuffer[MAX_ENCODED_LENGTH * 7];  
char charConversionBuffer[MAX_ENCODED_LENGTH]; 
char parityInsertBuffer[MAX_ENCODED_LENGTH * 8];  
char parityRemovalBuffer[MAX_ENCODED_LENGTH * 7];
char frameConstructionBuffer[BUFFER_SIZE];    
char frameExtractionBuffer[MAX_DECODED_LENGTH];        

void convertCharToBinary(char c, char *binaryStr);
char convertBinaryToChar(char *binaryStr);
void insertParityBit(char *binaryStr, char *parityBinaryStr);
void removeParityBit(char *parityBinaryStr, char *binaryStr);
void constructFrame(char *inputData, char *frame);
void extractFrame(char *frame, char *outputData);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void convertCharToBinary(char c, char *binaryStr) {
    for (int i = 6; i >= 0; i--) {
        binaryStr[6 - i] = (c & (1 << i)) ? '1' : '0';
    }
}

char convertBinaryToChar(char *binaryStr) {
    char c = 0;
    for (int i = 0; i < 7; i++) {
        c = (c << 1) | (binaryStr[i] - '0');
    }
    return c;
}

void insertParityBit(char *binaryStr, char *parityBinaryStr) {
    int parityCount = 0;
    for (int i = 0; i < 7; i++) {
        if (binaryStr[i] == '1') {
            parityCount++;
        }
    }
    memcpy(parityBinaryStr, binaryStr, 7);
    parityBinaryStr[7] = (parityCount % 2 == 0) ? '0' : '1'; 
}

void removeParityBit(char *parityBinaryStr, char *binaryStr) {
    strncpy(binaryStr, parityBinaryStr, 7);
}

void constructFrame(char *inputData, char *frame) {
    char header[16] = "0001011000010110"; 
    char dataLengthStr[9];                
    int inputLength = strlen(inputData);
    int numChars = inputLength / 8;     

    for (int i = 7; i >= 0; i--) {
        dataLengthStr[7 - i] = (numChars & (1 << i)) ? '1' : '0';
    } 
    memcpy(frame, header, 16);            
    memcpy(frame + 16, dataLengthStr, 8);     
    memcpy(frame + 24, inputData, inputLength);
    frame[24 + inputLength] = '\0';  
}

void extractFrame(char *frame, char *outputData) {
    char dataLengthStr[9] = {0};
    char extractedData[513] = {0};
    strncpy(dataLengthStr, frame + 16, 8);
    int dataLength = 0;
    for (int i = 0; i < 8; i++) {
        dataLength = (dataLength << 1) | (dataLengthStr[i] - '0');
    }
    strncpy(extractedData, frame + 24, 512);
    for (int i = 0; i < dataLength * 8; i++) {
        outputData[i] = extractedData[i];
    }
    outputData[dataLength * 8] = '\0';
}

// Thread handlers renamed accordingly
void *binaryConversionHandler(void *arg) {
    sem_wait(&sem_convertCharToBin);
    char *input = (char *)arg;
    int len = strlen(input) > MAX_ENCODED_LENGTH ? MAX_ENCODED_LENGTH : strlen(input);
    for (int i = 0; i < len; i++) {
        convertCharToBinary(input[i], &binConversionBuffer[i * 7]);
    }
    sem_post(&sem_insertParity);  
    return NULL;
}

void *parityInsertionHandler(void *arg) {
    sem_wait(&sem_insertParity);  
    int len = strlen(binConversionBuffer) / 7;
    for (int i = 0; i < len; i++) {
        insertParityBit(&binConversionBuffer[i * 7], &parityInsertBuffer[i * 8]);
    }
    sem_post(&sem_constructFrame);  
    return NULL;
}

void *frameConstructionHandler(void *arg) {
    sem_wait(&sem_constructFrame);  
    constructFrame(parityInsertBuffer, frameConstructionBuffer);
    return NULL;
}

void *frameExtractionHandler(void *arg) {
    sem_wait(&sem_extractFrame); 
    extractFrame((char *)arg, frameExtractionBuffer);
    sem_post(&sem_removeParity);  
    return NULL;
}

void *parityRemovalHandler(void *arg) {
    sem_wait(&sem_removeParity);  
    int len = strlen(frameExtractionBuffer) / 8;
    for (int i = 0; i < len; i++) {
        removeParityBit(&frameExtractionBuffer[i * 8], &parityRemovalBuffer[i * 7]);
    }
    sem_post(&sem_convertBinToChar); 
    return NULL;
}

void *charConversionHandler(void *arg) {
    sem_wait(&sem_convertBinToChar);  
    int len = strlen(parityRemovalBuffer) / 7;
    for (int i = 0; i < len; i++) {
        charConversionBuffer[i] = convertBinaryToChar(&parityRemovalBuffer[i * 7]);
    }
    charConversionBuffer[len] = '\0';  
    return NULL;
}

void processEncoding(char *input, char *output) {
    pthread_t threads[3];
    sem_post(&sem_convertCharToBin);
    pthread_create(&threads[0], NULL, binaryConversionHandler, (void *)input);  
    pthread_create(&threads[1], NULL, parityInsertionHandler, NULL);         
    pthread_create(&threads[2], NULL, frameConstructionHandler, NULL);     
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    strcpy(output, frameConstructionBuffer);
    output[strlen(frameConstructionBuffer)] = '\0';  
}

void processDecoding(char *input, char *output) {
    pthread_t threads[3];
    sem_post(&sem_extractFrame);
    pthread_create(&threads[0], NULL, frameExtractionHandler, (void *)input);
    pthread_create(&threads[1], NULL, parityRemovalHandler, NULL);
    pthread_create(&threads[2], NULL, charConversionHandler, NULL);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    strcpy(output, charConversionBuffer);
    output[strlen(charConversionBuffer)] = '\0'; 
}

// Main helper handler function for client requests
void *clientRequestHandler(void *arg) {
    int client_socket = *(int *)arg;
    char operationSignal[7];
    char inputBuffer[BUFFER_SIZE];
    char responseBuffer[BUFFER_SIZE];
    
    memset(binConversionBuffer, 0, MAX_ENCODED_LENGTH * 7);
    memset(parityInsertBuffer, 0, MAX_ENCODED_LENGTH * 8);
    memset(frameConstructionBuffer, 0, BUFFER_SIZE);
    memset(frameExtractionBuffer, 0, MAX_DECODED_LENGTH);
    memset(parityRemovalBuffer, 0, MAX_ENCODED_LENGTH * 7);
    memset(charConversionBuffer, 0, MAX_ENCODED_LENGTH);
    memset(responseBuffer, 0, BUFFER_SIZE);
    memset(inputBuffer, 0, BUFFER_SIZE);

    int bytesRead = read(client_socket, operationSignal, 1);
    if (bytesRead <= 0) {
        perror("Failed to read operation signal");
        close(client_socket);
        return NULL;
    }
    operationSignal[1] = '\0';

    while ((bytesRead = read(client_socket, inputBuffer, BUFFER_SIZE)) > 0) {
        if (strcmp(operationSignal, "E") == 0) {
            printf("Encoding initiated\n");
            processEncoding(inputBuffer, responseBuffer);
        } else if (strcmp(operationSignal, "D") == 0) {
            printf("Decoding initiated\n");
            processDecoding(inputBuffer, responseBuffer);
        }
        send(client_socket, responseBuffer, strlen(responseBuffer), 0);
    }

    close(client_socket);
    return NULL;
}

int main() {
    sem_init(&sem_convertCharToBin, 0, 1);
    sem_init(&sem_insertParity, 0, 0);
    sem_init(&sem_constructFrame, 0, 0);
    sem_init(&sem_extractFrame, 0, 1);
    sem_init(&sem_removeParity, 0, 0);
    sem_init(&sem_convertBinToChar, 0, 0);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    printf("Helper listening on port %d\n", SERVER_PORT);
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept");
            exit(EXIT_FAILURE);
        }
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, clientRequestHandler, (void *)&new_socket);
        pthread_detach(client_thread);
    }

    return 0;
}
