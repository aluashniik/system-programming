#include "csapp.h"
#include <semaphore.h>
#include <pthread.h>

#include <sys/time.h>//시간 측정하기 위해 

//주식 정보를 관리하기 위한 바이너리 트리 구조
// <stock_id> <left_stock> <price>
struct stock{
    int id;
    int left_stock;
    int price;
    struct stock *left, *right;
    int cnt;
};

//global variables
struct stock* stree = NULL;
sem_t stock_mutex; //이진 탐색 트리의 루트 

void *thread(void *vargp);
void echo_handler(int connfd);
void init_pool(int listenfd);
void add_client(int connfd);
void cmd_action(int connfd, char* buf);
struct stock* load_stock(char *filename);
void write_stock(char *filename, struct stock *bburi);
int buy_handle(struct stock* bburi, int ID, int NUM);
int sell_handle(struct stock* bburi, int ID, int NUM);
void print_stock(struct stock* bburi, char *output);
struct stock* find_stock(struct stock* crnt, int f_id);
struct stock* newitem(int ID, int left_stock, int price);
struct stock* insert_stock(struct stock* bburi, struct stock* item);


void cmd_action(int connfd, char* bufc){
    int ID, NUM;
    char jumun[10];
    char output[MAXLINE]={0};

    if(!strncmp(bufc, "show", 4)){
        print_stock(stree, output);
        strcpy(bufc, output);
    }
    else if (!strncmp(bufc, "buy", 3)){
        sscanf(bufc, "%s %d %d", jumun, &ID, &NUM);
        if(buy_handle(stree, ID, NUM)){
            strcpy(bufc, "[buy] success\n");
            write_stock("stock.txt", stree);
        }else{
            strcpy(bufc, "Not enough left stock\n");
        }
    }else if (!strncmp(bufc, "sell", 4)) {
        sscanf(bufc, "%s %d %d", jumun, &ID, &NUM);
        if (sell_handle(stree, ID, NUM)) {
            strcpy(bufc, "[sell] success\n");
            write_stock("stock.txt", stree);
        }
    }

}


void echo_handler(int connfd){
    size_t n;
    char buf[MAXBUF];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE))!=0){
        printf("Server recieved %zu bytes\n", n);
        if(!strcmp(buf, "exit\n")){
            break;
        }
        P(&stock_mutex);
        cmd_action(connfd, buf);
        V(&stock_mutex);
        
        size_t len=strlen(buf);
        if(len<MAXBUF){
            memset(buf+len, 0, MAXBUF-len);
        }

        Rio_writen(connfd, buf, MAXBUF);
    }
}

 //제일 마지막에 총 몇초 걸렸는지 알려줌
void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);

    //struct timeval start, end;
    //gettimeofday(&start, NULL); // Start timing the client session

    echo_handler(connfd); // Handle all client requests

    //gettimeofday(&end, NULL);   // End timing

    // Calculate elapsed time in milliseconds or microseconds
    // long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    // double elapsed_sec = elapsed_us / 1e6;

    // // printf("Client connection handled in %.3f seconds (%ld μs)\n", elapsed_sec, elapsed_us);
    // printf("Client connection handled in %.3f seconds\n", elapsed_sec);

    Close(connfd);
    return NULL;
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
    jana->price = price;
    jana->left_stock = left_stock;
    jana->cnt = 0;
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
        struct stock* temp= newitem(id, quantity, price);
        bburi=insert_stock(bburi, temp);
    }
    fclose(fp);
    return bburi;
}

//주식 트리에서 특정 id를 가진 노드 탐색색
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
    struct stock* b = find_stock(bburi, ID);
    int result=0;
    if(b!=NULL && b->left_stock >= NUM){
        b->left_stock -= NUM;
        result = 1;
    }
    return result;
}

//주식 판매 처리
int sell_handle(struct stock* bburi, int ID, int NUM) {
    struct stock* s = find_stock(bburi, ID);
    int result=0;
    if(s != NULL){
        s->left_stock += NUM;
        result = 1;
    }
    return result;
}

void print_stock(struct stock* bburi, char *output) {
    if (bburi == NULL) 
        return;
    print_stock(bburi->left, output);
    char line[64];
    sprintf(line, "%d %d %d\n", bburi->id, bburi->left_stock, bburi->price);
    strcat(output, line);
    print_stock(bburi->right, output);//출력 문자열에 저장장
}

//현재 트리 상태를 파일에 저장장
void write_stock(char *filename, struct stock *bburi) {
    FILE *fp = fopen(filename, "w");
    if(!fp){
        perror("fopen");
        return;
    }

    char bufc[MAXBUF]={0};
    print_stock(bburi, bufc);//문자열로 변환환
    fprintf(fp, "%s", bufc);
    fclose(fp);
}


int main(int argc, char **argv){
    int listenfd, *connfd;
    socklen_t clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    //port=atoi(argv[1]);

    Sem_init(&stock_mutex, 0, 1);
    P(&stock_mutex);
    stree=load_stock("stock.txt");
    V(&stock_mutex);

    listenfd=Open_listenfd(argv[1]);
    while(1){
        connfd=Malloc(sizeof(int));
        *connfd=Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        Pthread_create(&tid, NULL, thread, connfd);
    }
}