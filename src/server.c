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
#include <time.h>
#include "../lib/circularQueue.h"
#include "../lib/zambretti.h"

#define PORT 8080
#define BUFFERSIZE 256

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
    int server_fd, addrlen, new_socket, client_socket[36],
        max_clients = 36, activity, i, valread, sd;
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
    FILE *dataIn_log;
    FILE *dataOut_log;
    time_t ltime;

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);

    while (running)
    {
        char *received = calloc(1, BUFFERSIZE);
        char *message = calloc(1, BUFFERSIZE);

        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        if (FD_ISSET(server_fd, &readfds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            for (i = 0; i < max_clients; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (i = 0; i < max_clients; i++)
        {

            sd = client_socket[i];

            p_new = q->front;
            p_old = q->rear;

            act_press_trnd = pressureTrend(p_new, p_old);
            sea_level_pressure = pressureSeaLevel(temperature, pressure);
            act_case = caseCalculation(act_press_trnd, sea_level_pressure);
            result = lookUpTable(act_case);
            // printf("trend: %d, sea level: %f", act_press_trnd, sea_level_pressure);

            if (FD_ISSET(sd, &readfds))
            {
                if ((valread = read(sd, received, BUFFERSIZE)) == 0)
                {
                    close(sd);
                    client_socket[i] = 0;
                }
                else
                {
                    ltime = time(NULL);
                    dataIn_log = fopen("./log/data_received.log", "a");
                    if (dataIn_log != NULL)
                    {
                        struct tm *p = localtime(&ltime);
                        char s[32];

                        strftime(s, sizeof(s), "%a %b %d %H:%M:%S %Y", p);

                        fprintf(dataIn_log, "%s -> %s\n", s, received);
                        fclose(dataIn_log);
                    }
                    else
                    {
                        printf("fopen error");
                        exit(EXIT_FAILURE);
                    }

                    if (strstr(received, "P:") != NULL)
                    {

                        char *pressure_str = (char *)calloc(1, BUFFERSIZE);
                        char *temperature_str = (char *)calloc(1, BUFFERSIZE);
                        char *humidity_str = (char *)calloc(1, BUFFERSIZE);
                        char *tmp;

                        strcpy(pressure_str, received);
                        strcpy(temperature_str, received);
                        strcpy(humidity_str, received);

                        tmp = strstr(pressure_str, "T:");
                        strtok(tmp, ":");
                        double t = atof(strtok(NULL, ""));
                        if (t != (double)-1)
                        {
                            temperature = t;
                        }

                        tmp = NULL;
                        tmp = strstr(humidity_str, "H:");
                        strtok(tmp, ":");
                        double h = atof(strtok(NULL, ""));
                        if (h != (double)-1)
                        {
                            humidity = h;
                        }

                        tmp = NULL;
                        tmp = strstr(temperature_str, "P:");
                        strtok(tmp, "/:");
                        pressure = atof(strtok(NULL, "")) / 100; // conversion pascal in millibar (check it)

                        strcpy(message, "OK");
                        send(new_socket, message, strlen(message), 0);

                        free(pressure_str);
                        free(temperature_str);

                        enqueue(q, pressure);
                    }

                    else
                    {
                        sprintf(message, "%d, %s", act_case, result);
                        dataOut_log = fopen("./log/data_sended.log", "a");

                        if (dataOut_log != NULL)
                        {
                            // ltime = time(NULL);
                            struct tm *p = localtime(&ltime);
                            char s[32];

                            strftime(s, sizeof(s), "%a %b %d %H:%M:%S %Y", p);

                            fprintf(dataOut_log, "%s -> %s\n", s, message);
                            fclose(dataOut_log);
                        }

                        if (strcmp(received, "FORECAST") == 0)
                        {
                            if (isFull(q))
                            {
                                send(sd, message, strlen(result), 0);
                            }
                            else
                            {
                                sprintf(message, "Inaccurate forecast, try again later.\nActual forecast: %d, %s", act_case, result);
                                send(sd, message, strlen(message), 0);
                            }
                        }
                        else if (strcmp(received, "S_VALUE") == 0)
                        {
                            sprintf(message, "temperatura: %.f C, umidità: %.f %%, pressione: %.2f mBar", temperature, humidity, pressure);
                            send(sd, message, strlen(message), 0);
                        }
                    }
                }
            }
        }
        if (received != NULL)
        {
            free(received);
        }
        if (message != NULL)
        {
            free(message);
        }
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
