# Simple Shell

Simple Shell is a simple Unix shell implemented in C, featuring basic command execution, redirection, piping, and built-in commands like cd, help, and pat.

## Features
- **Command Execution:** Execute commands like in a standard shell environment.
- **Redirection:** Support for `<`, `>`, and `>>` redirection operators.
- **Piping:** Ability to pipe output from one command to another.
- **Built-in Commands:**
  - `cd`: Change current directory.
  - `help`: Display a help menu describing shell usage.
  - `pat`: Print the contents of a file (similar to `cat`).
- **Signal Handling**: Handles `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z) signals.
- **Save Command History**: Save history of commands automatically to `.my_shell_history `.

## Installation
1. Clone the Repository 
2. `$ gcc -o shell shell.c -lreadline`
3. `$ ./shell`

## Supported Commands
- `exit`: Exit the shell.
- `cd [directory]`: Change current directory.
- `help`: Display shell usage and available commands.
- `pat [file]`: Print the contents of a file.

## Redirection
- `< [file]`: Redirect input from a file.
- `> [file]`: Redirect output to a file (overwrite).
- `>> [file]`: Redirect output to a file (append).

## Pipes
- `|`: Pipe the output of one command to the input of another.

## Contributing
Contributions are welcome! Feel free to fork the repository and submit pull requests.

## License
This project is licensed under the Unlicense - see the LICENSE file for details.