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
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT_LENGTH 2048
#define MAX_ARGUMENT_NUMBER 512

struct processNode {
    pid_t processID;
    struct processNode* next;
};

bool isBackgroundProcessAllowed(char* command) {
    static bool response = true;
    
    if (strcmp(command, "toggle") == 0) {
        response = !response;
        return response;
    }

    return response;
}

void handleSIGINT(int signalNumber){
    	
}

void handleSIGTSTP(int signalNumber) {

    static bool allowBackgroundProcesses = false;
    static int count = 0;

    static pid_t* parentPID = NULL;
    pid_t currentPID = getpid();
    if (parentPID == NULL) {
        parentPID = malloc(sizeof(currentPID));
        *parentPID = currentPID;
    }

    if (currentPID == *parentPID) {
        if (count == 0) {
            //Skip messages on initialization
            count++;
            isBackgroundProcessAllowed("toggle");
        }

        else if (allowBackgroundProcesses) {
            char* enterMessage = "Entering foreground-only mode (& is now ignored).\n";
            write(STDIN_FILENO, enterMessage, 51);
        }

        else {
            char* exitMessage = "Exiting foreground-only mode.\n";
            write(STDIN_FILENO, exitMessage, 31);
        }

        allowBackgroundProcesses = !allowBackgroundProcesses;
        isBackgroundProcessAllowed("toggle");
        
    }

    return;
}

void setupSignals() {
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};
    
	// Fill out the SIGINT_action struct
	// Register handleSIGINT as the signal handler
	SIGINT_action.sa_handler = handleSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);

    SIGTSTP_action.sa_handler = handleSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

}

