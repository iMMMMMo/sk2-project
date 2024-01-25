#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define PORT 1111
#define MAX_DIRECTORIES 3
#define PATH_SIZE 128

struct ThreadArgs {
    int newSocket;
    int directoryIndex;
};  

const char *directories[MAX_DIRECTORIES] = {"testowy1", "testowy2", "testowy3"};

char client_message[BUFFER_SIZE];
char buffer[BUFFER_SIZE];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int client_sockets[MAX_CLIENTS];
int client_directories[MAX_CLIENTS];
int clients_count = 0;
char dirList[PATH_SIZE];
char current_directory[PATH_SIZE];

void sendToAll(char *message, int length, int current_socket, int directoryIndex);
void *socketThread(void *arg);
void handleOption(int option, int clientSocket, int directoryIndex);
void handleAddFile(int clientSocket, const char *fileName, int directoryIndex);
void handleRemoveFile(int clientSocket, const char *fileName, int directoryIndex);
void handleModifyFile(int clientSocket, const char *fileName, int directoryIndex);
void createDirectories(const char *currentDirectory);
int isDirectoryValid(int clientDirectoryIndex);

int main(int argc, char *argv[]) {

    if (getcwd(current_directory, sizeof(current_directory)) != NULL) {
        createDirectories(current_directory);
    } else {
        perror("getcwd error");
        return EXIT_FAILURE;
    }

    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    pthread_t thread_id;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    if (listen(serverSocket, MAX_CLIENTS) == 0)
        printf("Serwer nasłuchuje..\n");
    else
        printf("Error\n");

    int n;
    while (1) {
        addr_size = sizeof serverStorage;
 
        newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);

        struct ThreadArgs *threadArgs = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
        threadArgs->newSocket = newSocket;

        memset(&dirList, 0, sizeof(dirList));
        dirList[0] = '\0';
        for (int i = 0; i < MAX_DIRECTORIES; ++i) {
            char directoryWithNumber[PATH_SIZE];
            sprintf(directoryWithNumber, "%d. %s\n", i + 1, directories[i]);
            strcat(dirList, directoryWithNumber);
        }
        printf("%s\n", dirList);
        send(newSocket, dirList, strlen(dirList), 0);

        if (recv(newSocket, &n, sizeof(n), 0) > 0) {
            n -= 1;
            if (isDirectoryValid(n)) {
                send(newSocket, "OK", 2, 0);
                threadArgs->directoryIndex = n;

                if (pthread_create(&thread_id, NULL, socketThread, (void *)threadArgs) != 0) {
                    printf("Utworzenie wątku się nie powiodło\n");
                }
            pthread_detach(thread_id);
            } else {
                send(newSocket, "ERR", 3, 0);
            } 
        }
    }

    return 0;
}

int isDirectoryValid(int clientDirectoryIndex) {
    return (clientDirectoryIndex >= 0 && clientDirectoryIndex < MAX_DIRECTORIES);
}

void createDirectories(const char *currentDirectory) {
    for (int i = 0; i < MAX_DIRECTORIES; ++i) {
        char directoryPath[PATH_SIZE];
        snprintf(directoryPath, sizeof(directoryPath), "%s/%s", currentDirectory, directories[i]);

        struct stat st;
        if (stat(directoryPath, &st) != 0) {
            if (mkdir(directoryPath, 0777) == -1) {
                perror("Błąd podczas tworzenia katalogu");
                exit(EXIT_FAILURE);
            }
        } else if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Ścieżka %s nie jest katalogiem.\n", directoryPath);
            exit(EXIT_FAILURE);
        }
    }
}

void handleModifyFile(int clientSocket, const char *fileName, int directoryIndex) {
    char response[BUFFER_SIZE];
    char filePath[BUFFER_SIZE];
    char lockFilePath[BUFFER_SIZE];

    snprintf(filePath, BUFFER_SIZE, "%s/%s", directories[directoryIndex], fileName);
    snprintf(lockFilePath, BUFFER_SIZE + 5, "%s%s", filePath, ".lock");

    if (access(filePath, F_OK) == 0) {
        int lockFile = open(lockFilePath, O_CREAT | O_EXCL | O_WRONLY, 0666);

        if (lockFile != -1) {
            close(lockFile);

            send(clientSocket, filePath, strlen(filePath), 0);

            unlink(lockFilePath);
            memset(&response, 0, sizeof(response));
            if (recv(clientSocket, response, sizeof(response), 0) > 0) {
                if (strcmp(response, "M") == 0) {
                    sprintf(response, "Plik '%s' został zmodyfikowany.", fileName);
                    sendToAll(response, strlen(response), clientSocket, directoryIndex);
                }
            }

        } else {
            sprintf(response, "Plik '%s' jest obecnie edytowany przez innego klienta.", fileName);
            send(clientSocket, response, strlen(response), 0);
        }
    } else {
        sprintf(response, "Plik '%s' nie istnieje.", fileName);
        send(clientSocket, response, strlen(response), 0);
    }
}

