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
    char* pidString;
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
    char* pidString;
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

void parseInput(char* input) {

    /* Set up string token and while loop*/
    char whitespace[] = " \t\n\v\f\r";
    char* saveptr;
    char* token = strtok_r(input, whitespace, &saveptr);
    char* argument;
    int argNum = 0;
    int argumentLength;

    /* Save language in currentMovie while token is not NULL */
    while (token)
    {
        
        /* Catch comments */
        if ((argNum == 0) && (token[0] == '#')) {
            return;
        }

        /* Catch variable expansion tokens ("$$") */
        argumentLength = getArgumentLength(token);
        if (argNum == 0) {
            argument = (char*) malloc(argumentLength);
        } else {
            argument = (char*) realloc(argument, argumentLength);
        }
        
        expandVariable(token, argument);

        printf("%s\n", argument);
        token = strtok_r(NULL, whitespace, &saveptr);
        argNum++;
    }

    free(argument);
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

    char input[MAX_INPUT_LENGTH];
    

    getInput(input);

    while(strcmp(input, "exit") != 0) {
        
        parseInput(input);
        getInput(input);
    }

    return EXIT_SUCCESS;
}