#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define PROMPT "(simple shell) ~ $ "
#define RESET_TEXT_COLOR "\x1b[0m"
#define RED_TEXT "\x1b[31m"
#define TOKEN_BUFSIZE 32

typedef struct {
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
    char *bufferTokens;
} input_buffer_t;

input_buffer_t *new_input_buffer() {
    input_buffer_t *input_buffer = malloc(sizeof(input_buffer_t));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

void print_prompt() { printf(PROMPT); }

void read_input(input_buffer_t *const input_buffer) {

    ssize_t bytes_read = getline(&(input_buffer->buffer), &
            (input_buffer->buffer_length), stdin);

    if (bytes_read < 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    // Remove trailing newline
    input_buffer->buffer_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = '\0';
}

void close_input_buffer(input_buffer_t *input_buffer) {
    free(input_buffer->buffer);
    input_buffer->buffer = NULL;
    free(input_buffer);
    input_buffer = NULL;
}

void print_cmd_not_found(char const *const err_msg) {
    printf("%sError:%s '%s' command not found\n", RED_TEXT,
           RESET_TEXT_COLOR, err_msg);
}

void print_error(char const *const err_msg) {
    printf("%sError:%s '%s'\n", RED_TEXT,
           RESET_TEXT_COLOR, err_msg);
}

char **create_tokens(input_buffer_t *const inputBuffer) {
    int tkn_buf_size = TOKEN_BUFSIZE;
    int tkn_position = 0;

    char *save_position = NULL;
    char *token = strtok_r(inputBuffer->buffer, " ", &save_position);

    char **tokens = malloc(sizeof(char *) * tkn_buf_size);

    if (!tokens) {
        printf("Error: unable to allocate memory for tokens");
        close_input_buffer(inputBuffer); // Clean up before exit.
        exit(EXIT_FAILURE);
    }

    tokens[tkn_position++] = token;

    while (token != 0) {
        token = strtok_r(NULL, " ", &save_position);
        tokens[tkn_position++] = token;
    }

    tokens[tkn_position] = NULL;
    return tokens;
}

void free_tokens(char **const tokens) {
    free(tokens);
}

void shell_fork(char **const args) {
    pid_t pid;
    int status;

    pid = fork();

    // Attempt to fork and exec a new program.
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            print_cmd_not_found(args[0]);
        }
        exit(EXIT_FAILURE);
    } else if (pid == -1) {
        // Error Forking
        perror("SimSH: Error forking new process\n");
    } else {
        // Fork & exec successful: Wait for the new program to
        // terminate or be killed.
        do {
            waitpid(pid, &status, WUNTRACED);

            // WIFEXITED–Query status to see if a child process ended normally
            // WIFSIGNALED–Query status to see if a child process ended
            // abnormally
            // Wait until ended normally/abnormally

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

void load_ascii_art_from_file(char *filepath) {

    FILE *fp = fopen(filepath, "r");
    if(!fp) {
        printf("Error opening ascii art file\n");
        printf("*** SIMple SHell ***");
    }

    char *buffer = 0;
    size_t buffer_len = 0;
    size_t bytes_read = 0;

    while (bytes_read != -1) {
        bytes_read = getline(&buffer, &buffer_len, fp);
        printf("%s", buffer);
    }
    fclose(fp);
    free(buffer);
}

int main() {
    input_buffer_t *inputBuffer = new_input_buffer();
    char **tokens = NULL;

    load_ascii_art_from_file("../banner.txt");

    while (1) {
        print_prompt();

        read_input(inputBuffer);

        tokens = create_tokens(inputBuffer);

        if (strcmp(inputBuffer->buffer, "exit") == 0) {
            printf("Quitting.\n");
            close_input_buffer(inputBuffer);
            free(tokens);
            exit(EXIT_SUCCESS);
        } else if (strcmp(inputBuffer->buffer, "cd") == 0) {
            if (chdir(tokens[1]) == -1) {
                if (errno == ENOENT) {
                    print_error("Directory does not exist");
                }
            }
        } else {
            shell_fork(tokens);
        }

        free_tokens(tokens);
    }
}
