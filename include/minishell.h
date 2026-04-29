#ifndef MINISHELL_H
#define MINISHELL_H

#include "e_lib.h"

typedef struct s_cmd {
    char **args;        /* Array of strings: command and its arguments */
    char *cmd_path;     /* Absolute or resolved path to the command */
    int fd_in;          /* Input file descriptor (0 for STDIN by default) */
    int fd_out;         /* Output file descriptor (1 for STDOUT by default) */
    struct s_cmd *next; /* Pointer to the next command in the pipeline */
} t_cmd;

/* Prototypes for functions implemented in src/main.c */
int is_space(char c);
void argcv_handler(int argc, char **argv);
void free_string_array(char **arr);
char *get_next_token(char **str_ptr);
t_cmd *create_node(void);
void add_token_to_args(t_cmd **current_cmd, char *token);
char **split_path(char *path_str);
char *get_cmd_path(char *cmd, char **envp);
void process_token(char *token, t_cmd **current_cmd, char **envp,
                   char **tracker);

#endif
