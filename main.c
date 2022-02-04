/*
*   Author: Nic Nolan
*   Last Modified: 02/02/2022
*   OSU Email Address: nolann@oregonstate.edu
*   Course Number: CS344
*   Assignment Number: #3
*   Due Date: 02/07/2022
*   Description: This program does the following:
*       1) Creates a terminal or shell for the user to process commands with.
*       2) The shell ignores blank lines (whitespace) and lines beginning with
*           the '#' symbol (comments).
*       3) Expands "$$" within arguments passed to the shell and converts them
*            into the shell's process ID (PID).
*       4) The shell has built in "exit", "cd", and "status" commands.
*       5) The shell can execute other commands by running the shell in the 
*           same path as a the program to be run.
*       6) The shell can redirect input and output using the "<" and ">"
*           arguments respectively.
*       7) The shell can run children processes in the foreground (default) or
*           in the background by using "&" as the last argument.
*       8) The shell manages SIGINT (ctrl-C) and SIGTSTP (ctrl-Z) signals.
*           SIGINT signals are ignore by the shell, but terminate children
*           processes running in the foreground only. SIGTSTP prevents children
*           processes from running in the background and can be toggled.
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

/* Define constants for length of user input and number of arguments */
#define MAX_INPUT_LENGTH 2048
#define MAX_ARGUMENT_NUMBER 512

/* Create a linked list struct for storing background process IDs */
struct processNode {
    pid_t processID;
    struct processNode* next;
};

/* Create a linked list struct for storing SIGTSTP messages */
struct messageNode {
    char* message;
    int messageLength;
    struct messageNode* next;
};

/*
*   Creates a global function that returns whether background processes are allowed. The initialized response is true.
*   
*   Input: char* command -- use "toggle" to flip the boolean value of the response. Use NULL to get the response value.
*   Output: a boolean value corresponding to whether or not background processes are currently allowed.
*/
bool isBackgroundProcessAllowed(char* command) {
    
    /* Intialize a static variable so that its value is preserved throughout multiple function calls */
    static bool response = true;

    /* Catch NULL values so errors aren't thrown on the strcmp() function */
    if (command == NULL) return response;
    
    /* Check if user passed "toggle" argument and flip the response value if so */
    if (strcmp(command, "toggle") == 0) {
        response = !response;
    }

    /* Return the response value */
    return response;
}

/*
*   Stash SIGTSTP messages in a linked list so they can be printed when a foreground process is no longer running.
*   
*   Input: 
*       char* message -- The SIGTSTP message to be stored and printed later.
*       int messageLength -- the length of the message. We must use write() in signal handlers because printf is not re-entrant.
*       struct messageNode** headNode -- a double pointer to the head node of the linked list.
*   
*   Output: N/A. Messages are stored in linked list beginning at headNode pointer.
*/
void stashMessage(char* message, int messageLength, struct messageNode** headNode) {

    /* Create a new messageNode that will later be stored in the linked list */
    struct messageNode* newNode = malloc(sizeof(struct messageNode));
    newNode->message = malloc(strlen(message) + 1);
    strcpy(newNode->message, message);
    newNode->messageLength = messageLength;
    newNode->next = NULL;

    /* Traverse to the end of the linked list */
    struct messageNode* currentNode = *headNode;
    while (currentNode != NULL && currentNode->next != NULL) {
        currentNode = currentNode->next;
    }

    /* If there are no nodes in the linked list, make currentNode the headNode */
    if (currentNode == NULL) {
        *headNode = newNode;
    }
    /* Else, add new messageNode to the end of the linked list */
    else {
        currentNode->next = newNode;
    }
}

