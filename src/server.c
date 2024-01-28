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
void errorLog(FILE *f, char *s);

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
    char *error_string;
    error_string = calloc(1, BUFFERSIZE);

    FILE *error_log;

    fd_set readfds;
    int server_fd, addrlen, new_socket, client_socket[36],
        max_clients = 36, activity, i, valread, sd;
    int max_sd;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if ((error_log = fopen("./log/error.log", "a")) == NULL)
    {
        perror("error fopen");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        strcpy(error_string, "socket");
        errorLog(error_log, error_string);
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        strcpy(error_string, "setsockopt");
        errorLog(error_log, error_string);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        strcpy(error_string, "bind");
        errorLog(error_log, error_string);
        exit(EXIT_FAILURE);
    }

    float pressure = 0, temperature = 0, humidity = 0, sea_level_pressure = 0, p_new = 0, p_old = 0;
    int act_press_trnd = 0;
    circularQueue *q = initQueue(36);
    FILE *dataIn_log;

    // FILE *dataOut_log;
    time_t ltime;

    if (listen(server_fd, 3) == -1)
    {
        strcpy(error_string, "listen");
        errorLog(error_log, error_string);
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);

    while (running)
    {
        char *received = calloc(1, BUFFERSIZE);
        if (received == NULL)
        {
            strcpy(error_string, "calloc received");
            errorLog(error_log, error_string);
            exit(EXIT_FAILURE);
        }
        char *message = calloc(1, BUFFERSIZE);
        if (message == NULL)
        {
            strcpy(error_string, "calloc message");
            errorLog(error_log, error_string);
            exit(EXIT_FAILURE);
        }

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

        if ((activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout)) == -1)
        {
            strcpy(error_string, "select");
            errorLog(error_log, error_string);
            exit(EXIT_FAILURE);
        }

        if ((activity < 0) && (errno != EINTR))
        {
            strcpy(error_string, "if activity EINTR");
            errorLog(error_log, error_string);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) == -1)
            {
                strcpy(error_string, "accept");
                errorLog(error_log, error_string);
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

            p_new = q->arr[q->rear];
            p_old = q->arr[q->front];

            //printf("new: %f, old: %f\n", p_new, p_old);

            act_press_trnd = pressureTrend(p_new, p_old);
            sea_level_pressure = pressureSeaLevel(temperature, pressure);
            act_case = caseCalculation(act_press_trnd, sea_level_pressure);
            result = lookUpTable(act_case);
            // printf("trend: %d, sea level: %f\n", act_press_trnd, sea_level_pressure);

            if (FD_ISSET(sd, &readfds))
            {
                valread = read(sd, received, BUFFERSIZE);
                /*strcpy(error_string, "read");
                if (valread == -1)
                {
                    errorLog(error_log, error_string);
                    exit(EXIT_FAILURE);
                }*/
                if (valread == 0 || valread == -1)
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
                        strcpy(error_string, "fopen data received");
                        errorLog(error_log, error_string);
                        exit(EXIT_FAILURE);
                    }

                    if (strstr(received, "P:") != NULL)
                    {

                        char *pressure_str = (char *)calloc(1, BUFFERSIZE);
                        if (pressure_str == NULL)
                        {
                            strcpy(error_string, "calloc pressure string");
                            errorLog(error_log, error_string);
                            exit(EXIT_FAILURE);
                        }
                        char *temperature_str = (char *)calloc(1, BUFFERSIZE);
                        if (temperature_str == NULL)
                        {
                            strcpy(error_string, "calloc temperature string");
                            errorLog(error_log, error_string);
                            exit(EXIT_FAILURE);
                        }
                        char *humidity_str = (char *)calloc(1, BUFFERSIZE);
                        if (humidity_str == NULL)
                        {
                            strcpy(error_string, "calloc humidity string");
                            errorLog(error_log, error_string);
                            exit(EXIT_FAILURE);
                        }
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
                        int x = send(new_socket, message, strlen(message), 0);
                        if (x == -1)
                        {
                            strcpy(error_string, "send to arduino");
                            errorLog(error_log, error_string);
                            exit(EXIT_FAILURE);
                        }

                        free(pressure_str);
                        free(temperature_str);

                        if (isFull(q))
                        {
                            dequeue(q);
                        }
                        enqueue(q, pressure);
                        // printf("enqueued: %f\n", pressure);
                    }

                    else
                    {
                        sprintf(message, "Actual forecast: %s", result);
                        // dataOut_log = fopen("./log/data_sended.log", "a");

                        /*if (dataOut_log != NULL)
                        {
                            // ltime = time(NULL);
                            struct tm *p = localtime(&ltime);
                            char s[32];

                            strftime(s, sizeof(s), "%a %b %d %H:%M:%S %Y", p);

                            fprintf(dataOut_log, "%s -> %s\n", s, message);
                            fclose(dataOut_log);
                        }*/

                        if (strcmp(received, "FORECAST") == 0)
                        {
                            if (isFull(q))
                            {
                                int x = send(sd, message, strlen(message), 0);
                                if (x == -1)
                                {
                                    strcpy(error_string, "send to python client 1");
                                    errorLog(error_log, error_string);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                sprintf(message, "Inaccurate forecast, try again later.\nActual forecast: %s", result);
                                int x = send(sd, message, strlen(message), 0);
                                if (x == -1)
                                {
                                    strcpy(error_string, "send to python client 2");
                                    errorLog(error_log, error_string);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        else if (strcmp(received, "S_VALUE") == 0)
                        {
                            sprintf(message, "Temperature: %.f C\nHumidity: %.f %%\nAtmosferic pressure: %.2f mBar", temperature, humidity, pressure);
                            int x = send(sd, message, strlen(message), 0);
                            if (x == -1)
                            {
                                strcpy(error_string, "send to python client 3");
                                errorLog(error_log, error_string);
                                exit(EXIT_FAILURE);
                            }
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

void errorLog(FILE *f, char *s)
{
    time_t ltime;
    ltime = time(NULL);
    struct tm *p = localtime(&ltime);
    char t[32];

    strftime(t, sizeof(s), "%a %b %d %H:%M:%S %Y", p);

    fprintf(f, "%s -> error: %s, errno: %d\n", t, s, errno);
    fclose(f);
    free(s);
}
