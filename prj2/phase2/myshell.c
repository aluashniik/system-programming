#include <stdio.h>
#include "myshell.h"

/*
 • ls | grep filename
 • cat filename | less
 • cat filename | grep-i "abc" | sort-r
  
 '|' -> pipe,  output of one command serves as input to the next
  
 파이프 기준으로 왼쪽과, 오른쪽을 정해서 각 부모랑 자식이 되게 하자
 
 먼저 파이프가 있는지 체크-> 있음, pipe evaluate. 없음, 그냥 eval

 pipe eval을 dup2로 입력과 출력을 연결 하기
  
 파이프가 여러개 들어올 수 있으니까 split 해주기기
*/

int main(){
    char cmdline[MAXLINE];
    char *cmds[MAXARGS];  // 파이프 단위로 자른 명령어들
    char *args[MAXARGS];  // 각 명령어의 인자들
    
    while(1){
        printf("CSE4100-SP-P2>");
        fgets(cmdline, MAXLINE, stdin);
        if(feof(stdin)){
            exit(0);
        }
        eval(cmdline);
    }
}

//pipe인지 체크
int if_pipe(char **argv){
    for(int i=0; argv[i]; i++){
        if(strcmp(argv[i], "|") == 0)
            return 1;
    }
    return 0;
}

void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);

    if (strchr(cmdline, '|') != NULL) {
        parseline(buf, argv);
    
        pid = Fork(); // fork 추가가
        if (pid == 0) {
            pipe_eval(argv);  //자식 프로세스가 파이프 처리
            exit(0);           //처리 끝나면 종료
        } else {
            int status;
            waitpid(pid, &status, 0);  //부모는 기다리고
            return;
        }
    }
    
    bg = parseline(buf, argv); //명령어 문자열을 공백 기준으로 나눠서 argv[]에 저장
    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */

    //cd를 쉘 안에서 실행하니까 부모 프로세스 안에 해야 됨, int chdir(const char *path); 성공시 0, 실패시 1
    if (!builtin_command(argv)) { //cd 명령어 받기
        if(strcmp(argv[0], "cd")==0){
            chdir(argv[1]);
        }
        else{
            /*새로운 자식 프로세스를 생성하고 그 안에서 execvp()로 명령 실행, 
            부모 프로세스는 waitpid()로 기다리거나, 백그라운드라면 그냥 다음 루프로 넘어감*/
            if ((pid = Fork()) == 0) {   /* Child runs user job */
                if (execvp(argv[0], argv) < 0) { //기존 environ에서 에러가 나서 수정
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
            else
                printf("%d %s", pid, cmdline);
            }
        }
    return;
}

void pipe_eval(char **argv){
    char *left[MAXARGS], *right[MAXARGS];
    if(!if_pipe(argv)){
        //파이프ㅏㄱ 없음 그냥 실행
        execvp(argv[0], argv);
        unix_error("execvp error");
    }

    split_pipe(argv, left, right);
    int fd[2];
    pipe(fd);

    pid_t pid = Fork(); //fork 호출해서 부모, 자식 만들기
    if(pid==0){//child
        close(fd[0]);
        Dup2(fd[1], STDOUT_FILENO); //출력이 됨
        close(fd[1]);
        pipe_eval(left);
    }
    else{ //parent
        close(fd[1]);
        Dup2(fd[0], STDIN_FILENO); //입력으로 받음
        close(fd[0]);
        //pipe_eval(right);
        pipe_eval(right);
    }
}

void split_pipe(char **argv, char **left, char **right){
    int i = 0, j = 0;
    while (argv[i] && strcmp(argv[i], "|") != 0) {
        left[i] = argv[i];
        i++;
    }
    left[i] = NULL; // 왼쪽 끝

    if (argv[i]) i++; // '|' 넘기기

    while (argv[i]) {
        right[j++] = argv[i++];
    }
    right[j] = NULL; // 오른쪽 끝
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

    /* Remove double quotes from arguments */
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '"' || argv[i][0] == '\'') {
            char *start = argv[i];
            char *end = argv[i] + strlen(argv[i]) - 1;
            if (*end == '"' || *end == '\'') {
                *end = '\0';       // remove trailing quote
                argv[i] = start + 1; // skip leading quote
            }
        }
    }

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

int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}

