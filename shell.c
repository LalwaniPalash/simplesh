#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>

// Function declarations
char **split_by_pipe(char *input, int *num_commands);
void parse_redirection(char *command, char **args, char **input_file, char **output_file, int *append_mode);
char **file_completions(const char *text, int start, int end);
void remove_quotes(char *str);
void cd(char *directory);
void help();
void pat(char *path);
void load_history();
void save_history();
void handle_sigint(int sig);
void handle_sigtstp(int sig);

int main() {
    char *input;
    char **commands;
    int num_commands;
    char *input_file = NULL;
    char *output_file = NULL;
    int append_mode = 0;
    int input_fd = 0;
    int pipe_fd[2];

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    load_history();

    while (1) {
        input = readline("> ");

        if (input == NULL) {
            break;
        }

        add_history(input);

        if (strcmp(input, "exit") == 0) {
            save_history();
            break;
        }

        if (strcmp(input, "help") == 0) {
            help();
            continue;
        }

        commands = split_by_pipe(input, &num_commands);

        for (int i = 0; i < num_commands; i++) {
            char *command = commands[i];
            char *args[64];
            parse_redirection(command, args, &input_file, &output_file, &append_mode);

            if (strcmp(args[0], "cd") == 0) {
                cd(args[1]);
                continue;
            } else if (strcmp(args[0], "pat") == 0) {
                pat(args[1]);
                continue;
            }

            if (i < num_commands - 1) {
                if (pipe(pipe_fd) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid = fork();

            if (pid == 0) {
                if (input_file != NULL) {
                    input_fd = open(input_file, O_RDONLY);
                    if (input_fd == -1) {
                        perror("open input file");
                        exit(EXIT_FAILURE);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }

                if (output_file != NULL && i == num_commands - 1) {
                    int flags = O_WRONLY | O_CREAT;
                    flags |= append_mode ? O_APPEND : O_TRUNC;
                    int output_fd = open(output_file, flags, 0644);
                    if (output_fd == -1) {
                        perror("open output file");
                        exit(EXIT_FAILURE);
                    }
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }

                if (i > 0) {
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }

                if (i < num_commands - 1) {
                    dup2(pipe_fd[1], STDOUT_FILENO);
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                }

                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);

                if (input_fd != 0) {
                    close(input_fd);
                }

                if (i < num_commands - 1) {
                    input_fd = pipe_fd[0];
                    close(pipe_fd[1]);
                }
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }

        free(input_file);
        input_file = NULL;
        free(output_file);
        output_file = NULL;
        free(commands);
        free(input);
    }

    save_history();
    return 0;
}

// Function to change the current directory
void cd(char *directory) {
    if (directory == NULL) {
        directory = getenv("HOME");
    }
    if (chdir(directory) == -1) {
        perror("chdir");
    }
}

// Function to print the help menu
void help() {
    printf("Usage: shell [command]\n");
    printf("Commands:\n");
    printf("  exit                     Exit the shell\n");
    printf("  cd [directory]           Change the current directory\n");
    printf("  help                     Print this help menu\n");
    printf("  pat [file]               Print the contents of a file\n");
    printf("\nRedirection:\n");
    printf("  < [file]                 Redirect input from a file\n");
    printf("  > [file]                 Redirect output to a file (overwrite)\n");
    printf("  >> [file]                Redirect output to a file (append)\n");
    printf("\nPipes:\n");
    printf("  |                        Pipe the output of one command to the input of another\n");
}

// Function to read and print a file (like `cat`)
void pat(char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen");
    } else {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            printf("%s", buffer);
        }
        fclose(file);
    }
}

// Function to remove quotes from a string
void remove_quotes(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src != '\"') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

// Function to split a command by pipes
char **split_by_pipe(char *input, int *num_commands) {
    char **commands = malloc(sizeof(char*) * 64);
    char *token = strtok(input, "|");
    int index = 0;

    while (token != NULL) {
        commands[index++] = token;
        token = strtok(NULL, "|");
    }
    commands[index] = NULL;
    *num_commands = index;

    return commands;
}

// Function to load and save the shell history
void load_history() {
    read_history(".my_shell_history");
}
void save_history() {
    write_history(".my_shell_history");
}

// Function to get file completions
char **file_completions(const char *text, int start, int end) {
    char **matches = NULL;
    char *dir = ".";

    if (start == 0) {
        dir = ".";
    }

    DIR *d = opendir(dir);
    if (d) {
        struct dirent *entry;
        while ((entry = readdir(d))) {
            if (strncmp(entry->d_name, text, end - start) == 0) {
                matches = (char **)realloc(matches, sizeof(char *) * (matches ? (sizeof(matches) / sizeof(char *)) + 1 : 1));
                matches[matches ? (sizeof(matches) / sizeof(char *)) - 1 : 0] = strdup(entry->d_name);
            }
        }
        closedir(d);
    }
    return matches;
}

// Function to free file completions
void free_completions(char **completions) {
    if (completions) {
        for (int i = 0; completions[i]; i++) {
            free(completions[i]);
        }
        free(completions);
    }
}

// Signal handlers
void handle_sigint(int sig) {
    printf("\nCaught signal %d\n", sig);
    printf("> ");
    fflush(stdout);
    signal(SIGINT, handle_sigint);
}
void handle_sigtstp(int sig) {
    printf("\nCaught signal %d\n", sig);
    printf("> ");
    fflush(stdout);
}

// Function to parse redirection
void parse_redirection(char *command, char **args, char **input_file, char **output_file, int *append_mode) {
    *input_file = NULL;
    *output_file = NULL;
    *append_mode = 0;

    int arg_index = 0;
    char *token = strtok(command, " ");

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                remove_quotes(token);
                *input_file = strdup(token);
            }
        } else if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            *append_mode = (token[1] == '>');
            token = strtok(NULL, " ");
            if (token != NULL) {
                remove_quotes(token);
                *output_file = strdup(token);
            }
        } else {
            remove_quotes(token);
            args[arg_index++] = strdup(token);
        }
        token = strtok(NULL, " ");
    }
    args[arg_index] = NULL;
}