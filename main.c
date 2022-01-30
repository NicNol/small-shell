/*
*   Author: Nic Nolan
*   Last Modified: 01/22/2022
*   OSU Email Address: nolann@oregonstate.edu
*   Course Number: CS344
*   Assignment Number: #3
*   Due Date: 02/07/2022
*   Description: This program does the following:
*       1) 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 2048
#define MAX_ARGUMENT_NUMBER 512

/* Check for variables that need to be expanded from "$$" to the process ID */
int getArgumentLength(char* argument) {

    /* Declare and Initialize variables */
    pid_t pid = getpid();
    char pidString[20];
    sprintf(pidString, "%d", pid);
    int pidLength = strlen(pidString);
    int variableCount = 0;
    int i;

    /* Count instances of $$ that must be expanded */
    for(i = 0; i < strlen(argument) - 1; i++) {
        if(argument[i] == '$' && argument[i + 1] == '$') {
            variableCount++;
            i++;
        }
    }

    /* Calculate argument length */
    return strlen(argument) - (2 * variableCount) + (pidLength * variableCount);
}

void expandVariable(char* token, char* argument) {
    /* Set up and declare variables */
    int t_i; // token index
    int a_i = 0; // argument index
    pid_t pid = getpid();
    char pidString[20];
    sprintf(pidString, "%d", pid);
    int pidLength = strlen(pidString);

    /* Parse token for "$$" variable */

    for (t_i = 0; t_i < strlen(token); t_i++) {

        /* Copy pid to argument if "$$" is found */
        if(token[t_i] == '$' && token[t_i+1] == '$') {
            argument[a_i] = '\0';
            strcat(argument, pidString);
            t_i++;
            a_i += pidLength;
        }
        /* Else, just copy the token char to argument */
        else {
            argument[a_i] = token[t_i];
            a_i++;
        }
    }
}

/*
*   Solicit input from the user.
*   
*   Input: input -- a char* where the input string will be stored.
*   Output: None. 
*
* input method adopted from: https://stackoverflow.com/questions/30939517/loop-through-user-input-with-getchar/30939585
* accessed 1/27/2022
*/
void getInput(char* input) {
    
    /* Set up initial variables */
    int ch;
    int offset = 0;

    /* Get user input */
    printf(": ");

    while((ch = getchar()) != '\n') {
        input[offset] = ch;
        offset++;
    }

    /* Put a terminating character on the input*/
    input[offset] = '\0';
}

int parseInput(char* input, char** argumentsArray) {

    /* Set up string token and while loop*/
    char whitespace[] = " \t\n\v\f\r";
    char* saveptr;
    char* token = strtok_r(input, whitespace, &saveptr);
    int argNum = 0;
    int argumentLength;
    
    /* Save language in currentMovie while token is not NULL */
    while (token)
    {
        
        /* Catch comments */
        if ((argNum == 0) && (token[0] == '#')) {
            return 0;
        }

        /* Copy token to argumentsArray and expand any "$$" variables in token */
        argumentLength = getArgumentLength(token);
        argumentsArray[argNum] = (char*) malloc(argumentLength + 1);
        expandVariable(token, argumentsArray[argNum]);

        /* Set up for next loop */
        token = strtok_r(NULL, whitespace, &saveptr);
        argNum++;
    }
   
    /* Terminate argumentsArray with a NULL pointer so we know when the list ends */
    argumentsArray[argNum] = NULL;

   return argNum;
}

void changeDirectory(char* location, int wordCount) {

    /* If only the command was given, cd to the home directory */
    if (wordCount == 1) {
        setenv("PWD", getenv("HOME"), 1);
        return;
    }

    /* If first char in location path is '/', then path is absolute */
    if (location[0] == '/') {
        setenv("PWD", location, 1);
        return;
    }

    /* Else, path is relative  */
    /* Append location path to current PWD */
    int newPwdLength = strlen(getenv("PWD")) + strlen(location) + 2; // add a '/' seperator and '\0' at end
    char newPwd[newPwdLength];
    strcpy(newPwd, getenv("PWD"));
    strcat(newPwd, "/\0");
    strcat(newPwd, location);

    /* Set env with new PWD */
    setenv("PWD", newPwd, 1);
    return;
}

/*
  Create a child process using fork()
*/
int createNewProcess(char** argumentsArray, int wordCount){

    /* Declare and initialize variables */
	int childStatus;
	pid_t spawnPid = fork();
    char pidString[20];
    char pidStatusString[20];
    bool backgroundProcess = (strcmp(argumentsArray[wordCount - 1], "&") == 0);

    /* Create a switch case to catch errors, child process, and parent process */
	switch(spawnPid){
        
        /* Error Case */
        case -1:
            perror("fork()\n");
            exit(1);
            break;
        
        /* Child Case */
        case 0:
            execvp(argumentsArray[0], argumentsArray);
            perror("execvp");
            exit(2);
            break;
        
        /* Parent Case */
        default:
        
            /* Wait for child process to terminate */
            spawnPid = waitpid(spawnPid, &childStatus, 0);

            

            if (!backgroundProcess) {

                /* Convert spawnPID and its exit status to string*/
                sprintf(pidString, "%d", spawnPid);

                /* Check exit status */ 
                if(WIFEXITED(childStatus)) {
                    /* Exited normally */
                    sprintf(pidStatusString, "%d", WEXITSTATUS(childStatus));
                }
                else {
                    /* Exited abnormally */
                    sprintf(pidStatusString, "%d", WTERMSIG(childStatus));
                }

                /* Set spawnPID and its status to environment variable */
                setenv("spawnPID", pidString, 1);
                setenv("spawnPIDStatus", pidStatusString, 1);
            }
	} 
}

void executeInput(char** argumentsArray, int wordCount) {
    char* command = argumentsArray[0];

    if (strcmp(command, "cd") == 0) {
        return changeDirectory(argumentsArray[1], wordCount);
    }

    if (strcmp(command, "status") == 0) {
        char* pidString = getenv("spawnPID"); // if no spawnPID, returns NULL
        char* pidStatusString = getenv("spawnPIDStatus");

        if (pidString == NULL) {
            printf("exit status 0\n");
            return;
        }

        printf("PID %s exited with status %s\n", pidString, pidStatusString);
        return;
    }

    else {
        createNewProcess(argumentsArray, wordCount);
        return;
    }
}

void cleanup() {
    unsetenv("spawnPID");
    unsetenv("spawnPIDStatus");
}

/*
*   Driver of the program (main).
*
*   Handles the main menu and file process menu choices.
*
*   Compile the program as follows:
*       gcc --std=gnu99 -o movies_by_year main.c movies.h
*
*   Run the file by calling:
*       ./movies_by_year
* input adopted from: https://stackoverflow.com/questions/30939517/loop-through-user-input-with-getchar/30939585
* accessed 1/27/2022
*/
int main(void) {

    /* Declare Variables */
    char input[MAX_INPUT_LENGTH];
    char* argumentsArray[MAX_ARGUMENT_NUMBER];
    int wordCount;
    

    getInput(input);

    while(strcmp(input, "exit") != 0) {
        
        wordCount = parseInput(input, argumentsArray);
        if (wordCount > 0) executeInput(argumentsArray, wordCount);
        getInput(input);
    }

    cleanup();
    return EXIT_SUCCESS;
}