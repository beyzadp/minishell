#include "constants.h"
#include "lwlog.h"
#include "minishell.h"
#include <signal.h>
#include <stdio.h>

// A simplified mental model of a custom tokenizer

#include <stdlib.h>

/* Lightweight helpers live in utils.c */

void process_token(char *token, t_cmd **current_cmd, char **envp,
                   char **tracker) {
    if (strcmp(token, "|") == 0) {
        (*current_cmd)->next = create_node();
        *current_cmd = (*current_cmd)->next;
        e_free(token);
    } else if (strcmp(token, "<") == 0) {
        char *file_name = get_next_token(tracker); // Grab the actual file name
        if (file_name) {
            if ((*current_cmd)->fd_in != 0 && (*current_cmd)->fd_in != -1) {
                close((*current_cmd)->fd_in);
            }
            (*current_cmd)->fd_in = open(file_name, O_RDONLY, 0644);
            if ((*current_cmd)->fd_in == -1) {
                perror(file_name);
            }
            e_free(file_name); // Free the string after using it
        }
        e_free(token); // Free the operator token
    } else if (strcmp(token, ">") == 0) {
        char *file_name = get_next_token(tracker);
        if (file_name) {
            // If fd_out isn't the default STDOUT (1), close the old file!
            if ((*current_cmd)->fd_out != 1 && (*current_cmd)->fd_out != -1) {
                close((*current_cmd)->fd_out);
            }
            // 0644 is standard standard shell file permission (rw-r--r--)
            (*current_cmd)->fd_out =
                open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if ((*current_cmd)->fd_out == -1) {
                perror(file_name);
            }
            e_free(file_name);
        }
        e_free(token);
    } else {
        add_token_to_args(current_cmd, token);
        if ((*current_cmd)->cmd_path == NULL) {

            (*current_cmd)->cmd_path = get_cmd_path(token, envp);
        }
    }
}

void print_cmd_list(t_cmd *head) {
    t_cmd *current = head;
    while (current) {
        printf("Command: %s\n", current->cmd_path);
        printf("Arguments: ");
        if (current->args) {
            for (int i = 0; current->args[i] != NULL; i++) {
                printf("%s ", current->args[i]);
            }
        }
        printf("\nInput FD: %d, Output FD: %d\n", current->fd_in,
               current->fd_out);
        printf("----\n");
        current = current->next;
    }
}

int execute_commands(t_cmd *head, char **envp) {

    if (head == NULL) {
        lwlog_err("execute_commands: command list is empty");
        return 1;
    }

    int prev_fd_in = -1;

    while (head != NULL) {
        if (head->cmd_path == NULL) {
            lwlog_err("execute_commands: command has no path");
            return 1;
        }
        if (head->args == NULL) {
            lwlog_err("execute_commands: command has no arguments");
            return 1;
        }
        if (envp == NULL) {
            lwlog_err("execute_commands: environment variables are NULL");
            return 1;
        }

        int pipefd[2];

        if (head->next) {
            if (pipe(pipefd) == -1) {
                lwlog_err("execute_commands: failed to create pipe");
                return 1;
            }
        }

        // fork a child process for the current command

        pid_t pid = fork();
        if (pid == -1) {
            lwlog_err("execute_commands: failed to fork");
            return 1;
        } else if (pid == 0) {
            // In child process
            if (head->fd_in != 0) {
                if (dup2(head->fd_in, STDIN_FILENO) == -1) {
                    lwlog_err("execute_commands: failed to redirect input");
                    exit(1);
                }
            } else if (prev_fd_in != -1) {
                dup2(prev_fd_in, STDIN_FILENO);
            }

            if (head->fd_out != 1) {
                if (dup2(head->fd_out, STDOUT_FILENO) == -1) {
                    lwlog_err("execute_commands: failed to redirect output");
                    exit(1);
                }
            } else if (head->next) {
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    lwlog_err("execute_commands: failed to redirect output "
                              "to pipe");
                    exit(1);
                }
                // Close the read end of the pipe in the child process
                close(pipefd[0]);
            }
            execve(head->cmd_path, head->args, envp);
        }

        // in the parent process

        if (prev_fd_in != -1) {
            close(prev_fd_in);
        }
        if (head->next) {
            // Close the write end of the pipe in the parent process
            close(pipefd[1]);
            prev_fd_in = pipefd[0];
        }
        head = head->next;
    }

    while (wait(NULL) > 0)
        ;

    return 0;
}

int main(int argc, char **argv, char **envp) {

    signal(SIGINT, signal_handler);

    // startup
    lwlog_info("minishell starting");

    // to handle arguments
    argcv_handler(argc, argv);
    lwlog_debug("argc=%d", argc);

    // get the input
    e_printf("\n$> ");
    char *input = e_malloc(sizeof(char) * 100);
    if (!input) {
        lwlog_err("failed to allocate input buffer");
        return 1;
    }
    if (fgets(input, 100, stdin) == NULL) {
        lwlog_err("failed to read input from stdin");
        e_free(input);
        return 1;
    }

    char *tracker = input;

    t_cmd *cmd_list_head = create_node();
    t_cmd *current_cmd = cmd_list_head;

    // tokenize the input and build the linked list of commands
    char *token = get_next_token(&tracker);

    while (token != NULL) {
        lwlog_debug("processing token %s", token);

        // process the token and update the current_cmd accordingly
        process_token(token, &current_cmd, envp, &tracker);
        // token has processed and added to the current_cmd, now get the
        // next token
        token = get_next_token(&tracker);
    }

    e_free(input);
    lwlog_info("built command list, printing for debug");
    print_cmd_list(cmd_list_head);

    lwlog_info("executing commands");
    if (execute_commands(cmd_list_head, envp) != 0) {
        lwlog_err("failed to execute commands");
        return 1;
    }
    // its time to fork a child and execute the commands in the linked list

    /*
    read commands
    tokenize the input to separate the commands and their arguments, as well
    as the operators (<, >, |)

    Example Input: ls -l | grep "hello world" > outfile
    Tokens: [ls], [-l], [|], [grep], ["hello world"], [>], [outfile]

    parse the commands and put them in a linked list.

    start from the head
    if theres a next command that means we need a pipe
    call pipe() to create a pipe
    fork a child process for the current command
    in the child process:
        if theres a next command:
            redirect the standard output to the write end of the pipe using
    dup2()
        if theres an input redirection:
            (<) redirect the standard input to the file using dup2()
        if theres an output redirection: (>)
            redirect the standard output to the file using dup2() execute
    the command using execve() in the parent process: if theres a next
    command: redirect the standard input to the read end of the pipe using
    dup2() wait for the child process to finish using waitpid() move to the
    next command in the linked list and repeat the process until all
    commands are executed






    add variable expansion

    add signal handling for SIGINT and SIGQUIT






    */

    return 0;
}
