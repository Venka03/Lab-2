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
#include <stdbool.h>

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


bool is_numeric(char *str){
    /*
    check if string is numerical or not
    meaning if string(array of chars) contain integer number or not
    */
    if (str[0] == '\0')
        return false;
    int i = 0;
    if (str[0] == '-'){ // for case of negative number
        if (str[1] == '\0')
            return false;
        i++; // move staring position of checking numbers
    }
        
    for (; str[i]!='\0'; i++){ // read till the end of string(array of chars)
        if (str[i] < '0' || str[i] > '9')
            return false;
    }
    return true;
}

int mycalc(char **argvv){
    /*
    Controls the execution of internal command mycalc
    return -1 in case any error occurs during the execution
    */
    int result, a, b, i;
    for (i=0; argvv[i]!=NULL; i++); // count the amount of parameters
    if (i != 4){
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
        return -1;
    }
        
    if (!is_numeric(argvv[1]) || !is_numeric(argvv[3])){ // checks if non numerical values are passes in positions where numbers are supposed to be
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
        return -1;
    }
    
    a = atoi(argvv[1]);
    b = atoi(argvv[3]);
    if (!strcmp(argvv[2], "add")){ // branch execution for different operations passed
        result = a + b; // make this part look better
        char *path = getenv("Acc");
        int acc = atoi(path);
        acc += result;
        sprintf(path, "%d", acc);
        setenv("Acc", path, 1);
        fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", a, b, result, acc);
    }
    else if (!strcmp(argvv[2], "mul")){
        result = a * b;
        fprintf(stderr, "[OK] %d * %d = %d\n", a, b, result);

    }
    else if (!strcmp(argvv[2], "div")){
        if (b == 0){
            printf("[ERROR] Division by zero cannot be performed\n");
            return -1;
        }
        else {
            result = a / b;
            fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", a, b, result, a % b);
        }
    }
    else { // for operations that are not supposed to be passed in mycalc
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
        return -1;
    } 
    return 1; // mean that there was not problem in execution of the command
}

/* myhistory */
void print_command(struct command cmd){
    /*
    print the command(sequence of commands) with redirections and background execution mark(&)
    */
    for (int n=0; n<cmd.num_commands; n++){ 
        for (int m=0; m<cmd.args[n]; m++){
            fprintf(stderr, "%s ", cmd.argvv[n][m]); // print name of command and all parameters passed
        }
        if (n != cmd.num_commands -1)
            fprintf(stderr,"| "); // for sequences of command
    }
    // print is there are any file redirections and execution in background
    if (strcmp(cmd.filev[0], "0") !=0)
        fprintf(stderr, "< %s ", cmd.filev[0]);
    if (strcmp(cmd.filev[1], "0") !=0)
        fprintf(stderr, "> %s ", cmd.filev[1]);
    if (strcmp(cmd.filev[2], "0") !=0)
        fprintf(stderr, "!> %s ", cmd.filev[2]);
    if (cmd.in_background == 1)
        fprintf(stderr, "&");
    fprintf(stderr, "\n");
}

int myhistory(char **argvv, int *run_history){
    /*
    control the execution of myhistory commands
    if no parameters are passed, print 20 last commands executed
    if one parameter is passed, executes the command from history with given index (from 0 to 19)
    otherwise return -1 to indicate problem
    */
    int i;
    for (i=0; argvv[i]!=NULL; i++); // counts amounf of parameters
    if (i == 1){ // in case no additional parameters are passed, myhistory should print 20 last command executed
        int k;
        for (int j=0; j!=n_elem; j++){
            fprintf(stderr, "%d ", j);
            k = tail + j; 
            // since data is circular, values of tail and head can be any number from 0 to 19, so remainder of division sometimes is needed
            if (k >= 20)
                k -= 20;
            print_command(history[k]);
        }
        return 1;
    }
    else if (i == 2) { // for the case when command from history is neede to be executed
        if (!is_numeric(argvv[1])){
            fprintf(stdout, "ERROR: Command not found\n");
            return -1;
        }
        *run_history = atoi(argvv[1]);
        if (*run_history < 0 || *run_history >= n_elem){
            fprintf(stdout, "ERROR: Command not found\n");
            *run_history = 0; // to indicate that next program is not taken from history
            return -1; // to indicate that there was an error in execution
        }
        // is increased to have numbers from 1, so to indicate that next command should be executed from history
        *run_history = *run_history + 1;
        return 1;
            
    }
    fprintf(stdout, "ERROR: Command not found\n");
    return -1;
}

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
    (*cmd).num_commands = num_commands-1;
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