/*
*   Print SIGTSTP messages from the linked list and free the messageNodes from memory.
*   
*   Input: struct messageNode** headNode -- a double pointer to the head node of the linked list.
*   
*   Output: N/A. Messages are printed to the terminal and the linked list is removed from memory.
*/
void printMessages(struct messageNode** headNode) {

    /* Declare variables for freeing our messageNodes from memory */
    struct messageNode* garbageNode;
    char* garbageMessage;

    /* Print new line before printing results for legibility */
    if (*headNode != NULL) {
        write(STDOUT_FILENO, "\n", 2);
    }
    
    /* Traverse linked list of messageNodes */
    while (*headNode != NULL) {

        /* Print message at headNode */
        if(strlen((*headNode)->message) > 0) {
            write(STDOUT_FILENO, (*headNode)->message, (*headNode)->messageLength);
        }

        /* Assign message at headNode as garbage */
        garbageNode = *headNode;
        garbageMessage = (*headNode)->message;

        /* Move head pointer to next node */
        *headNode = (*headNode)->next;

        /* Free garbage from memory */
        if (garbageMessage != NULL) free(garbageMessage);
        if (garbageNode != NULL) free(garbageNode);
        
    }
}

/*
*   Handle SIGTSTP messages by either stashing, printing, or counting the currently stored messages.
*   
*   Input: 
*       char* action -- "stash", "print", or "count" are the actions that can be handled.
*       char* message -- The SIGTSTP message to be stored and printed later. Use NULL if action is not "stash".
*       int messageLength -- the length of the message. We must use write() in signal handlers because printf is not re-entrant.
*   
*   Output: Returns the count of messages if a valid action is given. Returns -1 on error or invalid command.
*/
int handleMessages(char* action, char* message, int messageLength) {

    /* Declare and initialize static variables that will persist through multiple calls of the handler */
    static struct messageNode** headNode = NULL;
    static int count = 0;

    /* Create an address for the headNode of the message stash to be stored on the first call */
    if (headNode == NULL) {
        headNode = malloc(sizeof(struct messageNode*));
        *headNode = NULL;
    }

    /* Catch NULL action values as they will throw errors on strcmp */
    if (action == NULL) return -1;

    /* If command is to stash the message, call the stash method and increment the count of messages */
    if (strcmp(action, "stash") == 0) {
        stashMessage(message, messageLength, headNode);
        count++;
        return count;
    }

    /* If command is to print the messages, ensure that a message exists and then call the print method */
    if ((*headNode != NULL) && (strcmp(action, "print") == 0)) {
        printMessages(headNode);
        count = 0;
        return count;
    }

    /* If command is to count the messages, return the current count */
    if (strcmp(action, "count") == 0) {
        return count;
    }

    /* If in invalid action is passed, return -1 to indicate an error */
    return -1;
}

/*
*   Create a global function that keep track of whether a foreground process is running to assist with SIGTSTP signal handling.
*   
*   Input: 
*       char* action -- "set" sets that response to true; "clear" sets the response to false; use NULL to only get the response.
*   
*   Output: A boolean value based on whether a foreground process is currently running.
*/
bool isProcessRunning(char* action) {
    static bool output = false;

    if (action == NULL) return output;

    if (strcmp(action, "set") == 0) {
        output = true;
    }

    if (strcmp(action, "clear") == 0) {
        output = false;
    }

    return output;
}

/* Create an empty function to complete the SIGINT sigaction struct */
void handleSIGINT(int signalNumber){
    /* No actions in this handler */
}

