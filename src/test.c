
    get_nth_element(&readfds)
    char *received = calloc(1, BUFFERSIZE);
    int valread = read(sd, received, BUFFERSIZE);
    /*strcpy(error_string, "read");
    if (valread == -1)
    {
        errorLog(error_string);

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
            errorLog(error_string);
        }

        if (strstr(received, "P:") != NULL)
        {

            char *pressure_str = (char *)calloc(1, BUFFERSIZE);
            if (pressure_str == NULL)
            {
                strcpy(error_string, "calloc pressure string");
                errorLog(error_string);
            }
            char *temperature_str = (char *)calloc(1, BUFFERSIZE);
            if (temperature_str == NULL)
            {
                strcpy(error_string, "calloc temperature string");
                errorLog(error_string);
            }
            char *humidity_str = (char *)calloc(1, BUFFERSIZE);
            if (humidity_str == NULL)
            {
                strcpy(error_string, "calloc humidity string");
                errorLog(error_string);
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
                errorLog(error_string);
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
                        errorLog(error_string);
                    }
                }
                else
                {
                    sprintf(message, "Inaccurate forecast, try again later.\nActual forecast: %s", result);
                    int x = send(sd, message, strlen(message), 0);
                    if (x == -1)
                    {
                        strcpy(error_string, "send to python client 2");
                        errorLog(error_string);
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
                    errorLog(error_string);
                }
            }
            else if (strcmp(received, "R4_VALUE") == 0)
            {
                sprintf(message, "T: %.f C\nH: %.f %%\nP: %.f mbar\nForecast:%s", temperature, humidity, pressure, result);
                int x = send(sd, message, strlen(message), 0);
                if (x == -1)
                {
                    strcpy(error_string, "send to arduino");
                    errorLog(error_string);
                }
            }
        }
    }
