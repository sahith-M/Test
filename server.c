#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

#define PORT 11122
#define BUF_SIZE 100076
#define ENCODED_SIZE 536
#define CHAR_SIZE 64
#define MAX_QUEUE_SIZE 6

#define A_IDX 0
#define E_IDX 1
#define I_IDX 2
#define O_IDX 3
#define U_IDX 4

// Global array to track vowel counts: [A, E, I, O, U]
int vowel_stats[5] = {0};

typedef struct {
    char *messages[MAX_QUEUE_SIZE];
    int head, tail;
    pthread_mutex_t lock;
    pthread_cond_t not_empty, not_full;
} Queue;

Queue queues[5];
sem_t semaphores[6];

void initializeQueue(Queue *queue) {
    queue->head = queue->tail = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
}

void pushQueue(Queue *queue, const char *message) {
    pthread_mutex_lock(&queue->lock);
    while ((queue->tail + 1) % MAX_QUEUE_SIZE == queue->head) {
        pthread_cond_wait(&queue->not_full, &queue->lock);
    }
    queue->messages[queue->tail] = strdup(message);
    queue->tail = (queue->tail + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
}

char *popQueue(Queue *queue) {
    pthread_mutex_lock(&queue->lock);
    while (queue->head == queue->tail) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }
    char *message = queue->messages[queue->head];
    queue->head = (queue->head + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
    return message;
}

void *charA(void *arg) {
    sem_wait(&semaphores[0]);
    printf("Thread A: Processing 'A' characters\n");
    char *message = popQueue(&queues[0]);
    char *queueMsgDuplicate = malloc(ENCODED_SIZE * sizeof(char));  // Dynamically allocate memory
    int index = 0;

    for (int i = 0; message[i] != '\0'; i++) {
        if (index >= ENCODED_SIZE) {  // If space is not enough, realloc to double the size
            queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
        }
        queueMsgDuplicate[index++] = message[i];
        if (message[i] == 'a' || message[i] == 'A') {
            vowel_stats[A_IDX]++;
            char count_str[10];
            snprintf(count_str, sizeof(count_str), "%d", vowel_stats[A_IDX]);
            for (int j = 0; count_str[j] != '\0'; j++) {
                if (index >= ENCODED_SIZE) {
                    queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
                }
                queueMsgDuplicate[index++] = count_str[j];
            }
        }
    }
    free(message);
    queueMsgDuplicate[index] = '\0';
    pushQueue(&queues[1], queueMsgDuplicate);
    sem_post(&semaphores[1]);
}

void *charE(void *arg) {
    sem_wait(&semaphores[1]);
    printf("Thread E: Processing 'E' characters\n");
    char *message = popQueue(&queues[1]);
    char *queueMsgDuplicate = malloc(ENCODED_SIZE * sizeof(char));  // Dynamically allocate memory
    int index = 0;

    for (int i = 0; message[i] != '\0'; i++) {
        if (index >= ENCODED_SIZE) {  // If space is not enough, realloc to double the size
            queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
        }
        queueMsgDuplicate[index++] = message[i];
        if (message[i] == 'e' || message[i] == 'E') {
            vowel_stats[E_IDX]++;
            char count_str[10];
            snprintf(count_str, sizeof(count_str), "%d", vowel_stats[E_IDX]);
            for (int j = 0; count_str[j] != '\0'; j++) {
                if (index >= ENCODED_SIZE) {
                    queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
                }
                queueMsgDuplicate[index++] = count_str[j];
            }
        }
    }
    free(message);
    queueMsgDuplicate[index] = '\0';
    pushQueue(&queues[2], queueMsgDuplicate);
    sem_post(&semaphores[2]);
}

void *charI(void *arg) {
    sem_wait(&semaphores[2]);
    printf("Thread I: Processing 'I' characters\n");
    char *message = popQueue(&queues[2]);
    char *queueMsgDuplicate = malloc(ENCODED_SIZE * sizeof(char));  // Dynamically allocate memory
    int index = 0;

    for (int i = 0; message[i] != '\0'; i++) {
        if (index >= ENCODED_SIZE) {  // If space is not enough, realloc to double the size
            queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
        }
        queueMsgDuplicate[index++] = message[i];
        if (message[i] == 'i' || message[i] == 'I') {
            vowel_stats[I_IDX]++;
            char count_str[10];
            snprintf(count_str, sizeof(count_str), "%d", vowel_stats[I_IDX]);
            for (int j = 0; count_str[j] != '\0'; j++) {
                if (index >= ENCODED_SIZE) {
                    queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
                }
                queueMsgDuplicate[index++] = count_str[j];
            }
        }
    }
    free(message);
    queueMsgDuplicate[index] = '\0';
    pushQueue(&queues[3], queueMsgDuplicate);
    sem_post(&semaphores[3]);
}

void *charO(void *arg) {
    sem_wait(&semaphores[3]);
    printf("Thread O: Processing 'O' characters\n");
    char *message = popQueue(&queues[3]);
    char *queueMsgDuplicate = malloc(ENCODED_SIZE * sizeof(char));  // Dynamically allocate memory
    int index = 0;

    for (int i = 0; message[i] != '\0'; i++) {
        if (index >= ENCODED_SIZE) {  // If space is not enough, realloc to double the size
            queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
        }
        queueMsgDuplicate[index++] = message[i];
        if (message[i] == 'o' || message[i] == 'O') {
            vowel_stats[O_IDX]++;
            char count_str[10];
            snprintf(count_str, sizeof(count_str), "%d", vowel_stats[O_IDX]);
            for (int j = 0; count_str[j] != '\0'; j++) {
                if (index >= ENCODED_SIZE) {
                    queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
                }
                queueMsgDuplicate[index++] = count_str[j];
            }
        }
    }

    queueMsgDuplicate[index] = '\0';
    pushQueue(&queues[4], queueMsgDuplicate);
    free(message);
    sem_post(&semaphores[4]);
}

void *charU(void *arg) {
    sem_wait(&semaphores[4]);
    printf("Thread U: Processing 'U' characters\n");
    char *message = popQueue(&queues[4]);
    char *queueMsgDuplicate = malloc(ENCODED_SIZE * sizeof(char));  // Dynamically allocate memory
    int index = 0;

    for (int i = 0; message[i] != '\0'; i++) {
        if (index >= ENCODED_SIZE) {  // If space is not enough, realloc to double the size
            queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
        }
        queueMsgDuplicate[index++] = message[i];
        if (message[i] == 'u' || message[i] == 'U') {
            vowel_stats[U_IDX]++;
            char count_str[10];
            snprintf(count_str, sizeof(count_str), "%d", vowel_stats[U_IDX]);
            for (int j = 0; count_str[j] != '\0'; j++) {
                if (index >= ENCODED_SIZE) {
                    queueMsgDuplicate = realloc(queueMsgDuplicate, (index + ENCODED_SIZE) * sizeof(char));
                }
                queueMsgDuplicate[index++] = count_str[j];
            }
        }
    }
    free(message);
    queueMsgDuplicate[index] = '\0';
    pushQueue(&queues[5], queueMsgDuplicate);
    sem_post(&semaphores[5]);
}

void decodeUsingHelper(char *enc, char *dec, int len) {
    int help_sock_fd;
    struct sockaddr_in helper_addr;
    if ((help_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exit(EXIT_FAILURE);
    }

    helper_addr.sin_family = AF_INET;
    helper_addr.sin_port = htons(14357);

    if (inet_pton(AF_INET, "127.0.0.1", &helper_addr.sin_addr) <= 0) {
        close(help_sock_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(help_sock_fd, (struct sockaddr *)&helper_addr, sizeof(helper_addr)) < 0) {
        close(help_sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to helper node");
    char Ops[] = "D";  

    send(help_sock_fd, Ops, 1, 0);
    send(help_sock_fd, enc, len, 0);
    recv(help_sock_fd, dec, CHAR_SIZE, 0);  
    close(help_sock_fd);
}

void *finalVowelCount(void *arg) {
    sem_wait(&semaphores[5]);
    printf("Thread F: Final Vowel Counts\n");
    printf("Letter aA: %d\n", vowel_stats[A_IDX]);
    printf("Letter eE: %d\n", vowel_stats[E_IDX]);
    printf("Letter iI: %d\n", vowel_stats[I_IDX]);
    printf("Letter oO: %d\n", vowel_stats[O_IDX]);
    printf("Letter uU: %d\n", vowel_stats[U_IDX]);

    FILE *file = fopen("vowelCount.txt", "w");
    fprintf(file, "Totals\n");
    fprintf(file, "Letter aA: %d\n", vowel_stats[A_IDX]);
    fprintf(file, "Letter eE: %d\n", vowel_stats[E_IDX]);
    fprintf(file, "Letter iI: %d\n",vowel_stats[I_IDX]);
    fprintf(file, "Letter oO: %d\n", vowel_stats[O_IDX]);
    fprintf(file, "Letter uU: %d\n", vowel_stats[U_IDX]);
    fclose(file);
   
}

void encodeUsingHelper(char *proc, char *enc) {
    int help_sock_fd;
    struct sockaddr_in helper_addr;
    if ((help_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        exit(EXIT_FAILURE);
    }
    helper_addr.sin_family = AF_INET;
    helper_addr.sin_port = htons(14357);

    if (inet_pton(AF_INET, "127.0.0.1", &helper_addr.sin_addr) <= 0) {
        close(help_sock_fd);
        exit(EXIT_FAILURE);
    }
    if (connect(help_sock_fd, (struct sockaddr *)&helper_addr, sizeof(helper_addr)) < 0) {
        close(help_sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected - helper node succesfully");
    char ops[] = "E";

    send(help_sock_fd, ops, 1, 0);
    send(help_sock_fd, proc, CHAR_SIZE, 0);

    recv(help_sock_fd, enc, ENCODED_SIZE, 0); 
    close(help_sock_fd);
}


void *handleClient(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[ENCODED_SIZE] = {0};
    int bytes_received;
    char encoded_message[BUF_SIZE];
    char decoded_message[BUF_SIZE];
    char processed_message[BUF_SIZE];
    char reencoded_message[BUF_SIZE];

    bytes_received = read(client_socket, encoded_message, BUF_SIZE);
    printf("Received content from client\n");

    char received_data[bytes_received];
    received_data[0] = '\0';

    int frames_received = (bytes_received + ENCODED_SIZE - 1) / ENCODED_SIZE;
    for (int i = 0; i < frames_received; i++) {
        char temp[CHAR_SIZE];
        if (i == frames_received - 1) {
            decodeUsingHelper(&encoded_message[i * ENCODED_SIZE], temp, bytes_received - (i * ENCODED_SIZE));
        } else {
            decodeUsingHelper(&encoded_message[i * ENCODED_SIZE], temp, ENCODED_SIZE);
        }
        strncat(received_data, temp, strlen(temp));
    }

    printf(" Decoded Content: %s\n", received_data);

    pushQueue(&queues[0], received_data);
    sem_post(&semaphores[0]);

    pthread_t threads[6];
    pthread_create(&threads[0], NULL, charA, NULL);
    pthread_create(&threads[1], NULL, charE, NULL);
    pthread_create(&threads[2], NULL, charI, NULL);
    pthread_create(&threads[3], NULL, charO, NULL);
    pthread_create(&threads[4], NULL, charU, NULL);
    pthread_create(&threads[5], NULL, finalVowelCount, NULL);

    for (int i = 0; i < 6; i++) {
        pthread_join(threads[i], NULL);
    }

    char *queueContentfinal = popQueue(&queues[5]);
    FILE *outputFile = fopen("output.txt", "w");
    fprintf(outputFile, "%s\n", queueContentfinal);
    fclose(outputFile);

    free(queueContentfinal);

    FILE *fileVowel = fopen("vowelCount.txt", "r");
    char vowelSummary[BUF_SIZE];
    fread(vowelSummary, 1, BUF_SIZE, fileVowel);
    fclose(fileVowel);

    //write(client_socket, vowelSummary, strlen(vowelSummary));
    int bit_size = 0;
    int chunk = (strlen(vowelSummary) + CHAR_SIZE - 1) / CHAR_SIZE;
    char encodedResponseToTf[chunk * ENCODED_SIZE]; 
    
    for (int i = 0; i < chunk; i++) {
        char buffer1[CHAR_SIZE];
        char encBuffer[ENCODED_SIZE];
        if (i == chunk - 1) {
            strncpy(buffer1, vowelSummary + (i * CHAR_SIZE), strlen(vowelSummary) - (i* CHAR_SIZE));
            buffer1[strlen(vowelSummary) - (i* CHAR_SIZE)] = '\0';
        } else {
            strncpy(buffer1, vowelSummary + (i * CHAR_SIZE), CHAR_SIZE);
        }
       encodeUsingHelper(buffer1, encBuffer);
        if (i == chunk - 1) {
            bit_size += 24 + (strlen(vowelSummary) - (i* CHAR_SIZE)) * 8;
        } else {
            bit_size += ENCODED_SIZE;
        }
        memcpy(&encodedResponseToTf[i * ENCODED_SIZE], encBuffer, bit_size);
        if (i == chunk - 1) {
            encodedResponseToTf[bit_size] = '\0';
        }
      
        
    } 
    char responseClient[bit_size + 1];
    memcpy(responseClient, encodedResponseToTf, bit_size);
    responseClient[bit_size] = '\0';
    send(client_socket, responseClient, bit_size + 1, 0);

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Initialize semaphore and queues
    for (int i = 0; i < 6; i++) {
        sem_init(&semaphores[i], 0, 0);
    }

    for (int i = 0; i < 5; i++) {
        initializeQueue(&queues[i]);
    }

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handleClient, &client_socket);
    }

    close(server_socket);
    for (int i = 0; i < 6; i++) {
        sem_destroy(&semaphores[i]);
    }
    for (int i = 0; i < 5; i++) {
        pthread_mutex_destroy(&queues[i].lock);
        pthread_cond_destroy(&queues[i].not_empty);
        pthread_cond_destroy(&queues[i].not_full);
    }

    return 0;
}

