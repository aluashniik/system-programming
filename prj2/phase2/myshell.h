#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#define	MAXLINE	 8192  /* Max text line length */
#define MAXARGS  128

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void unix_error(char *msg);
pid_t Fork(void);
int Dup2(int fd1, int fd2);
int if_pipe(char **argv);
void pipe_eval(char **argv);
void split_pipe(char **argv, char **left, char **right);
pid_t wait(int *status);