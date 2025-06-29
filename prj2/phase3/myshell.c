#include <stdio.h>
#include "myshell.h"

/*
 Your shell must start a command in the background if an ‘&’ is given in the command line arguments. Besides,
 your shell must also provide various built-in commands that support job control.
 Following shell commands with piping can be evaluated, e.g.,
 • jobs: List the running and stopped background jobs.
 • bg ⟨job⟩: Change a stopped background job to a running background job.
 • fg ⟨job⟩: Change a stopped or running a background job to a running in the foreground.
 • kill ⟨job⟩: Terminate a job
 Note that one should not be required to separate the ‘’ from the command by a space. For example, the
 commands ‘sort foo.txt &’, and ‘sort foo.txt&’ and ‘sort foo.txt &’ (blanks after the ampersand) are
 all valid.

 Hints: When pressing ctrl-c causes a SIGINT signal to be delivered to each process in the foreground job. The
 default action for SIGINT is to terminate the process. Similarly, pressing ctrl-z causes a SIGTSTP signal to be
 delivered to each process in the foreground job. The default action for SIGTSTP is to place a process in the
 stopped state, where it remains until it is awakened by receiving a SIGCONT signal.*/

//ctrl-c, ctrl-z에 대한 핸들러
void sigint_handler(int sig){
    if(fg_pid != 0){
        Kill(-fg_pid, SIGINT);
    }
}

void sigtstp_handler(int sig){
    if(fg_pid != 0){
        Kill(-fg_pid, SIGTSTP);
    }
}

void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    
    // Reap all terminated children
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job_t *job = job_get(pid);
        if (!job) continue;
        
        if (WIFSTOPPED(status)) {
            job_t *j = job_get(pid);
            job->state = ST;
            printf("[%d] Stopped                 %s", j->jid, j->cmdline);
        }
        else if (WIFSIGNALED(status)) {
            //printf("[%d] (%d) Terminated            %d\n", job->jid, pid, WTERMSIG(status));
            job_remove(pid);
        }
        else if (WIFEXITED(status)) {
            // Only remove if it's a background job (foreground jobs are removed in eval)
            if (job->state == BG) {
                job_remove(pid);
            }
        }
    }
    
    errno = olderrno;
}

int main(){
    char cmdline[MAXLINE];
    char *cmds[MAXARGS];  // 파이프 단위로 자른 명령어들
    char *args[MAXARGS];  // 각 명령어의 인자들
    
    Signal(SIGINT, sigint_handler);   // Ctrl-C
    Signal(SIGTSTP, sigtstp_handler); // Ctrl-Z
    Signal(SIGCHLD, sigchld_handler); // Child termination/stop

    while(1){
        printf("CSE4100-SP-P2>");
        fgets(cmdline, MAXLINE, stdin);
        if(feof(stdin)){
            exit(0);
        }
        eval(cmdline);
    }
    
}

//job hanldling
int jobs_handler(int argc, char **argv){
    jobs_list();
    return 1;
}

int bg_handler(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: bg <jobid>\n");
        return 1;
    }
    int jid = atoi(argv[1] + 1); // %1 형식에서 숫자 추출
    job_t *j = job_getById(jid);
    if (!j) {
        printf("bg: No such job\n");
        return 1;
    }
    if (j->state == ST) { // Stopped 상태인 경우
        j->state = BG; // 상태를 Running으로 변경
        Kill(-j->pid, SIGCONT); // SIGCONT 보내기
        printf("[%d] (%d) %s", j->jid, j->pid, j->cmdline);
    } else {
        printf("bg: Job is already running\n");
    }
    return 1;
}

int fg_handler(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: fg <jobid>\n");
        return 1;
    }
    int jid = atoi(argv[1] + 1); // %1 형식에서 숫자 추출
    job_t *j = job_getById(jid);
    if (!j) {
        printf("fg: No such job\n");
        return 1;
    }
    // SIGCONT 보내서 다시 실행
    Kill(-j->pid, SIGCONT);
    j->state = FG; // 상태를 Foreground로 변경
    int status;
    waitpid(j->pid, &status, WUNTRACED); // Ctrl+Z 대응 위해 WUNTRACED
    if (WIFSTOPPED(status)) {
        j->state = ST; // 상태를 Stopped로 변경
    } else {
        job_remove(j->pid); // 작업 목록에서 제거
    }
    return 1;
}

int kill_handler(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: kill <jobid>\n");
        return 1;
    }
    int jid = atoi(argv[1] + 1);
    job_t *j = job_getById(jid);
    if (!j) {
        printf("kill: No such job\n");
        return 1;
    }
    Kill(-j->pid, SIGKILL); // SIGKILL 보내기
    job_remove(j->pid); // 작업 목록에서 제거
    return 1;
}

int builtin_command(char **argv) 
{
    int argc = 0;
    while (argv[argc] != NULL) argc++;

    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);  
    if(!strcmp(argv[0], "exit"))
        exit(0);
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    if(!strcmp(argv[0], "jobs"))
        return jobs_handler(argc, argv);
    if(!strcmp(argv[0], "bg"))
        return bg_handler(argc, argv);
    if(!strcmp(argv[0], "fg"))
        return fg_handler(argc, argv);
    if(!strcmp(argv[0], "kill"))
        return kill_handler(argc, argv);
    return 0;                     /* Not a builtin command */
}

