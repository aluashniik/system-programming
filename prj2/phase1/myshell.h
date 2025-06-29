#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#define	MAXLINE	 8192  /* Max text line length */
#define MAXARGS  128

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void unix_error(char *msg);
pid_t Fork(void);
