#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/select.h>

#define BUFFER_SIZE 2048
#define PORT 1111

int clientSocket, option;
char buffer[BUFFER_SIZE];
char buffer2[BUFFER_SIZE];

void *receiveThread(void *arg);
void printMenu();
int selectDirectory();

int main() {
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    pthread_t receiveThreadId;

    clientSocket = socket(PF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    addr_size = sizeof serverAddr;
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

    if (selectDirectory() != 0) {
        printf("Niepoprawny katalog\n");
        return EXIT_FAILURE;
    }

    if (pthread_create(&receiveThreadId, NULL, receiveThread, NULL) != 0) {
        printf("Utworzenie wątku się nie powiodło\n");
        return EXIT_FAILURE;
    }

    do {
        printMenu();
        scanf("%d", &option);

        char optionStr[2];
        sprintf(optionStr, "%d", option);
        send(clientSocket, optionStr, strlen(optionStr), 0);


        memset(&buffer, 0, sizeof(buffer));
        memset(&buffer2, 0, sizeof(buffer2));
        switch (option) {
            case 1:
                printf("Dodawanie pliku - podaj jego nazwę: ");
                scanf("%s", buffer);
                send(clientSocket, buffer, strlen(buffer), 0);
                break;
            case 2:
                printf("Usuwanie pliku - podaj jego nazwę: ");
                scanf("%s", buffer);
                send(clientSocket, buffer, strlen(buffer), 0);
                break;
            case 3:
                printf("Modyfikacja pliku - podaj jego nazwę: ");
                scanf("%s", buffer);
                send(clientSocket, buffer, strlen(buffer), 0);

                memset(&buffer, 0, sizeof(buffer));
                if (recv(clientSocket, buffer, sizeof(buffer), 0) > 0) {
                    snprintf(buffer2, sizeof(buffer2) + 4, "vim %s", buffer);
                    system(buffer2);
                    send(clientSocket, "M", 1, 0);
                } else {
                    printf("ELSE\n");
                }
                break;
            case 4:
            {
                struct pollfd poll_fd;
                poll_fd.fd = clientSocket;
                poll_fd.events = POLLIN;

                int ready = poll(&poll_fd, 1, 2000); 

                if (ready == -1) {
                    perror("Błąd przy użyciu poll");
                } else if (ready > 0 && (poll_fd.revents & POLLIN)) {
                    memset(&buffer, 0, sizeof(buffer));
                    if (recv(clientSocket, buffer, sizeof(buffer), 0) > 0) {
                        if (strcmp(buffer, "err") == 0) {
                            printf("Obecny katalog nie zawiera żadnych plików\n");
                        } else {
                            printf("Lista plików w obecnym katalogu:\n");
                            printf("%s", buffer);
                        }
                    }
                } else {
                    printf("Brak danych od serwera w określonym czasie.\n");
                }
                break;
            }
            case 5:
                printf("Wyjście - Zamykam klienta...\n");
                break;
            default:
                printf("Nieprawidłowa opcja, spróbuj ponownie.\n");
        }
    } while (option != 5);

    pthread_join(receiveThreadId, NULL);
    close(clientSocket);

    return 0;
}

int selectDirectory() {
    int n;
    memset(&buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);

    printf("Dostępne katalogi:\n%s", buffer);

    printf("Wybierz katalog: ");
    scanf("%d", &n);

    send(clientSocket, &n, sizeof(n), 0);

    memset(&buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);

    if (strcmp(buffer, "OK") == 0) {
        return 0;
    } else {
        return -1;
    }
}


void printMenu() {
    printf("Menu:\n");
    printf("1. Dodaj plik\n");
    printf("2. Usuń plik\n");
    printf("3. Zmodyfikuj plik\n");
    printf("4. Wyświetl listę plików\n");
    printf("5. Wyjście\n");
}

void *receiveThread(void *arg) {
    char localBuffer[BUFFER_SIZE];  
    int bytesRead;
    struct pollfd poll_fd;
    poll_fd.fd = clientSocket;
    poll_fd.events = POLLIN;


    while (1) {
        int ret = poll(&poll_fd, 1, -1);

        if (ret > 0 && (poll_fd.revents & POLLIN)) {
            memset(&localBuffer, 0, sizeof(localBuffer));
            int bytesRead = recv(clientSocket, localBuffer, BUFFER_SIZE - 1, 0);
            if (bytesRead <= 0) {
                printf("Zerwano połączenie albo otrzymano error\n");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }

            localBuffer[bytesRead] = '\0';
            printf("Odpowiedź serwera: %s\n", localBuffer);

            strcpy(buffer, localBuffer);

        }
    }

    return NULL;
}
