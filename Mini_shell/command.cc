/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#define _DEBUG_LOG	TRUE
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "command.h"
#include <glob.h>

void handleCtrlC(int signo)
{
    printf("\nCtrl-C is disabled. Use 'exit' to exit the shell.\n");
    Command::_currentCommand.prompt();
}

SimpleCommand::SimpleCommand()
{
    // Create space for 5 arguments
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    // Check for wildcard characters in the argument
    glob_t globResult;
    glob(argument, GLOB_NOCHECK | GLOB_TILDE, NULL, &globResult);

    // Calculate the new number of arguments after expansion
    size_t newNumberOfArguments = _numberOfArguments + globResult.gl_pathc;

    // Allocate memory for the new arguments
    char **newArguments = (char **)realloc(_arguments, (newNumberOfArguments + 1) * sizeof(char *));

    if (newArguments == NULL)
    {
        perror("Error: realloc failed");
        exit(EXIT_FAILURE);
    }

    // Copy the expanded arguments
    for (size_t i = 0; i < globResult.gl_pathc; i++)
    {
        newArguments[_numberOfArguments + i] = strdup(globResult.gl_pathv[i]);

        if (newArguments[_numberOfArguments + i] == NULL)
        {
            perror("Error: strdup failed");
            exit(EXIT_FAILURE);
        }
    }

    // If no wildcard, add the original argument
    if (globResult.gl_pathc == 0)
    {
        newArguments[_numberOfArguments] = strdup(argument);

        if (newArguments[_numberOfArguments] == NULL)
        {
            perror("Error: strdup failed");
            exit(EXIT_FAILURE);
        }

        newNumberOfArguments++;
    }

    // Set the last element to NULL to indicate the end of arguments
    newArguments[newNumberOfArguments] = NULL;

    // Update the arguments and the number of arguments
    _arguments = newArguments;
    _numberOfArguments = newNumberOfArguments;

    // Release glob memory
    globfree(&globResult);
}


Command::Command()
{
    // Create space for one simple command
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = nullptr;
    _inputFile = nullptr;
    _errFile = nullptr;
    _background = 0;
    _append = 0;
    _freeonce = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    // Check if there's enough space for new simple commands
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands, _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (!_freeonce)
    {
        if (_outFile)
        {
            free(_outFile);
        }

        if (_errFile)
        {
            free(_errFile);
        }
    }
    else
    {
        if (_outFile)
        {
            free(_outFile);
        }
    }

    _numberOfSimpleCommands = 0;
    _outFile = nullptr;
    _inputFile = nullptr;
    _errFile = nullptr;
    _background = 0;
    _append = 0;
    _freeonce = 0;
}

void Command::print()
{
    char array[100];
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  ----------------------------------------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Background        CWD\n");
    printf("  ------------ ------------ ------------ ------------ --------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
           _background ? "YES" : "NO", getcwd(array, 100));
    printf("\n\n");
}