int execute_command(char ***argvv, char filev[3][64], int in_background, int command_counter, int *run_history){
    /*
    execute, in background or not, command or sequence of commands with parameters passed, given redirections. 
    differ the execution of internal commands from common, since they are not executed in background 
    as well as redirectorion of input and output and being in sequence 

    control the execution of commands, is any error occurs during the execution, 
    user is informed about the error and -1 is return

    */
    if (strcmp(argvv[0][0], "mycalc") == 0){ 
        if (strcmp(filev[0], "0") != 0 || strcmp(filev[1], "0") != 0 || strcmp(filev[2], "0") != 0){ // check for file redirections
            fprintf(stdout, "mycalc cannot have file redirections.");
            return -1;
        }
        else if (command_counter > 1){ // check if mycalc is first in sequence of commands
            fprintf(stdout, "mycalc cannot be part of the command sequences.");
            return -1;
        }
        else if (in_background == 1){
            fprintf(stdout, "mycalc cannot be executed in background.");
            return -1;
        }
        else 
            mycalc(argvv[0]);
    }
    else if (strcmp(argvv[0][0], "myhistory") == 0){
        if (strcmp(filev[0], "0") != 0 || strcmp(filev[1], "0") != 0 || strcmp(filev[2], "0") != 0){ // check for file redirections
            fprintf(stdout, "myhistory cannot have file redirections.");
            return -1;
        } 
        else if (command_counter > 1) { // check if myhistory is first in sequence of commands
            fprintf(stdout, "myhistory cannot be part of the command sequences.");
            return -1;
        }
        else if (in_background == 1) {
            fprintf(stdout, "myhistory cannot be executed in background.");
            return -1;
        }
        else {
            myhistory(argvv[0], run_history);
        }
    }
    else {
        int status = 0; 
        int pid, fde;
        if (strcmp(filev[2], "0") != 0){ // if there is error output to file, open that file
            fde = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fde == -1){
                perror("Error in open");
                return -1;
            }
        }
        int fd[command_counter-1][2];
        for (int i=0; i<command_counter; i++){
            
            if (strcmp(argvv[0][0], "mycalc") == 0 || strcmp(argvv[0][0], "myhistory") == 0){
                perror("[Error] internal commands mycalc and myhistory cannot be part of sequence\n");
                return -1;
            }
            
            if (command_counter > 1) // no need to create pipe for simple command
                pipe(fd[i]);
            pid = fork(); // child is created for execution of command
            switch (pid){
                case -1:
                    perror("Error in fork");
                    return -1;
                case 0:
                    if (i == 0){
                        if (strcmp(filev[0], "0") != 0){ // if there is input from file, table is changed so input is taken 
                            int fdi = open(filev[0], O_RDONLY);
                            if (fdi == -1){
                                perror("Error in open");
                                return -1;
                            }
                            close(STDIN_FILENO);
                            if (dup2(fdi, STDIN_FILENO) == -1){ // take input from the file
                                perror("Error in dup2");
                                return -1;
                            }
                        }
                    }
                    else {
                        close(STDIN_FILENO);
                        dup(fd[i-1][STDIN_FILENO]); // receive input by one pipe
                        close(fd[i-1][STDOUT_FILENO]);
                    }
                    if (i == command_counter-1){
                        if (strcmp(filev[1], "0") != 0){ // if there is  output to file, table is changed so to write output to file
                            int fdo = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                            if (fdo == -1) {
                                perror("Error in open");
                                return -1;
                            }
                            close(STDOUT_FILENO);
                            if (dup2(fdo, STDOUT_FILENO) == -1){ // set output to file
                                perror("Error in dup2");
                                return -1;
                            }
                        }
                    }
                    else {
                        close(STDOUT_FILENO);
                        dup(fd[i][STDOUT_FILENO]); // send output by another pipe
                        close(fd[i][STDIN_FILENO]);
                    }
                    
                    if (strcmp(filev[2], "0") != 0){ // if there is error output to file, table is changed so to write errors to file
                        close(STDERR_FILENO);
                        if (dup2(fde, STDERR_FILENO) == -1){ // set error output to file
                            perror("Error in dup2");
                            return -1;
                        }
                    }
                    execvp(argvv[i][0], argvv[i]);
                    perror("Error in execvp"); 
                    return -1;
                
                default:
                    if (i > 0){
                        close(fd[i-1][STDIN_FILENO]);
                        close(fd[i-1][STDOUT_FILENO]);
                    }
                    if (!in_background){
                        // wait for child to finish if no execution in background
                        if (waitpid(pid, &status, 0) == -1){ 
                            perror("Error in wait");
                            return -1;
                        }
                    }
                    else if (i == command_counter -1)
                        fprintf(stdout, "[%d]\n", pid); // print pid of child if executed in background
            }
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

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;
    setenv("Acc", "0", 1); // set envirnmental variable

	while (1) 
	{
        struct command cmd;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history)
        {
            // copy data of command that should be executed from the history
            argvv = cmd.argvv; 
            memcpy(filev, cmd.filev, 3 * 64 * sizeof(char));
            in_background = cmd.in_background;
            command_counter = cmd.num_commands;
            fprintf(stderr, "Running command %d\n", --run_history);
            run_history=0;
        }
        else{
            // Prompt 
            write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

            // Get command
            //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
            executed_cmd_lines++;
            if (end != 0 && executed_cmd_lines < end) {
                command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
            }
            else if (end != 0 && executed_cmd_lines == end)
                return 0;
            else
                command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
        }
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
        
        
        
        execute_command(argvv, filev, in_background, command_counter, &run_history);
        if (run_history){
            run_history--; // since it was intentionally increase by one in execute_command, decrease it to work with it
            // compute position of needed command in array
            run_history = (tail + run_history ) % 20; // since array is made to be circular, positions should be from 0 to 19
            // save command that should be executed at the next iteration
            store_command(history[run_history].argvv, history[run_history].filev, history[run_history].in_background, &cmd); 
            
            run_history++; // to have in range of 1 to 20, to make non zero value indicate that command is taken from history
        }

        if (n_elem == 0){
            store_command(argvv, filev, in_background, &history[head]); // even though command can be wrong, we still store it like real command
            n_elem++;
        }
        else if (n_elem < history_size){
            head++; // position of last entered command
            store_command(argvv, filev, in_background, &history[head]);
            n_elem++;
        }
        else {
            head = (head + 1) % 20;
            tail = (tail + 1) % 20;
            store_command(argvv, filev, in_background, &history[head]);
        }
	}
	
	return 0;
}
