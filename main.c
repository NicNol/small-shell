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

int parseInput(char* input, char** command, char** argumentsArray) {

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

        /* Catch variable expansion tokens ("$$"). 
        *   Token value is converted into argument with
        *   $$ variables expanded. */
        argumentLength = getArgumentLength(token);
        if (argNum == 0) {
            command[0] = (char*) malloc(argumentLength + 1);
            expandVariable(token, command[0]);
        }
        else {
            argumentsArray[argNum - 1] = (char*) malloc(argumentLength + 1);
            expandVariable(token, argumentsArray[argNum - 1]);
        }

        token = strtok_r(NULL, whitespace, &saveptr);
        argNum++;
    }
   
   return argNum;
}

void executeInput(char** command, char** argumentsArray, int wordCount) {
    if (strcmp(command[0], "cd") == 0) {

        

        /* If only the command was given, cd to the home directory */
        if (wordCount == 1) {
            setenv("PWD", getenv("HOME"), 1);
        }
        /* Else cd to the first argument directory */
        else {

        }

        return;
    }

    if (strcmp(command[0], "status") == 0) {

        return;
    }

    if (strcmp(command[0], "ls") == 0) {
        printf("PWD is %s\n", getenv("PWD"));
    }

    else {

        return;
    }
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
    char* command[1];
    char* argumentsArray[MAX_ARGUMENT_NUMBER];
    int wordCount;
    

    getInput(input);

    while(strcmp(input, "exit") != 0) {
        
        wordCount = parseInput(input, command, argumentsArray);
        if (wordCount > 0) executeInput(command, argumentsArray, wordCount);
        getInput(input);
    }

    return EXIT_SUCCESS;
}