void Command::execute()
{
    int pid;
    int inpfd;
    int outfd;
    int errfd;
    char array[100];

    // Don't do anything if there are no simple commands
    if (_numberOfSimpleCommands == 0)
    {
        prompt();
        return;
    }

    // Print contents of Command data structure
    print();
   // check cd command 
    if (!strcmp(_simpleCommands[0]->_arguments[0], "cd"))
    {
        printf("Current Working Directory: %s\n", getcwd(array, 100));
        if (_simpleCommands[0]->_numberOfArguments > 1)
        {
            chdir(_simpleCommands[0]->_arguments[1]);
        }
        else
        {
            chdir("/home");
        }
        printf("New Working Directory: %s\n", getcwd(array, 100));
    }
    else
    {
        if (_numberOfSimpleCommands >= 1)
        {
            int defaultin = dup(0); // Default file Descriptor for stdin
            int defaultout = dup(1); // Default file Descriptor for stdout
            int defaulterr = dup(2); // Default file Descriptor for stderr

            // Create new pipe
            int fdpipe[2];
            if (pipe(fdpipe) == -1)
            {
                perror("cat_grep: pipe");
                    clear();
                    prompt();
                
               // exit(2);
            }

            if (_inputFile)
            {
                inpfd = open(_inputFile, O_RDONLY, 0666);
                if (inpfd < 0)
                {
                    perror("err : inputfile");
                    clear();
                    prompt();
                    return;
              
                  //  exit(2);
                }
            }
            else
            {
                inpfd = defaultin;
            }

            if (_outFile)
            {
                // Create file descriptor
                if (_append)
                {
                    outfd = open(_outFile, O_APPEND | O_CREAT | O_WRONLY, 0666);
                }
                else
                {
                    outfd = open(_outFile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                }

                if (outfd < 0)
                {
                    perror("err : create outfile");
                    clear();
                    prompt();
                   // exit(2);
                }
            }
            else
            {
                outfd = defaultout;
            }

            if (_errFile)
            {
                // Create file descriptor
                if (_append)
                {
                    errfd = open(_errFile, O_APPEND | O_CREAT | O_WRONLY, 0666);
                }
                else
                {
                    errfd = open(_errFile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                }

                if (errfd < 0)
                {
                    perror("err : create outfile");
                     prompt();
                    return;
                   // exit(2);
                }
            }
            else
            {
                errfd = defaulterr;
            }

            for (int i = 0; i < _numberOfSimpleCommands; i++)
            {
                if (i == 0)
                {
                    if (_inputFile)
                    {
                        dup2(inpfd, 0);
                        close(inpfd);
                    }
                    else
                    {
                        dup2(defaultin, 0);
                    }
                }

                if (i != 0)
                {
                    dup2(fdpipe[0], 0);
                    close(fdpipe[0]);
                    close(fdpipe[1]);
                    if (pipe(fdpipe) == -1)
                    {
                        p+
                    
                       // exit(2);
                    }
                }

                if (i == _numberOfSimpleCommands - 1)
                {
                    if (_outFile)
                    {
                        dup2(outfd, 1);
                        close(outfd);
                    }
                    else
                    {
                        dup2(defaultout, 1);
                    }

                    if (_errFile)
                    {
                        dup2(errfd, 2);
                        close(errfd);
                    }

                    if (!_errFile)
                    {
                        dup2(defaulterr, 2);
                    }
                }

                if (i != _numberOfSimpleCommands - 1)
                {
                    dup2(fdpipe[1], 1);
                }

                pid = fork();
                if (pid == -1)
                {
                    perror("err : fork\n");
                    exit(2);
                }

                if (pid == 0)
                {
                    // Child
                    close(inpfd);
                    close(outfd);
                    close(fdpipe[0]);
                    close(fdpipe[1]);
                    close(defaultin);
                    close(defaultout);
                    close(defaulterr);

                    execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);

                    perror("err: exec");
                    clear();
                    prompt();
                    return;
                  
                   // exit(2);
                }

                if (pid > 0)
                {
                    dup2(defaultin, 0);
                    dup2(defaultout, 1);
                    dup2(defaulterr, 2);
                    if (!_background)
                    {
                        waitpid(pid, 0, 0);
                    }
                }
            }

            dup2(defaultin, 0);
            dup2(defaultout, 1);
            dup2(defaulterr, 2);

            close(fdpipe[0]);
            close(fdpipe[1]);
            close(inpfd);
            close(outfd);
            close(errfd);
            close(defaultin);
            close(defaultout);
            close(defaulterr);
        }
    }
    clear();
    prompt();
}

void Command::prompt()
{
    printf("myshell>");
    fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

void INTChandler(int sig)
{
    char buff[100];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buff, sizeof(buff), "Process terminated at : %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    std::ofstream outfile;
    outfile.open("log.txt", std::ios_base::app);
    outfile << buff;
    outfile.close();
    return;
}

int yyparse(void);

int main()
{
    Command::_currentCommand.prompt();
    signal(SIGINT, handleCtrlC);
    signal(SIGCHLD, INTChandler);
    yyparse();
    return 0;
}
