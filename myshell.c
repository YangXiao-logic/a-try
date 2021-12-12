#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>
#define COMANDLEN 30
#define COMANDMAXLEN 8

// if command is piped, parse it, store piped command into strpiped and return 1, if not, return 0.
int parse_piped_command(char *command, char **strpiped)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        strpiped[i] = strsep(&command, "|");
        if (strpiped[i] == NULL)
            break;
    }
    if (strpiped[1] == NULL)
        return 0;
    else
        return 1;
}

void split_command(char *command, char **splited_command)
{
    int i;
    for (i = 0; i < COMANDMAXLEN; i++)
    {
        splited_command[i] = strsep(&command, " ");
        if (splited_command[i] == NULL)
            break;
        if (strlen(splited_command[i]) == 0)
            i--;
    }
}

int parse_command(char *command, char **parsed_command, char **parsed_pipe_command)
{
    char *strpiped[2];
    int is_pipe = 0;
    is_pipe = parse_piped_command(command, strpiped);

    if (is_pipe)
    {
        split_command(strpiped[0], parsed_command);
        split_command(strpiped[1], parsed_pipe_command);
    }
    else
    {
        split_command(command, parsed_command);
    }
    return is_pipe;
}

int built_in_command(char **parsed_command)
{
    if (strcmp(parsed_command[0], "info") == 0)
    {
        printf("COMP2211 Simplified Shell by YangXiao\n");
        return 1;
    }
    else if (strcmp(parsed_command[0], "pwd") == 0)
    {
        char current_working_dir[1024];
        getcwd(current_working_dir, sizeof(current_working_dir));
        printf("%s\n", current_working_dir);
        return 1;
    }
    else if (strcmp(parsed_command[0], "cd") == 0)
    {
        chdir(parsed_command[1]);
        return 1;
    }
    else if (strcmp(parsed_command[0], "mygrep") == 0)
    {
        FILE *fp;
        char temp[200];
        fp = fopen(parsed_command[2], "r");
        while (!feof(fp))
        {
            fgets(temp, 1000, fp);
            if (strstr(temp, parsed_command[1]))
            {
                printf("%s", temp);
            }
        }
        return 1;
    }
    return 0;
}

void exec_system_command(char **parsed_command)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("\nFail to fork child.");
    }
    else if (pid == 0)
    {
        printf("1\n");
        printf("%s\n", parsed_command[0]);
        if (strcmp(parsed_command[0], "ex") == 0)
        {
            printf("%s\n", parsed_command[2]);
            if (strcmp(parsed_command[2], ">>") == 0)
            {
                printf("Child(pid:%d)\n", (int)getpid());
                close(STDOUT_FILENO);
                char *output_file = strdup("./");
                strcat(output_file, parsed_command[3]);
                open(output_file, O_CREAT | O_WRONLY | O_APPEND, 0600);
                char *myargs[3];
                myargs[0] = strdup("./");
                strcat(myargs[0], parsed_command[1]);
                myargs[1] = NULL;
                if (execvp(myargs[0], myargs) < 0)
                {
                    printf("\nCould not execute command.");
                }
            }
            else
            {
                printf("here\n");
                char *myargs[3];
                myargs[0] = strdup("./");
                strcat(myargs[0], parsed_command[1]);
                myargs[1] = strdup(parsed_command[2]);
                myargs[2] = NULL;
                if (execvp(myargs[0], myargs) < 0)
                {
                    printf("\nCould not execute command.");
                }
            }
        }
    }
    else
    {
        int wc = wait(NULL);
        printf("Parent of %d(wc:%d)(pid:%d)\n", wc, pid, (int)getpid());
    }
}

void exec_system_pipe_command(char **parsed_command, char **parsed_pipe_command)
{
    int pipefd[2];
    pid_t pid_1, pid_2;
    if (pipe(pipefd) < 0)
    {
        printf("\nPipe could not be initialized");
        return;
    }
    // int inFd = dup(STDIN_FILENO);
    // int outFd = dup(STDOUT_FILENO);
    pid_1 = fork();
    if (pid_1 < 0)
    {
        printf("\nFail to fork.");
        return;
    }
    else if (pid_1 == 0)
    {
        printf("33");
        printf("%s", parsed_command[1]);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        // write(pipefd[1], wr)
        close(pipefd[1]);
        char *myargs[2];
        myargs[0] = strdup("./");
        strcat(myargs[0], parsed_command[1]);
        myargs[1] = NULL;

        if (execvp(myargs[0], myargs) < 0)
        {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    }
    else
    {
        // wait(NULL);
        pid_2 = fork();

        if (pid_2 < 0)
        {
            printf("\nFail to fork.");
            return;
        }
        else if (pid_2 == 0)
        {
            printf("44");
            printf("%s", parsed_pipe_command[0]);
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            // open(STDOUT_FILENO);

            char *myargs[2];
            myargs[0] = strdup("./");
            strcat(myargs[0], parsed_pipe_command[1]);
            myargs[1] = NULL;

            if (execvp(myargs[0], myargs) < 0)
            {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        }
        else
        {
            wait(NULL);
            // wait(NULL);
        }
        
        
        // ex pipefile1 | ex pipefile2
    }
    // waitpid(pid_1,0,0);
    // waitpid(pid_2,0,0);
    // wait(NULL);
}

void main()
{
    char command[COMANDLEN];
    char *parsed_command[COMANDMAXLEN];
    char *parsed_pipe_command[COMANDMAXLEN];
    int is_pipe;
    int is_built_in_command;
    while (1)
    {

        printf("myshell$$");
        fgets(command, COMANDLEN, stdin);
        int i = 0;
        while (command[i] != '\n')
        {
            i++;
        }
        command[i] = '\0';
        is_pipe = parse_command(command, parsed_command, parsed_pipe_command);
        if (strcmp(parsed_command[0], "exit") == 0)
            break;
        is_built_in_command = built_in_command(parsed_command);
        if (is_built_in_command == 0)
        {
            if (is_pipe == 1)
                exec_system_pipe_command(parsed_command, parsed_pipe_command);
            else if (is_pipe == 0)
                exec_system_command(parsed_command);
        }
    }
    // printf("%s", command);
}