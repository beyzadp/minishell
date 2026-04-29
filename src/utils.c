#include "constants.h"
#include "lwlog.h"
#include "minishell.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Helper function to check for whitespace */
int is_space(char c) { return (c == ' ' || c == '\t' || c == '\n'); }

void signal_handler(int signum) {
    if (signum == SIGINT) {
        lwlog_info("Received SIGINT, exiting gracefully");
        exit(0);
    }
    if (signum == SIGQUIT) {
        lwlog_info("Received SIGQUIT, exiting gracefully");
        exit(0);
    }
}

void argcv_handler(int argc, char **argv) {
    (void)argc;
    (void)argv; // keep minimal for now; --help handled in main if needed
    if (argc == 2 && e_strcmp(argv[1], "--help") == 0) {
        lwlog_info("showing help and exiting");
        printf("Usage: ./minishell [arguments]\n");
        printf("Description: A simple shell-like command processor.\n");
        printf(
            "Example: ./minishell \"ls -l | grep 'hello world' > outfile\"\n");
        exit(0);
    }
}

void free_string_array(char **arr) {
    int i = 0;
    if (!arr)
        return;
    while (arr[i] != NULL) {
        e_free(arr[i]);
        i++;
    }
    e_free(arr);
}

char *get_next_token(char **str_ptr) {
    char *str = *str_ptr;
    char *token;
    int i = 0;
    int start = 0;
    char quote_state = 0; /* 0 = no quotes, or holds the current quote char */

    if (!str || !*str)
        return (NULL);

    while (str[i] && is_space(str[i]))
        i++;

    if (str[i] == '\0') {
        *str_ptr = &str[i];
        return (NULL);
    }

    start = i;

    lwlog_debug("get_next_token: starting at offset %ld",
                (long)(str - *str_ptr));
    while (str[i] != '\0') {
        if (str[i] == '\'' || str[i] == '"') {
            if (quote_state == 0)
                quote_state = str[i];
            else if (quote_state == str[i])
                quote_state = 0;
        } else if (quote_state == 0 && is_space(str[i])) {
            break;
        }
        i++;
    }

    int len = i - start;
    token = e_malloc(sizeof(char) * (len + 1));
    if (!token)
        return (NULL);

    for (int j = 0; j < len; j++)
        token[j] = str[start + j];
    token[len] = '\0';

    *str_ptr = &str[i];
    lwlog_debug("get_next_token: returning token '%s'", token);
    return (token);
}

t_cmd *create_node(void) {
    t_cmd *node = e_malloc(sizeof(t_cmd));
    node->args = NULL;
    node->cmd_path = NULL;
    node->fd_in = 0;
    node->fd_out = 1;
    node->next = NULL;
    lwlog_debug("create_node: new node %p", (void *)node);
    return node;
}

void add_token_to_args(t_cmd **current_cmd, char *token) {
    int count = 0;
    while ((*current_cmd)->args && (*current_cmd)->args[count])
        count++;

    char **new_args = e_malloc(sizeof(char *) * (count + 2));
    for (int i = 0; i < count; i++)
        new_args[i] = (*current_cmd)->args[i];
    new_args[count] = token;
    new_args[count + 1] = NULL;

    if ((*current_cmd)->args)
        e_free((*current_cmd)->args);
    (*current_cmd)->args = new_args;
    lwlog_debug("add_token_to_args: added token '%s' to cmd %p", token,
                (void *)(*current_cmd));
}

int count_directories(char *path_str) {
    int count = 1; // Even with 0 colons, there is at least 1 directory
    int i = 0;

    while (path_str[i]) {
        if (path_str[i] == ':') {
            count++;
        }
        i++;
    }
    return count;
}

char **split_path(char *path_str) {

    int dir_count = count_directories(path_str);
    char **directories = e_malloc(sizeof(char *) * (dir_count + 1));
    char *path_copy = e_strdup(path_str);
    int i = 0;
    char *token = e_strtok(path_copy, ":");
    while (token != NULL) {
        directories[i] = e_strdup(token);
        token = e_strtok(NULL, ":");
        i++;
    }

    directories[i] = NULL;
    e_free(path_copy);
    return directories;
}

char *get_cmd_path(char *cmd, char **envp) {

    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0)
            return e_strdup(cmd);
        lwlog_err("get_cmd_path: direct path '%s' not executable", cmd);
        return NULL;
    }

    int i = 0;
    while (envp[i] != NULL && e_strncmp(envp[i], "PATH=", 5) != 0)
        i++;

    if (envp[i] == NULL)
        return NULL;

    char **directories = split_path(envp[i] + 5);

    int j = 0;
    char *full_path;
    while (directories[j] != NULL) {
        full_path = e_malloc(strlen(directories[j]) + 1 + strlen(cmd) + 1);
        sprintf(full_path, "%s/%s", directories[j], cmd);

        if (access(full_path, X_OK) == 0) {
            lwlog_info("get_cmd_path: found executable '%s'", full_path);
            free_string_array(directories);
            return full_path;
        }
        e_free(full_path);
        j++;
    }

    free_string_array(directories);
    lwlog_err("get_cmd_path: command '%s' not found in PATH", cmd);
    return NULL;
}
