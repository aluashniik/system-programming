/*
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#include <semaphore.h>

//my port number - 60117

//adding mutex for thread safety
sem_t stock_mutex;

//주식 정보를 관리하기 위한 이진 탐색색 트리 구조체체
// <stock_id> <left_stock> <price>
struct stock{
    int id;
    int left_stock;
    int price;
    struct stock *left, *right;
};

//stock_t *stock=NULL;

typedef struct{
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
    struct timeval start_time[FD_SETSIZE];
} pool;

//int byte_cnt = 0;
struct stock* stree; //이진 탐색 트리의 루트

void echo(int connfd);
int cnt_active(pool *p);
void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);
void cmd_action(char* bufc);
struct stock* load_stock(char *filename);
void write_stock(char *filename, struct stock *bburi);
int buy_handle(struct stock* bburi, int ID, int NUM);
int sell_handle(struct stock* bburi, int ID, int NUM);
void print_stock(struct stock* bburi, char *output);
struct stock* find_stock(struct stock* crnt, int f_id);
struct stock* newitem(int ID, int left_stock, int price);
struct stock* insert_stock(struct stock* bburi, struct stock* item);

//활성 클라인트가 하나라도 있음 9, 없음 1
int cnt_active(pool *p){
    //int cnt=0;
    for(int i=0; i<=p->maxi; i++){
        if (p->clientfd[i] != -1)
            return 0;
    }
    return 1;
}

void init_pool(int listenfd, pool *p){
    int i;
    p->maxi=-1;//clientfd 배열의 최대 인덱스 초기화
    for(i=0; i<FD_SETSIZE; i++){
        p->clientfd[i]=-1;//클라인트 fd 모두 -1로 초기화 
    }
    p->maxfd=listenfd;//
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->ready_set); 
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p){
    int i;
    p->nready--; 
    for(i=0; i<FD_SETSIZE; i++){
        if(p->clientfd[i]<0){
            p->clientfd[i]=connfd;
            Rio_readinitb(&p->clientrio[i], connfd);
            
            ////타인 측정 시간간
            //gettimeofday(&p->start_time[i], NULL);

            FD_SET(connfd, &p->read_set);
            if(connfd>p->maxfd)
                p->maxfd=connfd;
            if(i>p->maxi)
                p->maxi=i;
            break;
        }
    }
    if(i==FD_SETSIZE)
        app_error("add_client error: Too many clients");
}

void check_clients(pool *p){
    int i,connfd,n;
    char buf[MAXBUF];
    rio_t *rio;

    for(i=0;(i<=p->maxi) && (p->nready>0); i++){
        buf[0]='\0';
        connfd = p->clientfd[i];
        rio = &p->clientrio[i];
        if((connfd>0) && (FD_ISSET(connfd,&p->ready_set))){//준비된 fd면
            //n=Rio_readnb(rio,buf,MAXLINE);
            p->nready--;
            n=Rio_readlineb(rio, buf, MAXLINE); //한줄씩 읽음
            printf("Server received %d bytes\n", n);
            if(n<=0||!strncmp(buf, "exit", 4)){//EOF or 'exit'명령 시 연결 종료
                // struct timeval end_time;
                // gettimeofday(&end_time, NULL);
                // long elapsed_us = (end_time.tv_sec - p->start_time[i].tv_sec) * 1000000L + 
                //                  (end_time.tv_usec - p->start_time[i].tv_usec);
                // double elapsed_sec = elapsed_us / 1e6;
                
                // printf("Client connection handled in %.3f seconds\n", elapsed_sec);
                
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }else{
                cmd_action(buf);//명령 처리
                Rio_writen(connfd, buf, MAXLINE);
            }
        }
    }
}

void cmd_action(char* bufc){
    int ID, NUM;
    char jumun[10];
    if(!strncmp(bufc, "show", 4)){
        bufc[0]= '\0';
        print_stock(stree, bufc);//주식 정보 출력
        bufc[strlen(bufc)-1]='\n'; //마지막 줄바꿈 처리
    }
    else if(!strncmp(bufc, "buy", 3)){
        sscanf(bufc, "%s %d %d\n", jumun, &ID, &NUM);
        if(buy_handle(stree, ID, NUM))//성공 시
            strcpy(bufc, "[buy] success\n");
        else
            strcpy(bufc, "Not enough left stock\n");
    }
    else if(!strncmp(bufc, "sell", 4)){
        sscanf(bufc, "%s %d %d\n", jumun, &ID, &NUM);
        sell_handle(stree, ID, NUM);
        strcpy(bufc, "[sell] success\n");
    } //exit은 위에서 처리
}

struct stock* insert_stock(struct stock* bburi, struct stock* item){
    if(bburi==NULL) return item;
    if(bburi->id < item->id){
        bburi->right = insert_stock(bburi->right, item);
    }else if(bburi->id > item->id){
        bburi->left = insert_stock(bburi->left, item);
    }else if(bburi->id == item->id){
        //동일 id가 있음 가격과 재고 업데이트
        bburi->price = item->price;
        bburi->left_stock = item->left_stock;
    }  
    return bburi;
}

//새 주식 노드 생성 함수
struct stock* newitem(int ID, int left_stock, int price){
    struct stock* jana = (struct stock*)malloc(sizeof(struct stock));
    if(jana==NULL){
        printf("ERROR\n");
        return NULL;
    }

    jana->id = ID;
    jana->left_stock = left_stock;
    jana->price = price;
    jana->left = NULL;
    jana->right = NULL;
    return jana;
}

//파일에서 주식 데이터 로드하여 트리 생성
struct stock* load_stock(char *filename){
    FILE *fp = fopen(filename, "r");
    if(!fp){
        perror("fopen");
        exit(1);
    }
    struct stock* bburi = NULL;
    int id, quantity, price;
    while(fscanf(fp, "%d %d %d", &id, &quantity, &price)!=EOF){
        struct stock* temp = newitem(id, quantity, price);
        bburi=insert_stock(bburi, temp);
    }
    fclose(fp);
    return bburi;
}

//주식 트리에서 특정 id를 가진 노드 탐색
struct stock* find_stock(struct stock* crnt, int f_id){
    if(crnt==NULL) 
        return NULL;
    if(crnt->id==f_id)
        return crnt;
    if(crnt->id>f_id) 
        return find_stock(crnt->left, f_id);
    if(crnt->id<f_id) 
        return find_stock(crnt->right, f_id);
    return crnt;
}

//주식 구매 처리
int buy_handle(struct stock* bburi, int ID, int NUM) {
    P(&stock_mutex); //lock
    struct stock* b = find_stock(bburi, ID);
    int result=0;
    if(b!=NULL && b->left_stock >= NUM){//재고가 충분하면
        b->left_stock -= NUM;//감소
        result = 1;//성공 반환
    }
    V(&stock_mutex); //unlock
    return result;
}

//주식 판매 처리
int sell_handle(struct stock* bburi, int ID, int NUM) {
    P(&stock_mutex); //lock
    struct stock* s = find_stock(bburi, ID);
    int result=0;
    if(s != NULL){
        s->left_stock += NUM;//재고 증가 
        result = 1;
    }
    V(&stock_mutex); //unlock
    return result;
}

//
void print_stock(struct stock* bburi, char *output) {
    if (bburi == NULL) 
        return;
    print_stock(bburi->left, output);
    char line[64];
    sprintf(line, "%d %d %d\n", bburi->id, bburi->left_stock, bburi->price);
    strcat(output, line);
    print_stock(bburi->right, output);//출력 문자열에 저장 
}

//현재 트리 상태태를 파일에 저장 
void write_stock(char *filename, struct stock *bburi) {
    P(&stock_mutex); //lock
    FILE *fp = fopen(filename, "w");
    if(!fp){
        V(&stock_mutex);//unlock
        perror("fopen");
        return;
    }

    char bufc[MAXBUF]={0};
    print_stock(bburi, bufc);//문자열로 변환 
    fprintf(fp, "%s", bufc);
    fclose(fp);
    V(&stock_mutex); //unlock
}

int main(int argc, char **argv) 
{
    Sem_init(&stock_mutex, 0, 1);//세마포어 초기화 

    int listenfd, connfd;
    socklen_t clientlen = sizeof(struct sockaddr_storage);
    struct sockaddr_storage clientaddr; /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    static pool pool;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    
    stree=load_stock("stock.txt"); //초기 주식 데이터 로드

    while (1) {
        pool.ready_set=pool.read_set; //복사
        pool.nready=Select(pool.maxfd+1,&pool.ready_set,NULL,NULL,NULL);
        if(FD_ISSET(listenfd, &pool.ready_set)){//새러운 연결 요청 
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool);
            //echo(connfd);
	        //Close(connfd);
        }

        check_clients(&pool);
        
        if(cnt_active(&pool)){//더 이상 없으면
            write_stock("stock.txt", stree);//파일에 저장 
        }
    }
    exit(0);
}

/* $end echoserverimain */



