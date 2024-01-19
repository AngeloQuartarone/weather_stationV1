// il problema è nella sincronizzazione
// provare a dividere comunicazione tra server e client e tra server ed esp32

#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "./lib/circularQueue.h"
#include "./lib/zambretti.h"

#define PORT 8080
#define BUFFERSIZE 128

int running = 1;

void *fun_sig_handler(void *);

int main()
{
    /*sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);

    pthread_t signal_handler;
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&signal_handler, NULL, fun_sig_handler, &set);
    pthread_detach(signal_handler);*/
    struct sockaddr_in address;
    int opt = 1;
    int act_case = 0;
    char *result = NULL;

    fd_set readfds;
    int server_fd, addrlen, new_socket, client_socket[30],
        max_clients = 30, activity, i, valread, sd;
    int max_sd;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    float pressure = 0, temperature = 0, humidity = 0, sea_level_pressure = 0, p_new = 0, p_old = 0;
    int act_press_trnd = 0;
    circularQueue *q = initQueue(36);
    FILE *output;
    char *received = calloc(1, BUFFERSIZE);

    if (listen(server_fd, 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);

    while (running)
    {
        // clear the socket set
        FD_ZERO(&readfds);
        // add master socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // add child sockets to set
        for (i = 0; i < max_clients; i++)
        {
            // socket descriptor
            sd = client_socket[i];

            // if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            // highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }
        ssize_t bytesRead;
        if (FD_ISSET(server_fd, &readfds))
        {
            // ssize_t bytesRead = read(new_socket, received, BUFFERSIZE);

            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                // if position is empty
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }

        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
            received[0] = '\0';

            if (FD_ISSET(sd, &readfds))
            {
                // Check if it was for closing , and also read the
                // incoming message
                if ((valread = read(sd, received, BUFFERSIZE)) == 0)
                {
                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                }
                else
                {

                    if (isFull(q))
                    {
                        p_new = q->arr[1];
                        p_old = q->arr[35];

                        act_press_trnd = pressureTrend(p_new, p_old);
                        sea_level_pressure = pressureSeaLevel(temperature, pressure);
                        act_case = caseCalculation(act_press_trnd, sea_level_pressure);
                        result = lookUpTable(act_case);

                        fprintf(output, "%d, %s\n", act_case, result);
                        fclose(output);

                        // printf("%d, %s\n", act_case, result);
                    }

                    // printf("Server in attesa di connessioni...\n");

                    // char *received = calloc(1, BUFFERSIZE);

                    if (strstr(received, "P:") != NULL)
                    {
                        fprintf(output, "%s\n", received);
                        fclose(output);

                        char *pressure_str = (char *)calloc(1, BUFFERSIZE);
                        char *temperature_str = (char *)calloc(1, BUFFERSIZE);
                        char *humidity_str = (char *)calloc(1, BUFFERSIZE);
                        char *tmp;

                        strcpy(pressure_str, received);
                        strcpy(temperature_str, received);
                        strcpy(humidity_str, received);

                        tmp = strstr(pressure_str, "T:");
                        strtok(tmp, ":");
                        temperature = atof(strtok(NULL, "")); // conversion pascal in millibar (check it)

                        tmp = NULL;
                        tmp = strstr(humidity_str, "H:");
                        strtok(tmp, ":");
                        humidity = atof(strtok(NULL, ""));

                        tmp = NULL;
                        tmp = strstr(temperature_str, "P:");
                        strtok(tmp, "/:");
                        pressure = atof(strtok(NULL, "")) / 100;

                        // printf("ricevuto: %s\n", received);
                        // printf("temperatura: %f, pressione: %f\n", temperature, pressure);

                        char *message = "OK";
                        send(new_socket, message, strlen(message), 0);

                        free(pressure_str);
                        free(temperature_str);
                        close(sd);

                        enqueue(q, pressure);
                    }
                    /*else if (strcmp(received, "FORECAST") == 0)
                    {
                        fprintf(output, "%s\n", received);
                        fclose(output);
                        if (isFull(q))
                        {
                            send(new_socket, result, strlen(result), 0);
                            close(new_socket);
                        }
                        else
                        {
                            char *message = "Non ci sono abbastanza dati, Torna più tardi!";
                            send(new_socket, message, strlen(message), 0);
                            close(new_socket);
                        }
                    }
                    else if (strcmp(received, "S_VALUE") == 0)
                    {
                        fprintf(output, "%s\n", received);
                        fclose(output);
                        char *message = calloc(1, BUFFERSIZE);
                        sprintf(message, "temperatura: %.2f C, umidità: %.2f %, pressione: %.2f mBar\n", temperature, humidity, pressure);
                        send(new_socket, message, strlen(message), 0);
                        close(new_socket);
                        free(message);
                    }*/
                }
            }
        }

        // free(received);
    }

    deleteQueue(q);

    return 0;
}

void *fun_sig_handler(void *arg)
{
    sigset_t *set = arg;
    int s, sig;

    while (1)
    {
        s = sigwait(set, &sig);
        if (s == 0)
        {
            running = 0;
        }
    }
    return NULL;
}
