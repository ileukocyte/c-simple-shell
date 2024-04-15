#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"

char *format = "tuhd";

char *read_line(void) {
    char *line = NULL;
    ssize_t buffsize = 0;

    if (getline(&line, &buffsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        }

        perror("read_line");
        exit(EXIT_FAILURE);
    }

    return line;
}

void exec_and_return(char **args) {
    if (execvp(args[0], args) == -1) {
        perror("This process cannot be executed");
    }

    exit(EXIT_FAILURE);
}

// Command execution function
int launch(char **args, int arg_count) {
    if (strcmp(args[0], "help") == 0) {
        help_cmd();

        return 1;
    }

    if (strcmp(args[0], "prompt") == 0) {
        if (arg_count == 1) {
            format = malloc(5);
            strcpy(format, "tuhd");
        } else {
            if (strcmp(args[1], "-h") == 0) {
                printf("This command changes the prompt's format\n");
                printf("Possible variables:\n");
                printf("t - current time\n");
                printf("u - current user's name\n");
                printf("h - host name\n");
                printf("d - working directory's name\n\n");
                printf("All the variablse are optional but no other can be provided\n");
                printf("The default format is \"tuhd\" (which will also be set in case no arguments are provided)\n");
            } else {
                format = malloc(strlen(args[1]) + 1);
                strcpy(format, args[1]);
            }
        }

        return 1;
    }

    if (strcmp(args[0], "halt") == 0) {
        exit(0);
    }

    pid_t pid = fork(), wpid;

    int status;

    if (pid == 0) {
        int in = index_of_arr(args, "<", 0, arg_count);
        int out = index_of_arr(args, ">", 0, arg_count);
        char in_desc_file[256] = {0}, out_desc_file[256] = {0};

        for (int i = 0; i < arg_count; i++) {
            if (in == i) {
                strcpy(in_desc_file, args[i + 1]);
                break;
            }

            if (out == i) {
                strcpy(out_desc_file, args[i + 1]);
                break;
            }
        }

        int index = in > -1 ? in : out;

        if (in > -1 || out > -1) {
            args[index] = NULL;

            if (in > -1) {
                int desc = open(in_desc_file, O_RDONLY);
                dup2(desc, STDIN);
                close(desc);
            } else {
                int desc = open(out_desc_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(desc, STDOUT);
                close(desc);
            }
        }

        exec_and_return(args);
    } else if (pid < 0) {
        perror("Error while forking");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!(WIFEXITED(status) || WIFSIGNALED(status)));
    }

    return 1;
}

// A function to process a command pipeline
int launch_pipeline(char **commands, int cmd_count) {
    int pipefd[cmd_count - 1][2];

    for (int i = 0; i < cmd_count - 1; i++) {
        pipe(pipefd[i]);
    }

    for (int i = 0; i < cmd_count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error while forking");
        } else if (pid == 0) {
            // Redirecting the input
            if (i != 0) {
                dup2(pipefd[i - 1][0], STDIN);
            }

            // Redirecting the output
            if (i != cmd_count - 1) {
               dup2(pipefd[i][1], STDOUT);
            }

            // Closing the pipes
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }

            char **args = split_cmd(commands[i], TOKEN_DELIM);

            // Child process
            exec_and_return(args);

            free(args);
        }
    }

    // Closing the pipes
    for (int j = 0; j < cmd_count - 1; j++) {
        close(pipefd[j][0]);
        close(pipefd[j][1]);
    }

    // Parent processes
    for (int j = 0; j < cmd_count; j++) {
        wait(NULL);
    }

    return 1;
}

void shell_loop();

void start_server(int port) {
    int client_sockets[16], max_sd, sd, activity, i, valread;
    struct sockaddr_in serv_addr, cli_addr;

    socklen_t clilen;

    char buffer[1024];

    fd_set readfds;

    // Initialize all client sockets to 0
    for (i = 0; i < 16; i++) {
        client_sockets[i] = 0;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("Error opening socket");
        return;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        close(sockfd);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        max_sd = sockfd;

        for (i = 0; i < 16; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Error in select");

            close(sockfd);
        }

        // Check for new connections
        if (FD_ISSET(sockfd, &readfds)) {
            int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

            if (newsockfd < 0) {
                perror("Error on accept");

                continue;
            }

            printf("New connection (fd = %d)\n", newsockfd);

            for (i = 0; i < 16; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = newsockfd;

                    break;
                }
            }
        }

        // Handle IO for each socket
        for (i = 0; i < 16; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    //getpeername(sd, (struct sockaddr*) &cli_addr, &clilen);
                    printf("Host disconnected\n");

                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';

                    if (dup2(sd, STDOUT) < 0) {
                        break;
                    }

                    int count = 0;

                    char **commands = split_string(buffer, &count);

                    if (commands) {
                        for (int j = 0; j < count; j++) {
                            // If a pipeline is found
                            if (index_of(commands[j], '|', 0, strlen(commands[j])) > -1) {
                                char **cmds = split_cmd(commands[j], "|");
                                int cmd_count = 0;

                                while (cmds[cmd_count] != NULL) {
                                    cmd_count++;
                                }

                                launch_pipeline(cmds, cmd_count);

                                free(cmds);
                            } else {
                                char **args = split_cmd(commands[j], TOKEN_DELIM);
                                int arg_count = 0;

                                while (args[arg_count] != NULL) {
                                    arg_count++;
                                }

                                launch(args, arg_count);

                                free(args);
                            }
                        }

                        // Free the memory after use
                        free_string_arr(commands, count);
                    }
                }
            }
        }
    }
}