/*
*   Create a handler function that keep track of whether users can use background processes when the SIGTSTP signal (CTRL-Z) is called.
*   
*   Input: 
*       int signalNumber -- required by sigaction but not used in this function.
*   
*   Output: N/A. Prevents users from running background processes when the SIGTSTP signal is called. Allows background processes again
*           when the SIGTSTP signal is called again.
*/
void handleSIGTSTP(int signalNumber) {

    /* Declare and initialize variables */
    int messageLength;
    int maxLength = 60;
    char message[maxLength];
    static bool allowBackgroundProcesses = true;

    /* Create a message to be sent to the user based on if background processes were allowed before the signal call */
    if (allowBackgroundProcesses) {
        strcpy(message, "Entering foreground-only mode (& is now ignored).\n");
        messageLength = 51;
    }
    else {
        strcpy(message, "Exiting foreground-only mode.\n");
        messageLength = 31;
    }

    /* Handle the message by either stashing it or printing it based on if a process is currently running in the foreground */
    if (isProcessRunning(NULL)) {
        handleMessages("stash", message, messageLength);
    }
    else {
        write(STDOUT_FILENO, message, messageLength);
        write(STDOUT_FILENO, ": ", 3);
    }
    
    /* Toggle background processes (True -> False or False -> True) */
    allowBackgroundProcesses = !allowBackgroundProcesses;
    isBackgroundProcessAllowed("toggle");
}

/*
*   Setup Program signals that will remain throughout the rest of the program.
*   
*   Input: N/A.
*   
*   Output: N/A. Environment prevents SIGINT (CTRL-C) from killing the shell, but will kill foreground children processes.
*/
void setupSignals() {

    /* Create sigaction structs for SIGINT and SIGTSTP signals */
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};
    
	/* Create a SIGINT_action struct for the SIGINT signal */
	SIGINT_action.sa_handler = handleSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);

    /* Create a SIGINT_action struct for the SIGTSTP signal */
    SIGTSTP_action.sa_handler = handleSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    /* Initialize SIGTSTP message handler here to prevent errors on first signal call */
    handleMessages("count", "\0", 0);
}

/*
*   Sets up signal handling in children processes that are forked off of the parent process.
*   
*   Input: bool isBackgroundProcess -- a boolean value representing whether the child process is a background process
*   
*   Output: N/A. Signals are changed in the child process according to whether child is run in the foreground or background.
*/
void handleChildSignals(bool isBackgroundProcess) {

    /* All children ignore the SIGTSTP signal per the requirements */
    signal(SIGTSTP, SIG_IGN);

    /* Only background processes ignore the SIGINT signal per the requirements */
    if (isBackgroundProcess) {
        signal(SIGINT, SIG_IGN);
    }
    /* Ensure that foreground processes get the default SIGINT signal handling in the child process */
    else {
        signal(SIGINT, SIG_DFL);
    }
}

/*
*   Check the passed argument for variables that need to be expanded from "$$" to the process ID. If "$$" is included in the argument,
*   it is expanded to the PID. This function returns how long the argument will be after all "$$" variables are expanded.
*   
*   Input: char* argument -- a pointer to the argument to be checked.
*   
*   Output: An integer representing the value of the argument after all "$$" variables have been expanded.
*/
int getArgumentLength(char* argument) {

    /* Declare and Initialize variables */
    pid_t pid = getpid();
    char pidString[20];
    sprintf(pidString, "%d", pid);
    fflush(stdout);
    int pidLength = strlen(pidString);
    int variableCount = 0;
    int i;

    /* Count instances of "$$" that must be expanded */
    for(i = 0; i < strlen(argument) - 1; i++) {
        if(argument[i] == '$' && argument[i + 1] == '$') {
            variableCount++;
            i++;
        }
    }

    /* Calculate and return the argument length */
    return strlen(argument) - (2 * variableCount) + (pidLength * variableCount);
}