void handleRemoveFile(int clientSocket, const char *fileName, int directoryIndex) {
    char response[BUFFER_SIZE];
    char filePath[BUFFER_SIZE];

    snprintf(filePath, BUFFER_SIZE, "%s/%s", directories[directoryIndex], fileName);
    printf("%s\n", filePath);

    if (access(filePath, F_OK) == 0) {
        if (remove(filePath) == 0) {
            sprintf(response, "Plik '%s' został usunięty.", fileName);
            send(clientSocket, response, strlen(response), 0);

            sprintf(response, "Plik '%s' został usunięty.", fileName);
            sendToAll(response, strlen(response), clientSocket, directoryIndex);
        } else {
            perror("Błąd podczas usuwania pliku");
            sprintf(response, "Błąd podczas usuwania pliku '%s'.", fileName);
            send(clientSocket, response, strlen(response), 0);
        }
    } else {
        sprintf(response, "Plik '%s' nie istnieje.", fileName);
        send(clientSocket, response, strlen(response), 0);
    }
}

void handleAddFile(int clientSocket, const char *fileName, int directoryIndex) {
    char response[BUFFER_SIZE];
    char filePath[BUFFER_SIZE];

    snprintf(filePath, BUFFER_SIZE, "%s/%s", directories[directoryIndex], fileName);
    printf("%s\n", filePath);

    if (access(filePath, F_OK) != 0) {
        FILE *file = fopen(filePath, "w");
        if (file != NULL) {
            fclose(file);

            sprintf(response, "Plik '%s' został utworzony.", fileName);
            send(clientSocket, response, strlen(response), 0);

            sprintf(response, "Nowy plik '%s' został utworzony.", fileName);
            sendToAll(response, strlen(response), clientSocket, directoryIndex);
        } else {
            perror("Błąd podczas tworzenia pliku");
            sprintf(response, "Błąd podczas tworzenia pliku '%s'.", fileName);
            send(clientSocket, response, strlen(response), 0);
        }
    } else {
        sprintf(response, "Plik '%s' już istnieje.", fileName);
        send(clientSocket, response, strlen(response), 0);
    }
}

void sendFiles(int clientSocket, int directoryIndex) {
    DIR *dir;
    struct dirent *ent;
    char file_list[BUFFER_SIZE] = "";
    struct stat st;

    dir = opendir(directories[directoryIndex]);
    if (dir != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            char file_path[BUFFER_SIZE];
            snprintf(file_path, BUFFER_SIZE, "%s/%s", directories[directoryIndex], ent->d_name);

            if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode)) {
                strcat(file_list, ent->d_name);
                strcat(file_list, "\n");
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
        return;
    }

    if (strlen(file_list) > 0) {
        send(clientSocket, file_list, strlen(file_list), 0);
    } else {
        send(clientSocket, "err", 3, 0);
    }
}

void handleOption(int option, int clientSocket, int directoryIndex) {
    memset(&buffer, 0, sizeof(buffer));

    switch (option) {
        case 1:
            printf("Otrzymano żądanie dodania pliku od klienta %d.\n", clientSocket);
            if (recv(clientSocket, buffer, sizeof(buffer), 0) > 0) {
                handleAddFile(clientSocket, buffer, directoryIndex);
            }
            break;
        case 2:
            printf("Otrzymano żądanie usunięcia pliku od klienta %d.\n", clientSocket);
            if (recv(clientSocket, buffer, sizeof(buffer), 0) > 0) {
                handleRemoveFile(clientSocket, buffer, directoryIndex);
            }
            break;
        case 3:
            printf("Otrzymano żądanie modyfikacji pliku od klienta %d.\n", clientSocket);
            if (recv(clientSocket, buffer, sizeof(buffer), 0) > 0) {
                handleModifyFile(clientSocket, buffer, directoryIndex);
            }
            break;
        case 4:
            printf("Otrzymano żądanie wyświetlenia listy plików od klienta %d.\n", clientSocket);
            sendFiles(clientSocket, directoryIndex);
            break;
        case 5:
            printf("Otrzymano żądanie wyjścia od klienta.\n");
            strcpy(buffer, "Klient opuścił serwer.");
            sendToAll(buffer, strlen(buffer), clientSocket, directoryIndex);
            close(clientSocket);
            break;
        default:
            printf("Nieprawidłowa opcja od klienta.\n");
    }
}

void sendToAll(char *message, int length, int current_socket, int directoryIndex) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < clients_count; i++) {
        if (client_sockets[i] != current_socket && client_directories[i] == directoryIndex) {
            send(client_sockets[i], message, length, 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void *socketThread(void *arg) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)arg;
    int newSocket = threadArgs->newSocket;
    int directoryIndex = threadArgs->directoryIndex;
    int n;

    printf("Nowy wątek klienta nr %d\n", newSocket);

    pthread_mutex_lock(&lock);

    if (clients_count < MAX_CLIENTS) {
        client_directories[clients_count] = directoryIndex;
        client_sockets[clients_count++] = newSocket;
    } else {
        printf("Osiągnięto maksymalną liczbę klientów.\n");
    }

    pthread_mutex_unlock(&lock);

    int option;
    do {
        int n = recv(newSocket, client_message, sizeof(client_message), 0);
        if (n <= 0) {
            pthread_mutex_lock(&lock);
            for (int i = 0; i < clients_count; i++) {
                if (client_sockets[i] == newSocket) {
                    for (int j = i; j < clients_count - 1; j++) {
                        client_sockets[j] = client_sockets[j + 1];
                        client_directories[j] = client_directories[j + 1];
                    }
                    clients_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
            break;
        }

        option = atoi(client_message);
        handleOption(option, newSocket, directoryIndex);

        memset(&client_message, 0, sizeof(client_message));
    } while (option != 5);

    printf("Zamykanie wątku klienta %d\n", newSocket);
    close(newSocket);
    free(arg);
    pthread_exit(NULL);
}
