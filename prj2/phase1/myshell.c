#include <stdio.h>
#include "myshell.h"

// Reading: Read the command from standard input.
//  input = myshell readinput ()
// // Parsing: transform the input string into command line arguments.
// args = myshell parseinput (input);
// // Executing: Execute the command by forking a child process and return to parent process.
// myshell execute(args);


int main(){
    char cmdline[MAXLINE];
    while(1){
        printf("CSE4100-SP-P2>");
        fgets(cmdline, MAXLINE, stdin);
        if(feof(stdin)){
            exit(0);
        }
        eval(cmdline);
    }
}

void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); //명령어 문자열을 공백 기준으로 나눠서 argv[]에 저장
    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */

    //cd를 쉘 안에서 실행하니까 부모 프로세스 안에 해야 됨, int chdir(const char *path); 성공시 0, 실패시 1

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if(strcmp(argv[0], "cd")==0){
            chdir(argv[1]);
        }
        else{
            /*새로운 자식 프로세스를 생성하고 그 안에서 execvp()로 명령 실행, 
            부모 프로세스는 waitpid()로 기다리거나, 백그라운드라면 그냥 다음 루프로 넘어감*/
            if ((pid = Fork()) == 0) {   /* Child runs user job */
                if (execvp(argv[0], argv) < 0) { ////ex) /bin/ls ls -al &,  기존 environ에서 에러가 나서 수정
                    printf("%s: Command not found.\n", argv[0]);
                    exit(0);
                }
            }
            /* Parent waits for foreground job to terminate */
            if (!bg) {
                int status;
                if (waitpid(pid, &status, 0) < 0)
                unix_error("waitfg: waitpid error");
            }
            else //when there is backgrount process!
                printf("%d %s", pid, cmdline);
            }
        }
    return;
}

int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);  
    if(!strcmp(argv[0], "exit"))
        exit(0);
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    return 0;                     /* Not a builtin command */
}

int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}