/*
*   Check the passed argument for variables that need to be expanded from "$$" to the process ID. If "$$" is included in the argument,
*   it is expanded to the PID. This function copies the original string stored in 'token' and saves the expanded version into 'argument'.
*   
*   Input:
*           char* token -- a pointer to the token that is parsing the user input. This is essentially the "word" delimited by all whitespace.
*           char* argument -- a pointer to where the expanded token string should be stored after expansion.
*   
*   Output: An integer representing the value of the argument after all "$$" variables have been expanded.
*/
void expandVariable(char* token, char* argument) {

    /* Set up and declare variables */
    int t_i = 0; // token index
    int a_i = 0; // argument index

    pid_t pid = getpid();
    char pidString[20];
    sprintf(pidString, "%d", pid); // Convert PID from type pid_t to string
    fflush(stdout);
    int pidLength = strlen(pidString);

    /* Parse token for "$$" variable */
    for (t_i = 0; t_i < strlen(token); t_i++) {

        /* Concatenate pid to argument if "$$" is found */
        if((token[t_i] == '$') && (t_i + 1 < strlen(token)) && (token[t_i+1] == '$')) {
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

    /* Terminate argument with null character */
    argument[a_i] = '\0';
}

/*
*   Solicit input from the user and store it in the passed input address.
*   
*   Input: char* input -- a char pointer where the input string from the user will be stored.
*   Output: N/A.
*
*   Input method adopted from: https://stackoverflow.com/questions/30939517/loop-through-user-input-with-getchar/30939585
*   Accessed on 1/27/2022
*/
void getInput(char* input) {
    
    /* Declare and initialize variables */
    int ch;
    int offset = 0;

    /* Prompt user for input */
    printf(": ");
    fflush(stdout);

    /* Get input one character at a time */
    while(((ch = getchar()) != '\n') && offset < MAX_INPUT_LENGTH - 1) {

        /* Flush stdout in case a message is printed while waiting to get input */
        fflush(stdout);

        /* Set each character at the input address one byte at a time */
        input[offset] = ch;
        offset++;
    }

    /* Put a terminating character on the input*/
    input[offset] = '\0';
}

/*
*   Take the user input, remove whitespace, and store each word in an argumentsArray.
*   
*   Input:
*       char* input -- a char pointer where the input string from the user is stored.
*       char** argumentsArray -- a double char pointer to where the arguments array is to be stored.
*
*   Output: returns an integer representing the number of arguments (words) after parsing.
*/
int parseInput(char* input, char** argumentsArray) {

    /* Set up string token and while loop variables */
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

        /* Get the length of token after we account for expanded "$$" variables */
        argumentLength = getArgumentLength(token);

        /* Use malloc to create a block of memory for our expanded argument */
        argumentsArray[argNum] = (char*) malloc(argumentLength + 1);

        /* Expand any "$$" variables in token, and then store the expanded string in our argumentsArray */
        expandVariable(token, argumentsArray[argNum]);

        printf("\nargumentLength = %d", argumentLength);
        printf("\nargumentsArray[%d] = %s\n", argNum, argumentsArray[argNum]);
        sleep(1);

        /* Set up for next loop */
        token = strtok_r(NULL, whitespace, &saveptr);
        argNum++;
    }
   
    /* Terminate argumentsArray with a NULL pointer so we know when the list ends */
    argumentsArray[argNum] = NULL;

    return argNum;
}

/*
*   Redirect user input and output by passing an inputFile and an outputFile to this function.
*   
*   Input:
*       char* inputFile -- a char pointer of the input file or source path. Use NULL for stdin.
*       char* outputFile -- a char pointer of the outpath file or source path. Use NULL for stdout.
*
*   Output: N/A. Redirects input and/or output according to arguments passed.
*/
void redirect(char* inputFile, char* outputFile) {

    /* Declare variables */
    int result;
    int sourceFD;
    int targetFD;

    /* If the user passes a non-NULL inputFile, redirect input */
    if (inputFile) {

        /* Open the input file */
        sourceFD = open(inputFile, O_RDONLY);

        /* If file does not exist, print an error message */
        if (sourceFD == -1) { 
            perror("Input file does not exist"); 
            exit(1); 
        }

        /* Redirect stdin to input file using dup2 */
        result = dup2(sourceFD, 0);

        /* If an error is raised, print an error message */
        if (result == -1) { 
            perror("Could not redirect input"); 
            exit(1); 
        }
    }

    /* If the user passes a non-NULL outputFile, redirect output */
    if (outputFile) {

        /* Open the output file */
        targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        /* If file could not be opened or created, print an error message */
        if (targetFD == -1) { 
            perror("Could not open or create output file"); 
            exit(1); 
        }
    
        /* Redirect stdout to output file using dup2 */
        result = dup2(targetFD, 1);

        /* If an error is raised, print an error message */
        if (result == -1) { 
            perror("Could not redirect output"); 
            exit(1); 
        }
    }
}

/*
*   Check the argumentsArray for the redirect words "<" (input) and ">" (output).
*   If found, store the input or output path in the argument location.
*   
*   Input:
*       char** argumentsArray -- the list of arguments, already parsed as input.
*       char** inputFile -- a double char pointer to the location where the inputFile path should be stored.
*       char** outputFile -- a double char pointer to the location where the outputFile path should be stored.
*
*   Output: N/A. Redirects input and/or output according to arguments passed.
*/
void checkForRedirect(char** argumentsArray, char** inputFile, char** outputFile) {

    /* Declare and initialize variables */
    int index = 0;

    /* Check each index in the argumentsArray for redirection symbols */
    while (argumentsArray[index] != NULL) {

        /* If "<" argument is found, copy the next argument to inputFile */
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

        /* If ">" argument is found, copy the next argument to outputFile */
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

        /* If symbol is not found, check next index of argumentsArray */
        index++;
    }

}

/*
*   Handles any redirects in the user input argumentsArray by changing stdin or stdout to
*   the argument path location, or to a "/dev/null" file if the process runs in the background.
*   
*   Input:
*       char** argumentsArray -- the list of arguments, already parsed as input.
*       bool backgroundProcess -- a boolean value about whether the argument is to be
*                                   ran as a backgroundProcess.
*
*   Output: N/A. Redirects input and/or output according to arguments passed.
*/
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

    /* If input or output file is not specified AND process is run in the background, 
    *   mute shell by using a null file */
    if ((*inputFile == NULL) && backgroundProcess) {
        *inputFile = malloc(sizeof(nullFile));
        strcpy(*inputFile, nullFile);
    }
    if ((*outputFile == NULL) && backgroundProcess) {
        *outputFile = malloc(sizeof(nullFile));
        strcpy(*outputFile, nullFile);
    }

    /* Perform redirects according to user input */
    redirect(*inputFile, *outputFile);

    /* Free memory */
    free(*inputFile);
    free(inputFile);
    free(*outputFile);
    free(outputFile);
}

