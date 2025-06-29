#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
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

typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Kill(pid_t pid, int signum);
void Setpgid(pid_t pid, pid_t pgid);
pid_t Getpgrp();

int jobs_handler(int argc, char **argv);
int fg_handler(int argc, char **argv);
int bg_handler(int argc, char **argv);
int kill_handler(int argc, char **argv);

typedef enum { FG, BG, ST } job_state;

typedef struct job {
    int jid;
    pid_t pid;
    job_state state;
    char cmdline[MAXLINE];
    struct job *next;
} job_t;

pid_t fg_job_pid(void);

job_t* job_add(pid_t pid, int state, char *cmdline);
void job_remove(pid_t pid);
job_t* job_get(pid_t pid);
job_t* job_getById(int jid);
void jobs_list();
int max_jid(void);
void sigchld_handler(int sig);

job_t *job_list = NULL;
int next_kid=1;
static job_t *jobs = NULL;
pid_t fg_pid = 0; // 전역 변수로 foreground job pid 저장

#endif