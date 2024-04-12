//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}

int mycalc(char **argvv, int *acc){
    int result, a, b;
    if (!is_numeric(argvv[1]))
        return -1;
    a = atoi(argvv[1]);
    if (!is_numeric(argvv[3]))
        return -1;
    b = atoi(argvv[3]);
    if (!strcmp(argvv[2], "add")){
        result = a + b;
        *acc = *acc + result;
        printf("[OK] %d + %d = %d; Acc %d\n", a, b, result, *acc);
    }
    else if (!strcmp(argvv[2], "mul")){
        result = a * b;
        printf("[OK] %d * %d = %d\n", a, b, result);

    }
    else if (!strcmp(argvv[2], "div")){
        if (b == 0){
            printf("[ERROR] Division by zero cannot be performed\n");
            return -1;
        }
        else {
            result = a / b;
            printf("[OK] %d / %d = %d; Remainder %d\n", a, b, result, a % b);
        }
    }
    else 
        return -1;
    return 1;
}

/* myhistory */

/* myhistory */

struct command
{
    // Store the number of commands in argvv
    int num_commands;
    // Store the number of arguments of each command
    int *args;
    // Store the commands
    char ***argvv;
    // Store the I/O redirection
    char filev[3][64];
    // Store if the command is executed in background or foreground
    int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1; // why do we subtracts?
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}


/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;
    int Acc = 0;

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;

	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history)
        {
            run_history=0;
            // add code here
        }
        else{
            // Prompt 
            write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

            // Get command
            //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
            executed_cmd_lines++;
            if( end != 0 && executed_cmd_lines < end) {
                command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
            }
            else if( end != 0 && executed_cmd_lines == end)
                return 0;
            else
                command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
        }
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
        

        if (strcmp(argvv[0][0], "mycalc") == 0){
            if (strcmp(filev[0], "0") != 0 || strcmp(filev[1], "0") != 0 || strcmp(filev[2], "0") != 0)
                printf("mycalc cannot have file redirections.")
            // do other tests
            else 
                mycalc(argvv[0], &Acc);
        }
        else {
            int fdi, fdo, fde;
            if (strcmp(filev[0], "0") != 0){ // input file
                fdi = open(filev[0], O_RDONLY);
                if (fdi == -1){
                    perror("Error in open");
                    return -1;
                }
            }
            if (strcmp(filev[1], "0") != 0){ // output file
                fdo = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fdo == -1){
                    perror("Error in open");
                    return -1;
                }
            }
            if (strcmp(filev[2], "0") != 0){ // error file
                fde = open(filev[2], O_WRONLY | O_CREAT, 0666);
                if (fde == -1){
                    perror("Error in open");
                    return -1;
                }
            }
            int pid;
            if (command_counter == 1){
                pid = fork();
                switch (pid){
                    case -1:
                        perror("Error in fork");
                        return -1;
                    case 0:
                        if (strcmp(filev[0], "0") != 0){ // input file
                            close(STDIN_FILENO);
                            if (dup2(fdi, STDIN_FILENO) == -1){
                                perror("Error in dup2");
                                return -1;
                            }
                        }
                        if (strcmp(filev[1], "0") != 0){ // output file
                            close(STDOUT_FILENO);
                            if (dup2(fdo, STDOUT_FILENO) == -1){
                                perror("Error in dup2");
                                return -1;
                            }
                        }
                        if (strcmp(filev[2], "0") != 0){ // error file
                            close(STDERR_FILENO);
                            if (dup2(fde, STDERR_FILENO) == -1){
                                perror("Error in dup2");
                                return -1;
                            }
                        }
                        execvp(argvv[0][0], argvv[0]);
                        perror("Error in execvp");
                        return -1;
                    default:
                        if (!in_background){
                            if (wait(&status) == -1){
                                perror("Error in wait");
                                return -1;
                            }
                        }
                }
            }
            else {
            int fd[command_counter-1][2];
            pipe(fd[0]); 
            pid = fork();
            switch(pid){
                case -1:
                    perror("Error in fork");
                    return -1;
                case 0:
                    if (strcmp(filev[0], "0") != 0){ // input file
                        close(STDIN_FILENO);
                        if (dup2(fdi, STDIN_FILENO) == -1){
                            perror("Error in dup2");
                            return -1;
                        }
                    }
                    if (strcmp(filev[2], "0") != 0){ // error file
                        close(STDERR_FILENO);
                        if (dup2(fde, STDERR_FILENO) == -1){
                            perror("Error in dup2");
                            return -1;
                        }
                    }
                    close(STDOUT_FILENO); 
                    dup(fd[0][STDOUT_FILENO]);
                    close(fd[0][STDIN_FILENO]);
                    execvp(argvv[0][0], argvv[0]);
                    perror("Error in execvp");
                    return -1;
                default:
                    if (!in_background){
                        if (wait(&status) == -1){
                            perror("Error in wait");
                            return -1;
                        } 
                    }
                      
            }

            for (int i=1; i<command_counter-1;i++){
                pipe(fd[i]);
                pid = fork();
                switch (pid){
                    case -1:
                        perror("Error in fork");
                        return -1;
                    case 0:
                        close(STDIN_FILENO);
                        dup(fd[i-1][STDIN_FILENO]); // receive by one pipe
                        close(fd[i-1][STDOUT_FILENO]);

                        close(STDOUT_FILENO);
                        dup(fd[i][STDOUT_FILENO]); // send by another pipe
                        close(fd[i][STDIN_FILENO]);
                        if (strcmp(filev[2], "0") != 0){ // error file
                            close(STDERR_FILENO);
                            if (dup2(fde, STDERR_FILENO) == -1){
                                perror("Error in dup2");
                                return -1;
                            }
                        }
                        execvp(argvv[i][0], argvv[i]);
                        perror("Error in execvp");
                        return -1;
                    default:
                        close(fd[i-1][STDIN_FILENO]);
                        close(fd[i-1][STDOUT_FILENO]);
                    	if (!in_background){
                            if (wait(&status) == -1){
                                perror("Error in wait");
                                return -1;
                            }
                        }
                }
            }

            pid = fork();
            switch(pid){
                case -1:
                    perror("Error in fork");
                    return -1;
                case 0:
                    if (strcmp(filev[1], "0") != 0){ // output file
                        close(STDOUT_FILENO);
                        if (dup2(fdo, STDOUT_FILENO) == -1){
                            perror("Error in dup2");
                            return -1;
                        }
                    }
                    if (strcmp(filev[2], "0") != 0){ // error file
                        close(STDERR_FILENO);
                        if (dup2(fde, STDERR_FILENO) == -1){
                            perror("Error in dup2");
                            return -1;
                        }
                    }
                    close(STDIN_FILENO); 
                    dup(fd[command_counter-2][STDIN_FILENO]);
                    close(fd[command_counter-2][STDOUT_FILENO]); 
                    execvp(argvv[command_counter-1][0], argvv[command_counter-1]);
                    perror("Error in execvp");
                    return -1;
                
                default:
                    close(fd[command_counter-2][STDIN_FILENO]);
                    close(fd[command_counter-2][STDOUT_FILENO]);
                    if (!in_background){
                        if (wait(&status) == -1){
                            perror("Error in wait");
                            return -1;
                        }
                    }
                    
                
            }
        }
        }



        //************************************************************************************************
        printf("Command ended\n");
        /*
        if (command_counter > 0) {
                if (command_counter > MAX_COMMANDS){
                    printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
                }
                else {
                    // Print command
                    print_command(argvv, filev, in_background);
                }
            }
        }
        */
    }
    return 0;

}
