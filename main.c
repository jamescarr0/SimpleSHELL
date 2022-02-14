#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define RESET_TEXT_COLOR "\x1b[0m"
#define RED_TEXT "\x1b[31m"

#define TOKEN_BUFSIZE 32

typedef struct {
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
    char *bufferTokens;
} InputBuffer;

InputBuffer *newInputBuffer() {
    InputBuffer *input_buffer = malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

void print_prompt() { printf("$>"); }

void read_input(InputBuffer *inputBuffer) {

    ssize_t bytesRead = getline(&(inputBuffer->buffer), &
            (inputBuffer->buffer_length), stdin);

    if (bytesRead < 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    // Remove trailing newline
    inputBuffer->buffer_length = bytesRead - 1;
    inputBuffer->buffer[bytesRead - 1] = '\0';
}

void closeInputBuffer(InputBuffer *inputBuffer) {
    free(inputBuffer->buffer);
    inputBuffer->buffer = NULL;
    free(inputBuffer);
    inputBuffer = NULL;
}

void printCmdNotFound(char *errMsg) {
    printf("%sError:%s '%s' command not found\n", RED_TEXT,
           RESET_TEXT_COLOR, errMsg);
}

char **createTokens(InputBuffer *inputBuffer) {
    int tokenBufSize = TOKEN_BUFSIZE;
    int tokenPosition = 0;

    char *savePosition = NULL;
    char *token = strtok_r(inputBuffer->buffer, " ", &savePosition);

    char **tokens = malloc(sizeof(char *) * tokenBufSize);

    if (!tokens) {
        printf("Error: unable to allocate memory for tokens");
        closeInputBuffer(inputBuffer); // Clean up before exit.
        exit(EXIT_FAILURE);
    }

    tokens[tokenPosition++] = token;

    while (token != 0) {
        token = strtok_r(NULL, " ", &savePosition);
        tokens[tokenPosition++] = token;
    }

    tokens[tokenPosition] = NULL;
    return tokens;
}

void freeTokens(char **tokens) {
    free(tokens);
}

void shellFork(char **args) {
    pid_t pid;
    int status;

    pid = fork();

    // Attempt to fork and exec a new program.
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            printCmdNotFound(args[0]);
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

int main() {
    InputBuffer *inputBuffer = newInputBuffer();
    char **tokens = NULL;

    while (1) {
        print_prompt();

        read_input(inputBuffer);

        tokens = createTokens(inputBuffer);

        if (strcmp(inputBuffer->buffer, "exit") == 0) {
            printf("Quitting.\n");
            closeInputBuffer(inputBuffer);
            free(tokens);
            exit(EXIT_SUCCESS);
        }

        shellFork(tokens);
        freeTokens(tokens);
    }
}
