#ifndef UTILS_H
#define UTILS_H

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO
#define STDERR STDERR_FILENO

#define TOKEN_DELIM " \t\r\n\a"

char *get_prompt(char *format);
void help_cmd();
int index_of(char *str, char c, int start, int end);
int index_of_arr(char **arr, char *str, int start, int end);
char **split_cmd(char *line, char *delim);
char **add_to_array(char **array, int *size, char *start, int length);
char **split_string(char *str, int *count);
void free_string_arr(char **strings, int count);
#endif