void handleChildSignals(bool isBackgroundProcess) {
    /* All children ignore the SIGTSTP signal per the requirements */
    signal(SIGTSTP, SIG_IGN);

    /* Only background processes ignore the SIGINT signal per the requirements */
    if (isBackgroundProcess) {
        signal(SIGINT, SIG_IGN);
    }
    else {
        signal(SIGINT, SIG_DFL);
    }
}

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
    fflush(stdout);

    while((ch = getchar()) != '\n') {

        if (ch == EOF) {
            clearerr(stdin);
            printf(": ");
            fflush(stdout);
        }

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
    
    /* Parse words in input while token is not NULL */
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

void redirect(char* inputFile, char* outputFile) {

    /* Declare variables */
    int result;
    int sourceFD;
    int targetFD;

    if (inputFile) {
        // Open source file
        sourceFD = open(inputFile, O_RDONLY);
        if (sourceFD == -1) { 
            perror("Input file does not exist"); 
            exit(1); 
        }

        // Redirect stdin to source file
        result = dup2(sourceFD, 0);
        if (result == -1) { 
            perror("Could not redirect input"); 
            exit(1); 
        }
    }

    if (outputFile) {
        // Open target file
        targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (targetFD == -1) { 
            perror("Could not open or create output file"); 
            exit(1); 
        }
    
        // Redirect stdout to target file
        result = dup2(targetFD, 1);
        if (result == -1) { 
            perror("Could not redirect output"); 
            exit(1); 
        }
    }
}

void checkForRedirect(char** argumentsArray, char** inputFile, char** outputFile) {

    /* Declare and initialize variables */
    int index = 1;

    /* Check arguments array for redirection */
    while (argumentsArray[index] != NULL) {

        /* If '<' argument is found, copy the next argument to inputFile */
        if (strcmp(argumentsArray[index], "<") == 0) {

            /* Create space for inputFile string, then copy it from the argument array */
            *inputFile = malloc(sizeof(argumentsArray[index + 1]));
            strcpy(*inputFile, argumentsArray[index + 1]);

            /* Remove redirection from the argument array */
            argumentsArray[index] = '\0';
            argumentsArray[index + 1] = '\0';

            /* Advance the index pointer */
            index += 2;
            continue;
        }

        /* If '>' argument is found, copy the next argument to outputFile */
        if (strcmp(argumentsArray[index], ">") == 0) {

            /* Create space for ouputFile string, then copy it from the argument array */
            *outputFile = malloc(sizeof(argumentsArray[index + 1]));
            strcpy(*outputFile, argumentsArray[index + 1]);

            /* Remove redirection from the argument array */
            argumentsArray[index] = '\0';
            argumentsArray[index + 1] = '\0';

            /* Advance the index pointer */
            index += 2;
            continue;
        }

        index++;
    }

}

void handleRedirects(char** argumentsArray, bool backgroundProcess) {
    
    /* Create a pointer to nullFile to be used with background processes */
    char* nullFile = "/dev/null";

    /* Create a double pointer to keep track of input and output file paths */
    char** inputFile = malloc(sizeof(char*));
    char** outputFile = malloc(sizeof(char*));

    /* Set the each file path to NULL in case there is no redirection */
    *inputFile = NULL;
    *outputFile = NULL;

    /* Check for redirects */
    checkForRedirect(argumentsArray, inputFile, outputFile);

    /* If input or output file is not specified, mute shell by using a null file */
    if ((*inputFile == NULL) && backgroundProcess) {
        *inputFile = malloc(sizeof(nullFile));
        strcpy(*inputFile, nullFile);
    }
    if ((*outputFile == NULL) && backgroundProcess) {
        *outputFile = malloc(sizeof(nullFile));
        strcpy(*outputFile, nullFile);
    }

    /* Perform redirects */
    redirect(*inputFile, *outputFile);

    /* Free memory */
    free(*inputFile);
    free(inputFile);
    free(*outputFile);
    free(outputFile);
}

void changeDirectory(char* location, int wordCount) {

    /* If only the command was given, cd to the home directory */
    if (wordCount == 1) {
        chdir(getenv("HOME"));
        return;
    }

    /* Else change to the location directory */
    chdir(location);
    return;
}

void getStatus() {
    /* Get last child PID and its status from env */
    char* pidString = getenv("spawnPID"); // if no spawnPID, returns NULL
    char* pidStatusString = getenv("spawnPIDStatus");

    /* If no child process has been created, print status 0 */
    if (pidString == NULL) {
        printf("exit status 0\n");
        fflush(stdout);
        return;
    }

    /* Else print the last child PID and its status code */
    if ((strcmp(pidStatusString, "0") == 0) || (strcmp(pidStatusString, "1") == 0)) {
        printf("PID %s exited with value %s\n", pidString, pidStatusString);
        fflush(stdout);
        return;
    }
    else {
        printf("PID %s terminated with status %s\n", pidString, pidStatusString);
        fflush(stdout);
        return;
    }
    
}

void setSpawnPID(pid_t spawnPid, int childStatus) {

    /* Declare variables */
    char pidString[20];
    char pidStatusString[20];

    /* Convert spawnPID to string*/
    sprintf(pidString, "%d", spawnPid);

    /* Check exit status and convert to string  */ 
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

void stashSpawnPID(pid_t spawnPid, struct processNode** headNode) {

    /* Create a new node */
    struct processNode* newNode = malloc(sizeof(struct processNode));
    newNode->processID = spawnPid;
    if (headNode != NULL) newNode->next = *headNode;

    /* Move the head to our newNode */
    *headNode = newNode;

}

void checkSpawnPID(struct processNode** headNode) {

    /* Declare and initialize variables */
    int waitResult;
    int processStatus;
    struct processNode* currentNode = *headNode;
    struct processNode* previousNode = NULL;
    struct processNode* removedNode = NULL;
    char pidString[20];
    char pidStatusString[20];

    /* Traverse Linked List */
    while(currentNode != NULL) {

        /* Check if process has terminated */
        waitResult = waitpid(currentNode->processID, &processStatus, WNOHANG);

        /* If result is not zero, process has terminated */
        if (waitResult != 0) {

            /* Convert spawnPID to string*/
            sprintf(pidString, "%d", currentNode->processID);

            /* Check exit status and convert to string  */ 
            if(WIFEXITED(processStatus)) {
                /* Exited normally */
                sprintf(pidStatusString, "%d", WEXITSTATUS(processStatus));
            }
            else {
                /* Exited abnormally */
                sprintf(pidStatusString, "%d", WTERMSIG(processStatus));
            }

            /* Print message that process has terminated */
            printf("Background PID %s exited with status %s\n", pidString, pidStatusString);
            fflush(stdout);

            /* Remove this processNode from the linked list */
            if (currentNode == *headNode) {
                *headNode = currentNode->next;
            }
            else {
                previousNode->next = currentNode->next;
            }
            removedNode = currentNode;
            currentNode = currentNode->next;
            free(removedNode);
        }
        /* Else process is still running. Check next process in linked list */
        else {
            previousNode = currentNode;
            currentNode = currentNode->next;
        }
    }
}

void killSpawnPID(struct processNode** headNode) {

    /* Declare and initialize variables */
    struct processNode* currentNode = *headNode;
    struct processNode* removedNode = NULL;

    /* Traverse Linked List */
    while(currentNode != NULL) {

        /* Kill each the processID of each node and free the memory */
        kill(currentNode->processID, SIGKILL);
        removedNode = currentNode;
        currentNode = currentNode->next;
        free(removedNode);
    }
}

void handleSpawnPID(char* action, pid_t spawnPid) {

    /* On first call of this function, create a static headNode pointer */
    static struct processNode** headNode = NULL;
    if (headNode == NULL) {
        headNode = malloc(sizeof(malloc(sizeof(struct processNode))));
    }

    /* Action #1: Stash background spawnPid so that it can be checked later */
    if (strcmp(action, "stash") == 0) {
        stashSpawnPID(spawnPid, headNode);
    }

    /* Action #2: Check all background processes to see if any have ended */
    if ((headNode != NULL) && (strcmp(action, "check") == 0)) {
        checkSpawnPID(headNode);
    }

    /* Action #3: Kill all background processes */
    if ((headNode != NULL) && (strcmp(action, "kill") == 0)) {
        killSpawnPID(headNode);
    }
}

/*
  Create a child process using fork()
*/
int createNewProcess(char** argumentsArray, int wordCount){

    /* Declare and initialize variables */
	int childStatus;
    pid_t result;
	pid_t spawnPid = fork();
    bool backgroundProcess = (strcmp(argumentsArray[wordCount - 1], "&") == 0);
    bool backgroundProcessAllowed = isBackgroundProcessAllowed("request");

    /* Remove '&' from list of arguments */
    if (backgroundProcess) {
        argumentsArray[wordCount - 1] = NULL;

        /* Disallow the background process if the global flag has been set */
        if (!backgroundProcessAllowed) {
            backgroundProcess = false;
        }
    }
    
    /* Create a switch case to catch errors, child process, and parent process */
	switch(spawnPid){
        
        /* Error Case */
        case -1:
            perror("fork()\n");
            exit(1);
            break;
        
        /* Child Case */
        case 0:
            handleChildSignals(backgroundProcess);
            handleRedirects(argumentsArray, backgroundProcess);
            execvp(argumentsArray[0], argumentsArray);
            perror("Could not find command in path\n");
            exit(1);
            break;
        
        /* Parent Case */
        default:

            /* If process should run in the background, add spawnPid to list of background processes */
            if (backgroundProcess) {
                handleSpawnPID("stash", spawnPid);
                printf("PID %d is running in the background\n", spawnPid);
                fflush(stdout);
            }
            /* If process is not a background process, set env variables
            *   with PID and exit status */
            else {
                /* Wait for child process to terminate */
                result = waitpid(spawnPid, &childStatus, 0);

                /* Set env variables with last PID and status */
                setSpawnPID(spawnPid, childStatus);
                
                /* If an error was found, print the status */
                if (WIFSIGNALED(childStatus)) {
                    getStatus();
                }
                
            }
	} 
}

void executeInput(char** argumentsArray, int wordCount) {
    char* command = argumentsArray[0];

    if (strcmp(command, "cd") == 0) {
        return changeDirectory(argumentsArray[1], wordCount);
    }

    if (strcmp(command, "status") == 0) {
        return getStatus();
    }

    else {
        createNewProcess(argumentsArray, wordCount);
        return;
    }
}

void cleanup() {
    unsetenv("spawnPID");
    unsetenv("spawnPIDStatus");
    handleSpawnPID("kill", 0);
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
    
    /* Set up signal handling */
    setupSignals();
    /* Initialize handler with Parent PID */
    handleSIGINT(0);
    handleSIGTSTP(0);

    getInput(input);

    while(strcmp(input, "exit") != 0) {
        
        wordCount = parseInput(input, argumentsArray);
        if (wordCount > 0) executeInput(argumentsArray, wordCount);
        handleSpawnPID("check", 0);
        getInput(input);
    }

    cleanup();
    return EXIT_SUCCESS;
}