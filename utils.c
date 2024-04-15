#include "utils.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <bits/local_lim.h>

// A function that customizes the prompt's format
char *get_prompt(char *format) {
    char *prompt = malloc(1024 * sizeof(char));
    prompt[0] = '\0';

    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    char time_str[6];

    snprintf(time_str, sizeof(time_str), "%02d:%02d", local->tm_hour, local->tm_min);

    struct passwd *pw;
    pw = getpwuid(getuid());

    char *username = pw->pw_name;
    char hostname[HOST_NAME_MAX + 1] = {0};
    char cwd[1024] = {0};

    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    int len = strlen(format);

    for (int i = 0; i < len; i++) {
        if (format[i] == 't') {
            strcat(prompt, time_str);
            strcat(prompt, i != len - 1 ? " " : "$");
        } else if (format[i] == 'u') {
            strcat(prompt, username);
            strcat(prompt, i != len - 1 ? "@" : "$");
        } else if (format[i] == 'h') {
            strcat(prompt, hostname);
            strcat(prompt, i != len - 1 ? ":" : "$");
        } else if (format[i] == 'd') {
            strcat(prompt, cwd);
            strcat(prompt, i != len - 1 ? ":" : "$");
        } else if (i == len - 1) {
            if (prompt[strlen(prompt) - 1] != '$') {
                strcat(prompt, "$");
            }
        }
    }

    return strlen(prompt) > 0 ? prompt : "$";
}

void help_cmd() {
    printf("This program is the implementation of the assignment #2 ");
    printf("by Oleksandr Oksanich (AIS ID: 122480)\n\n");
    printf("Internal commands:\n");
    printf("help: displays the current message\n");
    printf("prompt: configures the prompt (use -h for help)\n");
    printf("quit: (client-only) terminates the server connection\n");
    printf("halt: terminates the program\n\n");
    printf("Implemented bonus tasks: 1, 14, 17, 28, 30\n\n");
    printf("Usage:\n");
    printf("spaasm2\n");

    printf("spaasm2 -h\n");
    printf("spaasm2 < script_file\n");
    printf("spaasm2 -c -p port\n");
    printf("spaasm2 -s -p port\n");
}

// A function that returns the index of a character within a string if found
int index_of(char *str, char c, int start, int end) {
    if (str == NULL) {
        return -1;
    }

    // Loop through the string looking for the character
    for (int i = start; i < end && str[i] != '\0'; i++) {
        if (str[i] == c) {
            return i;  // Return the index if the character is found
        }
    }

    // Return -1 if the character is not found
    return -1;
}

// A function that returns the index of an elemnet within an array if found
int index_of_arr(char **arr, char *str, int start, int end) {
    if (arr == NULL || str == NULL) {
        return -1;
    }

    // Loop through the array looking for the stringa
    for (int i = start; i < end; i++) {
        if (strcmp(arr[i], str) == 0) {
            return i;  // Return the index if the string is found
        }
    }

    // Return -1 if the string is not found
    return -1;
}

// A function that splits a single command into an array of arguments
char **split_cmd(char *line, char *delim) {
    int buffsize = 64, position = 0;
    char **tokens = malloc(buffsize * sizeof(char *));

    if (!tokens) {
        fprintf(stderr, "Allocation error\n");

        exit(EXIT_FAILURE);
    }

    char *token = strtok(line, delim);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= buffsize) {
            buffsize += 64;
            tokens = realloc(tokens, buffsize * sizeof(char *));

            if (!tokens) {
                fprintf(stderr, "Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, delim);
    }

    tokens[position] = NULL;

    return tokens;
}

// A utility function to add a substring to the array
char **add_to_array(char **array, int *size, char *start, int length) {
    // Allocate memory for the new substring
    char *substring = malloc(length + 1);

    if (!substring) {
        return NULL;
    }

    // Copy the substring into newly allocated space
    strncpy(substring, start, length);
    substring[length] = '\0';

    // Resize the array to hold one more pointer
    char **new_array = realloc(array, (*size + 1) * sizeof(char *));

    if (!new_array) {
        free(substring);
        return NULL;
    }

    new_array[*size] = substring;
    (*size)++;

    return new_array;
}

// A function to split input string by '\n' or ';'
char **split_string(char *str, int *count) {
    if (!str) {
        return NULL;
    }

    char **result = NULL;
    int num_substrings = 0;
    char *start = str;
    char *p = str;

    // Loop through each character in the string
    while (*p) {
        if (*p == '\n' || *p == ';') {
            if (p != start) { // Avoid adding empty strings
                result = add_to_array(result, &num_substrings, start, p - start);

                if (!result) {
                    return NULL;
                }
            }

            start = p + 1; // Move start to character after the delimiter
        }

        p++;
    }

    // Add the last substring if it's not empty
    if (p != start) {
        result = add_to_array(result, &num_substrings, start, p - start);

        if (!result) {
            return NULL;
        }
    }

    *count = num_substrings;

    return result;
}

void free_string_arr(char **strings, int count) {
    for (int i = 0; i < count; i++) {
        free(strings[i]);
    }

    free(strings);
}