//pipe인지 체크
int if_pipe(char **argv){
    for(int i=0; argv[i]; i++){
        if(strcmp(argv[i], "|") == 0)
            return 1;
    }
    return 0;
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


void eval(char *cmdline) 
{
    char buf[MAXLINE];   /* Holds modified command line */
    char *argv[MAXARGS]; /* Argument list for execve() */
    int bg;              /* Should the job run in background? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    
    // 파이프 명령어 처리
    if (strchr(cmdline, '|') != NULL) {
        parseline(buf, argv);
        pid = Fork();
        if (pid == 0) { // 자식 프로세스
            pipe_eval(argv);  // 파이프 처리
            exit(0);          // 처리 후 종료
        } else { // 부모 프로세스
            int status;
            waitpid(pid, &status, 0);  // 자식 프로세스 완료 대기
            return;
        }
    }

    // 일반 명령어 처리
    bg = parseline(buf, argv); // 명령어 파싱 및 백그라운드 여부 확인
    
    if (argv[0] == NULL)  
        return;   /* 빈 줄 무시 */

    // 내장 명령어 처리
    if (builtin_command(argv))
        return;

    // cd 명령어 처리 (부모 프로세스에서 실행)
    if (strcmp(argv[0], "cd") == 0) {
        if (chdir(argv[1]) < 0) {
            printf("cd: %s: No such file or directory\n", argv[1]);
        }
        return;
    }

    // 새로운 자식 프로세스 생성
    if ((pid = Fork()) == 0) {   /* Child process */
        Setpgid(0, 0); // Job control을 위한 새 프로세스 그룹 설정
        if (execvp(argv[0], argv) < 0) { // 명령어 실행
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }

    // 부모 프로세스에서 작업 관리
    if (!bg) { // Foreground job
        fg_pid = pid; // 현재 foreground job으로 설정
        job_add(pid, FG, cmdline);
        int status;
        //ctrl+c, ctrl+z 처리를 다시 여기서도 처리, 중복돼도 ok
        waitpid(pid, &status, WUNTRACED); // Ctrl+Z 처리를 위해 WUNTRACED 옵션 사용
        if (WIFSTOPPED(status)) { // Ctrl+Z에 의해 정지된 경우
            job_t *j = job_get(pid);
            j->state = ST; // Stopped 상태로 변경
            printf("\n[%d] Stopped                 %s", j->jid, j->cmdline);
        } else if (WIFSIGNALED(status)) {
            job_t *j = job_get(pid);
            printf("\n");//아무것도 출력 안 해도 됨됨
            job_remove(pid);
        } else {
            job_remove(pid);
        }
        fg_pid = 0; // foreground job 초기화
    } else { // Background job
        job_t *j = job_add(pid, BG, cmdline); // 새 작업 추가
        printf("[%d] %d\n", j->jid, j->pid); // 
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


int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg = 0;          /* Background job? */

    // Trailing '\n' or ' '를 제거
    buf[strlen(buf)-1] = ' ';  

    // Leading spaces 무시
    while (*buf && (*buf == ' ')) 
        buf++;

    // Build the argv list
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        *delim = '\0'; // Null-terminate the current argument
        argv[argc++] = buf;
        buf = delim + 1;

        // Ignore spaces
        while (*buf && (*buf == ' ')) 
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0)  /* Ignore blank line */
        return 0;

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

    // Check for background job
    if (argc > 0) {
        char *last_arg = argv[argc-1];
        if (strcmp(last_arg, "&") == 0) {
            // Standalone &
            argv[--argc] = NULL; // Remove "&" from the argument list
            bg = 1; // Set background flag
        } else if (strlen(last_arg) > 0 && last_arg[strlen(last_arg)-1] == '&') {
            // Command ending with &, like "sleep 100&"
            last_arg[strlen(last_arg)-1] = '\0'; // Remove "&" from the last argument
            // If we now have an empty string, remove it
            if (strlen(last_arg) == 0) {
                argv[--argc] = NULL;
            }
            bg = 1; // Set background flag
        }
    }

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
/////////////////////////////////////////////////

/* $begin kill */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

//job handling
int max_jid(){
    int max=0;
    job_t *j = jobs;
    while(j){
        if(j->jid>max)
           max=j->jid;
        j=j->next;
    }
    return max;
}


job_t* job_add(pid_t pid, int state, char *cmdline) {
    job_t *new = malloc(sizeof(job_t));
    // if (!new) {
    //     unix_error("malloc error in job_add");
    //     return NULL;
    // }
    new->pid = pid;
    new->jid = max_jid() + 1; // 새 JID 할당
    new->state = state;
    strcpy(new->cmdline, cmdline);
    new->next = jobs;
    jobs = new;
    return new; // 새로 생성된 작업 포인터 반환
}


void job_remove(pid_t pid) {
    job_t **curr = &jobs;
    while (*curr) {
        if ((*curr)->pid == pid) {
            job_t *tmp = *curr;
            *curr = (*curr)->next;
            free(tmp);
            return;
        }
        curr = &(*curr)->next;
    }
}
job_t* job_get(pid_t pid) {
    job_t *j = jobs;
    while (j) {
        if (j->pid == pid)
            return j;
        j = j->next;
    }
    return NULL;
}

job_t* job_getById(int jid) {
    job_t *j = jobs;
    while (j) {
        if (j->jid == jid)
            return j;
        j = j->next;
    }
    return NULL;
}

void jobs_list(void) {
    //jobs로 쳐서 리스트로 나열해주기기
    job_t *j = jobs;
    while (j) {
        char *state;
        if (j->state == BG) state = "Running";
        else if (j->state == ST) state = "Stopped";
        else state = "Foreground";

        printf("[%d] %s                 %s", j->jid, state,j->cmdline);
        j = j->next;
    }
}


