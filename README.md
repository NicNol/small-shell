# small shell

A miniature version of a Unix shell written in C.

![Preview of small shell](/preview.png?raw=true)

## Background

A shell provides users with an interface to the Unix system. It gathers input and executes programs based on that input. When a program finishes executing, it displays that program's output.

A shell is an environment in which we can run our commands, programs, and shell scripts. There are different flavors of a shell, just as there are different flavors of operating systems. Each flavor of shell has its own set of recognized commands and functions. [1](https://www.tutorialspoint.com/unix/unix-what-is-shell.htm)

This program was developed as part of an assignment for Oregon State University's CS 344 Operating Systems course. The assignment requirements are available [here](/requirements.md).

## Features

This program has the following features:

1. Small shell creates a terminal or shell for the user to process typical Unix commands with.

2. Small shell ignores blank lines, excess whitespace, and lines beginning with the `#` symbol (comments).

3. Small shell expands `$$` strings within arguments passed to the shell and expands them into the shell's process ID (PID).

4. Small shell has custom-built `exit`, `cd`, and `status` commands.

5. Small shell can execute other programs (and Unix commands) by running small shell in the same path as the program to be run.

6. Small shell can redirect input and output using the "<" and ">" arguments respectively.

Example: The command `wc < file1 > file 2` will call the `wc` command using `file1` in place of standard input (stdin) and print the results to `file2` in place of standard output (stdout).

7. Small shell can run children processes in the foreground (default) or in the background by using "&" as the last argument. This command does not work with the `exit`, `cd`, or `status` commands.

8. Small shell has built-in handling for SIGINT (<kbd>CTRL</kbd>+<kbd>C</kbd>) and SIGTSTP (<kbd>CTRL</kbd>+<kbd>Z</kbd>) signals. SIGINT signals are ignored by small shell, but terminate children processes running in the foreground only. SIGTSTP prevents children processes from running in the background and can be toggled on and off.

## Development

System Requirements: Unix (Linux, Mac OS, or WSL2)

1. Clone the repository.

2. In your terminal, navigate to the project directory.

3. Compile by using the command `gcc --std=gnu99 -o smallsh main.c` .

4. Run the file by using the command `./smallsh` .

## Test

1. In your terminal, navigate to the project directory. Ensure the program has already been compiled per the above instructions.

2. Test using the command: `p3testscript 2>&1` .
