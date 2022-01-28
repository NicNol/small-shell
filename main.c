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
    

    while(strcmp(input, "exit") != 0) {

        getInput(input);
        printf("%s", input);
        printf("\n");
    }

    return EXIT_SUCCESS;
}