void start_client(const char *hostname, int port) {
    struct sockaddr_in serv_addr;
    struct hostent *server = gethostbyname(hostname);

    char buffer[65536];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("Error opening socket");

        return;
    }

    if (server == NULL) {
        perror("No such host");

        return;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        perror("Error connecting");

        return;
    }

    while (true) {
        printf("Please enter the message: ");

        char *input = read_line();

        if (strcmp(input, "quit\n") == 0) {
            printf("Disconnecting from the server and going into the local mode\n");

            break;
        }

        int bytes = write(sockfd, input, strlen(input));

        free(input);

        if (bytes < 0) {
            perror("Error writing to socket");

            break;
        }

        bzero(buffer, 65536);
        bytes = read(sockfd, buffer, 65535);

        if (bytes < 0) {
            perror("Error reading from socket");

            break;
        }

        printf("%s\n", buffer);
    }

    close(sockfd);
    shell_loop();
}

// Main execution loop
void shell_loop() {
    int status;

    do {
        char *prompt = get_prompt(format);
        printf("%s ", prompt);

        free(prompt);

        char *input_line = read_line();
        int count = 0;
        char **commands = split_string(input_line, &count);

        if (commands) {
            for (int i = 0; i < count; i++) {
                if (index_of(commands[i], '|', 0, strlen(commands[i])) > -1) {
                    char **cmds = split_cmd(commands[i], "|");
                    int cmd_count = 0;

                    while (cmds[cmd_count] != NULL) {
                        cmd_count++;
                    }

                    status = launch_pipeline(cmds, cmd_count);

                    free(cmds);
                } else {
                    char **args = split_cmd(commands[i], TOKEN_DELIM);
                    int arg_count = 0;

                    while (args[arg_count] != NULL) {
                        arg_count++;
                    }

                    status = launch(args, arg_count);

                    free(args);
                }
            }

            // Free the memory after use
            free_string_arr(commands, count);
        }

        free(input_line);
    } while (status);
}

int main(int argc, char* argv[]) {
    // Interactive mode
    if (isatty(STDIN)) {
        if (argc == 1) {
            shell_loop();
        } else {
            bool is_client = false;
            bool is_server = false;
            int port = 0;

            for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-h") == 0) {
                    help_cmd();
                } else if (strcmp(argv[i], "-c") == 0) {
                    is_client = true;
                } else if (strcmp(argv[i], "-s") == 0) {
                    is_server = true;
                } else if (strcmp(argv[i], "-p") == 0) {
                    if (i + 1 < argc) {
                        port = atoi(argv[i + 1]);
                    } else {
                        perror("No port provided");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            if (is_server && port > 0) {
                start_server(port);
            } else if (is_client && port > 0) {
                start_client("localhost", port);
            }
        }
    } else { // Non-interactive mode
        char buff[1024];
        int count = 0;

        while (fgets(buff, sizeof(buff), stdin) != NULL) {
            char **commands = split_string(buff, &count);

            if (commands) {
                for (int i = 0; i < count; i++) {
                    // If a pipeline is found
                    if (index_of(commands[i], '|', 0, strlen(commands[i])) > -1) {
                        char **cmds = split_cmd(commands[i], "|");
                        int cmd_count = 0;

                        while (cmds[cmd_count] != NULL) {
                            cmd_count++;
                        }

                        launch_pipeline(cmds, cmd_count);

                        free(cmds);
                    } else {
                        char **args = split_cmd(commands[i], TOKEN_DELIM);
                        int arg_count = 0;

                        while (args[arg_count] != NULL) {
                            arg_count++;
                        }

                        launch(args, arg_count);

                        free(args);
                    }
                }

                // Free the memory after use
                free_string_arr(commands, count);
            }
        }
    }
}