/*
*   This function changes the current working directory using either a relative or absolute file path.
*   
*   Input:
*       char* location -- the file path to change directories to. Can be absolute or relative.
*       int wordCount -- the number of arguments (words) passed by the user as input.
*
*   Output: N/A. Changes the current working directory to the location passed as an argument.
*/
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

/*
*   This function prints the process ID (PID) and status of the last command ran.
*   Excludes built in functions ("cd", "status", and "exit")
*   
*   Input: N/A.
*
*   Output: N/A. Prints the PID and status of the last command ran.
*/
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

    /* If the child status is 0 or 1, use the noun "value" prior to the status integer */
    if ((strcmp(pidStatusString, "0") == 0) || (strcmp(pidStatusString, "1") == 0)) {
        printf("PID %s exited with value %s\n", pidString, pidStatusString);
        fflush(stdout);
        return;
    }

    /* Else, use the noun "signal" prior to the status integer */
    else {
        printf("PID %s terminated with signal %s\n", pidString, pidStatusString);
        fflush(stdout);
        return;
    }
}

/*
*   This function sets an environmental variable that keeps track of the last child's
*   process ID (PID) and the status code when it exited or terminated.
*   
*   Input:
*           pid_t spawnPid -- This is the process ID of the child that has exited or been terminated.
*           int childStatus -- This is the int status value modified by waitpid.
*
*   Output: N/A. Sets two environment variables "spawnPID" and "spawnPIDStatus" with corresponding values.
*/
void setSpawnPID(pid_t spawnPid, int childStatus) {

    /* Declare variables for storing the child PID and status */
    char pidString[20];
    char pidStatusString[20];

    /* Convert spawnPID to string*/
    sprintf(pidString, "%d", spawnPid);
    fflush(stdout);

    /* Check child process status and convert to string  */ 
    if(WIFEXITED(childStatus)) {
        /* Exited normally */
        sprintf(pidStatusString, "%d", WEXITSTATUS(childStatus));
        fflush(stdout);
    }
    else {
        /* Exited abnormally */
        sprintf(pidStatusString, "%d", WTERMSIG(childStatus));
        fflush(stdout);
    }

    /* Set spawnPID and its status to environment variables */
    setenv("spawnPID", pidString, 1);
    setenv("spawnPIDStatus", pidStatusString, 1);
}

/*
*   This function stashes a background process's PID into a linked list so that it can be
*   checked at a later time for exit or termination.
*   
*   Input:
*           pid_t spawnPid -- This is the process ID of the child that will be stored.
*           struct processNode** headNode -- This is a double pointer to the headNode of the
*                                              linked list.
*
*   Output: N/A. Stores the child PID in the linked list.
*/
void stashSpawnPID(pid_t spawnPid, struct processNode** headNode) {

    /* Create a new processNode */
    struct processNode* newNode = malloc(sizeof(struct processNode));
    newNode->processID = spawnPid;

    /* If another node exists in the list at the head location, set the newNode's next pointer to it */
    if (headNode != NULL) newNode->next = *headNode;

    /* Move the address of the headNode to our newNode */
    *headNode = newNode;

}

/*
*   This function checks all child PIDs in the linked list. If a PID is found to have exited or been
*   terminated, this function prints the PID and its status. It also frees the processNodes from memory
*   after the child process has exited or been terminated.
*   
*   Input: struct processNode** headNode -- This is a double pointer to the headNode of the linked list.
*
*   Output: N/A. Prints messages to the terminal about processes that have finished.
*/
void checkSpawnPID(struct processNode** headNode) {

    /* Declare and initialize variables */
    int waitResult;
    int processStatus;
    struct processNode* currentNode = *headNode;
    struct processNode* previousNode = NULL;
    struct processNode* removedNode = NULL;
    char pidString[20];
    char pidStatusString[20];
    char exitNoun[20];
    char exitVerb[20];

    /* Traverse Linked List */
    while(currentNode != NULL) {

        /* Check if process has terminated */
        waitResult = waitpid(currentNode->processID, &processStatus, WNOHANG);

        /* If result is not zero, process has terminated */
        if (waitResult != 0) {

            /* Convert spawnPID to string*/
            sprintf(pidString, "%d", currentNode->processID);
            fflush(stdout);

            /* Check exit status and convert to string  */ 
            if(WIFEXITED(processStatus)) {
                /* Exited normally */
                sprintf(pidStatusString, "%d", WEXITSTATUS(processStatus));
                fflush(stdout);
                strcpy(exitVerb, "exited");
                strcpy(exitNoun, "value");
            }
            else {
                /* Exited abnormally */
                sprintf(pidStatusString, "%d", WTERMSIG(processStatus));
                fflush(stdout);
                strcpy(exitVerb, "terminated");
                strcpy(exitNoun, "signal");
            }

            /* Print message that process has exited or been terminated */
            printf("Background PID %s %s with %s %s\n", pidString, exitVerb, exitNoun, pidStatusString);
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

/*
*   This function kills all child PIDs in the linked list. It also frees the processNodes from memory
*   after the child process has exited or been killed.
*   
*   Input: struct processNode** headNode -- This is a double pointer to the headNode of the linked list.
*
*   Output: N/A. Background processes are killed.
*/
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

/*
*   This function handles all child process ID (PID) actions related to background processes.
*   Available actions include "stash", "check", and "kill".
*   
*   Input: 
*           char* action -- the desired action: "stash", "check", or "kill"
*           pid_t spawnPid -- the child PID if stashing. Else, use 0 (zero).
*
*   Output: N/A. The desired action is taken.
*/
void handleSpawnPID(char* action, pid_t spawnPid) {

    /* On first call of this function, create a static headNode pointer and initialize the location to NULL */
    static struct processNode** headNode = NULL;
    if (headNode == NULL) {
        headNode = malloc(sizeof(struct processNode*));
        *headNode = NULL;
    }

    /* Catch NULL actions to prevent errors with strcmp */
    if (action == NULL) return;

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
*   This function creates a fork and handles both the child and parent process.
*   The argumentsArray determines which actions are performed by the child and
*   parent processes.
*   
*   Input: 
*           char** argumentsArray -- the list of arguments, already parsed as input.
*           int wordCount -- the number of arguments (words) passed by the user as input.
*
*   Output: Int value of the exit status of each process.
*/
int createNewProcess(char** argumentsArray, int wordCount){

    /* Declare and initialize variables */
	int childStatus;
    pid_t result;
	pid_t spawnPid = fork(); // Create child process
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

            /* Handle signals, redirects, and execute process */
            handleChildSignals(backgroundProcess);
            handleRedirects(argumentsArray, backgroundProcess);
            execvp(argumentsArray[0], argumentsArray);

            /* Handle Errors */
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

            /* If process is not a background process, set env variables with PID and exit status */
            else {

                /* Wait for child process to terminate */
                isProcessRunning("set");
                result = waitpid(spawnPid, &childStatus, 0);
                isProcessRunning("clear");

                /* Set env variables with last PID and status */
                setSpawnPID(spawnPid, childStatus);
                
                /* If an error was found, print the status */
                if (WIFSIGNALED(childStatus)) {
                    getStatus();
                }
            }
	}

    return 0;
}

/*
*   This function handles all user input after parsing. If a built in command is called,
*   it is ran. Otherwise an exec() function is used to run all other commands.
*   
*   Input: 
*           char** argumentsArray -- the list of arguments, already parsed as input.
*           int wordCount -- the number of arguments (words) passed by the user as input.
*
*   Output: N/A. 
*/
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

/*
*   This function handles all clean up actions when our shell is ready to exit.
*   
*   Input: N/A.
*
*   Output: N/A. Environment variables created are deleted. Background processes are killed.
*/
void cleanup() {
    unsetenv("spawnPID");
    unsetenv("spawnPIDStatus");
    handleSpawnPID("kill", 0);
}

/*
*   Driver of the program (main).
*
*   Handles the system variable set up, signal handling, and begins repeatedly taking user
*   input until the "exit" command is given.
*
*   Compile the program as follows:
*       gcc --std=gnu99 -o smallsh main.c
*
*   Run the file by calling:
*       ./smallsh
*/
int main(void) {

    /* Declare Variables */
    char input[MAX_INPUT_LENGTH];
    char* argumentsArray[MAX_ARGUMENT_NUMBER];
    int wordCount;
    
    /* Set up signal handling */
    setupSignals();

    /* Begin getting user input */
    getInput(input);

    /* Continuing getting user input until "exit" is passed */
    while(strcmp(input, "exit") != 0) {
        
        /* Count the number of "words" in the users argument */
        wordCount = parseInput(input, argumentsArray);

        /* If input is not whitespace or comment, execute the input */
        if (wordCount > 0) executeInput(argumentsArray, wordCount);

        /* Print signal SIGTSTP messages before soliciting input again */
        handleMessages("print", NULL, 0);

        /* Print messages about background processes ending before soliciting input again */
        handleSpawnPID("check", 0);

        /* Get user inpuut again */
        getInput(input);
    }

    cleanup();
    return EXIT_SUCCESS;
}