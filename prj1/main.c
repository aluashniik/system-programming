#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "list.h"
#include "hash.h"
#include "bitmap.h"
#include <time.h>

// 전역 변수 선언
struct list list0, list1;
struct hash hash0, hash1;
struct bitmap *bitmaps[10] = {NULL}; //비트맵을 배열로 받기
//리스트랑 해시도 비트맵이랑 비슷하게 배열로 받으면 좋았을 것이다, 근데 각자 0인지1인지 구분하면서 해도 문제 없었다

///////////////////////////////////// list //////////////////////////////////////////////////

//리스트트 출력
void dumpdata_list(struct list *list) {
    struct list_elem *e;
    bool first = true;
    for (e = list_begin(list); e != list_end(list); e = list_next(e)) {
        struct foo *f = list_entry(e, struct foo, elem);
        if (!first) printf(" ");
        printf("%d", f->value);
        first = false;
    }
    printf("\n");
}

// 리스트 삭제
void delete_list(struct list *list) {
    while (!list_empty(list)) {//plain 돌릴때 timeout 안 뜨게게
        struct list_elem *e = list_pop_front(list);
        struct foo *f = list_entry(e, struct foo, elem);
        free(f);
    }
}

void list_swap(struct list *list, int index1, int index2) {
    //인접하는가, 안 하는가, 같는가 경들 있을 수 있음  
    // 0번째와 1을 교환
    struct list_elem *elem1 = NULL, *elem2 = NULL;
    struct list_elem *e; int i = 0;
    
    // index1과 index2에 해당하는 요소 찾기
    for (e = list_begin(list); e != list_end(list); e = list_next(e)) {
        if (i == index1) elem1 = e;
        if (i == index2) elem2 = e;
        i++;
    }
    
    if (elem1 && elem2) {
    // 두 요소의 value 교환
    struct foo *f1 = list_entry(elem1, struct foo, elem);
    struct foo *f2 = list_entry(elem2, struct foo, elem);
    int temp = f1->value;
    f1->value = f2->value;
    f2->value = temp;
    }
    
}

void list_shuffle(struct list *list) {
  /*
-Funclionality : Shuflle elements of list in the parameter
-Parameter : Pointer of list that will be shuffled
-Return value : None
쓸 수 있는 알고리즘이 많음,-> Fisher-Yates 셔플 써줬음음
*/

    // 리스트의 길이 계산
    int length = 0;
    struct list_elem *e;
    for (e = list_begin(list); e != list_end(list); e = list_next(e)) {
        length++;
    }
    // 리스트를 배열로 복사
    struct list_elem **array = malloc(length * sizeof(struct list_elem *));
    int i = 0;
    for (e = list_begin(list); e != list_end(list); e = list_next(e)) {
        array[i++] = e;
    }
    srand(time(NULL)); // 랜덤 시드 초기화
    for (i = length - 1; i > 0; i--) {
        int j = rand() % (i + 1); // 0부터 i까지의 랜덤 인덱스
        struct list_elem *temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
    // 섞인 배열을 기반으로 리스트 재구성
    for (i = 0; i < length; i++) {
        list_remove(array[i]);      //기존 위치에서 제거하고
        list_push_back(list, array[i]); //리스트의 끝에 추가
    }
    free(array); //배열 해제
}

//list_max, list_min 처리할때 필요함함
bool less_func(const struct list_elem *a, const struct list_elem *b, void *aux) {
    struct foo *fa = list_entry(a, struct foo, elem);
    struct foo *fb = list_entry(b, struct foo, elem);
    return fa->value < fb->value;
}
/////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////// hash //////////////////////////////////////////////////
struct hash_item {
    struct hash_elem hash_elem;
    int value;
};

unsigned my_hash_hash_func(const struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, hash_elem);
    return hash_int(item->value);
}

//hash less, 동작은 리스트랑 비슷슷
bool my_hash_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct hash_item *item_a = hash_entry(a, struct hash_item, hash_elem);
    struct hash_item *item_b = hash_entry(b, struct hash_item, hash_elem);
    return item_a->value < item_b->value;
}

void my_hash_action_func(struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, hash_elem);
    printf("%d ", item->value);
}

//해시 출력력
void dumpdata_hash(struct hash *h) {
    struct hash_iterator i;
    hash_first(&i, h);
    while (hash_next(&i)) {
        struct hash_item *item = hash_entry(hash_cur(&i), struct hash_item, hash_elem);
        printf("%d ", item->value);
    }
    printf("\n");
}

void delete_hash(struct hash *h) {
    hash_clear(h, NULL);
}

//요소의 값을 ^2
void square(struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, hash_elem);
    item->value = item->value * item->value;
}

//요소의 값을 ^3
void triple(struct hash_elem *e, void *aux) {
    struct hash_item *item = hash_entry(e, struct hash_item, hash_elem);
    item->value = item->value * item->value * item->value;  
}
////////////////////////////// bitmap /////////////////////////////////////////////////
//bitmap_size는 bm 이후 9개까지 들어올 수 있어서 인덱스를 받아옴
int get_bitmap(const char *name){
    if(strncmp(name, "bm", 2) == 0 && strlen(name) >= 3){
        int index = atoi(name + 2);
        if(index >= 0 && index < 10){
            return index;
        }
    }
    return -1;
}
//비트맵 출력력
void dumpdata_bitmap(struct bitmap *bm0){
    size_t size = bitmap_size(bm0); 
    for (size_t i = 0; i < size; i++) {
        printf("%d", bitmap_test(bm0, i)); 
    }  
    printf("\n");
}

//////////////////////////////////////////////////////////////////////////
void init_all(void) {
    list_init(&list0);
    list_init(&list1);
    hash_init(&hash0, my_hash_hash_func, my_hash_less_func, NULL);
    hash_init(&hash1, my_hash_hash_func, my_hash_less_func, NULL);

    // 비트맵 초기화
    size_t size = 1024;
    for(int i=0; i<10; i++){
        char name[4];
        snprintf(name, sizeof(name), "bm%d", i);
        bitmaps[i] = bitmap_create(size);
    }
}

int main() {
    char command[64];

    init_all(); // 리스트 초기화

    while (1) {
        if (scanf("%s", command) == EOF) break;

        if (strcmp(command, "create") == 0) {
            char type[64], name[64];
            //size_t size;
            scanf("%s %s", type, name);
            /////create list//////
            if (strcmp(type, "list") == 0){
                if(strcmp(name, "list0") == 0) {
                    list_init(&list0);
                }
                else if(strcmp(name, "list1") == 0){
                    list_init(&list1);
                }
            }
            ///////create hash/////
            else if (strcmp(type, "hashtable") == 0) {
                if (strcmp(name, "hash0") == 0) {
                    hash_init(&hash0, my_hash_hash_func, my_hash_less_func, NULL);
                }
                else if (strcmp(name, "hash1") == 0) {
                    hash_init(&hash1, my_hash_hash_func, my_hash_less_func, NULL);
                }
            }
            ////////////create bitmap/////////////////
            else if (strcmp(type, "bitmap") == 0) {
                size_t size;
                scanf("%zu", &size); 
                int index = get_bitmap(name);
                if(index != -1){
                    void *block = malloc(bitmap_buf_size(size)); 
                    bitmaps[index] = bitmap_create_in_buf(size, block, bitmap_buf_size(size));
                }
    
            }
        }            
        else if (strcmp(command, "dumpdata") == 0) {
            char type[64];
            scanf("%s", type);
            if (strcmp(type, "list0") == 0) {
                dumpdata_list(&list0);
            }
            else if (strcmp(type, "list1") == 0){
                dumpdata_list(&list1);
            }
            //////////////////////////////
            else if (strcmp(type, "hash0") == 0) {
                dumpdata_hash(&hash0);
            }
            else if (strcmp(type, "hash1") == 0) {
                dumpdata_hash(&hash1);
            }
            /////////////////////////
            else{
                int index = get_bitmap(type);
                if(index != -1 && bitmaps[index] != NULL){
                    dumpdata_bitmap(bitmaps[index]);
                }
            }
            
        } else if (strcmp(command, "list_push_back") == 0) {
            char name[64];
            int value;
            scanf("%s %d", name, &value);
            if (strcmp(name, "list0") == 0) {
                struct foo *new_foo = malloc(sizeof(struct foo));
                new_foo->value = value;
                list_push_back(&list0, &new_foo->elem);
            }
            else if (strcmp(name, "list1") == 0){
                struct foo *new_foo = malloc(sizeof(struct foo));
                new_foo->value = value;
                list_push_back(&list1, &new_foo->elem);
            }
        }
        else if (strcmp(command, "list_push_front") == 0) {
            char name[64];
            int value;
            scanf("%s %d", name, &value);
            if (strcmp(name, "list0") == 0) {
                struct foo *new_foo = malloc(sizeof(struct foo));
                new_foo->value = value;
                list_push_front(&list0, &new_foo->elem);
            }
            else if (strcmp(name, "list1") == 0){
                struct foo *new_foo = malloc(sizeof(struct foo));
                new_foo->value = value;
                list_push_back(&list1, &new_foo->elem);
            }
        } 
        else if (strcmp(command, "list_front") == 0) {
            char name[64];
            scanf("%s", name);
            if (strcmp(name, "list0") == 0) {
                struct list_elem *front = list_front(&list0);
                struct foo *f = list_entry(front, struct foo, elem);
                printf("%d\n", f->value);
            }
            
        } 
          else if (strcmp(command, "list_back") == 0) {
            char name[64];
            scanf("%s", name);
            if (strcmp(name, "list0") == 0) {
                struct list_elem *back = list_back(&list0);
                struct foo *f = list_entry(back, struct foo, elem);
                printf("%d\n", f->value);
            }
        } 
        else if (strcmp(command, "list_pop_front") == 0) {
            char name[64];
            scanf("%s", name);
            if (strcmp(name, "list0") == 0) {
                struct list_elem *front = list_pop_front(&list0);
                struct foo *f = list_entry(front, struct foo, elem);
                //제거만 함, 출력 안 해도 됨
                free(f);
            }
        } 
        else if (strcmp(command, "list_pop_back") == 0) {
            char name[64];
            scanf("%s", name);
            if (strcmp(name, "list0") == 0) {
                struct list_elem *back = list_pop_back(&list0);
                struct foo *f = list_entry(back, struct foo, elem);
                //제거만 함, -출력 안 해도 됨
                free(f);
            }
        }
        //list_empty 리스트 비어있음 true, 아니면 false
        else if(strcmp(command, "list_empty") == 0){
            char name[64];
            scanf("%s", name);
            if (strcmp(name, "list0") == 0) {
                printf("%s\n", list_empty(&list0) ? "true" : "false");
            }
            
        }
        else if(strcmp(command, "list_size") == 0){
            char name[64];
            scanf("%s", name);
            if(strcmp(name, "list0") == 0){ 
                printf("%zu\n", list_size(&list0)); //size_t의 출력 형식 -> zu
            }
        }
        else if(strcmp(command, "list_max") == 0){
            char name[64];
            scanf("%s", name);
            if(strcmp(name, "list0") == 0){
                struct list_elem *max_elem = list_max(&list0, less_func, NULL);
                struct foo *max_value = list_entry(max_elem, struct foo, elem);
                printf("%d\n", max_value->value);
            }
        }
        else if(strcmp(command, "list_min") == 0){
            char name[64];
            scanf("%s", name);
            if(strcmp(name, "list0") == 0){
                struct list_elem *min_elem = list_min(&list0, less_func, NULL);
                struct foo *min_value = list_entry(min_elem, struct foo, elem);
                printf("%d\n", min_value->value);
            }
        }

        else if(strcmp(command, "list_reverse") == 0){
            char name[64];
            scanf("%s", name);
            if(strcmp(name, "list0") == 0){
                list_reverse(&list0);
            }
        }
        else if (strcmp(command, "list_swap") == 0) {
            char name[64];
            int index1, index2;
            scanf("%s %d %d", name, &index1, &index2);
            if (strcmp(name, "list0") == 0) {
                list_swap(&list0, index1, index2);
            }
        } 

        else if (strcmp(command, "list_insert") == 0) {
            char name[64];
            int index, value;
            scanf("%s %d %d", name, &index, &value);
            if (strcmp(name, "list0") == 0) {
                struct foo *new_foo = malloc(sizeof(struct foo));
                new_foo->value = value;
                //인덱스 위치 찾기
                struct list_elem *e = list_begin(&list0);
                for (int i = 0; i < index; i++) {
                    e = list_next(e);
                }
                list_insert(e, &new_foo->elem);
            }
        }
        else if (strcmp(command, "list_insert_ordered") == 0) {
            char name[64];
            int value;
            scanf("%s %d", name, &value);
            if (strcmp(name, "list0") == 0) {
                struct foo *new_foo = malloc(sizeof(struct foo));
                new_foo->value = value;
                //정렬된 순서로 삽입
                list_insert_ordered(&list0, &new_foo->elem, less_func, NULL);
            }
        }
        else if (strcmp(command, "list_remove") == 0) {
            char name[64];
            int index;
            scanf("%s %d", name, &index);
            if (strcmp(name, "list0") == 0) {
                struct list_elem *e = list_begin(&list0);
                for (int i = 0; i < index; i++) {
                    e = list_next(e);
                }
                list_remove(e);
            }
        }
        else if (strcmp(command, "list_sort") == 0) {
            char name[64];
            scanf("%s", name);
            if (strcmp(name, "list0") == 0) {
                list_sort(&list0, less_func, NULL);
            }
        }
        else if (strcmp(command, "list_shuffle") == 0) {
            char list_name[10];
            struct list *list_ptr = NULL;
            if (strcmp(list_name, "list0") == 0) {
                list_ptr = &list0;
            } else if (strcmp(list_name, "list1") == 0) {
                list_ptr = &list1;
            }
            list_shuffle(list_ptr);
        }
        //ilst_splice list0 2 list1 1 4   (list0의 2번째에 list1의 1~4미만(3)까지를 잘라넣어라)
        else if (strcmp(command, "list_splice") == 0) {
            char list_dest_name[64], list_src_name[64];
            int dest_index, src_start, src_end;
            scanf("%s %d %s %d %d", list_dest_name, &dest_index, list_src_name, &src_start, &src_end);
            struct list *dest_list = NULL, *src_list = NULL;// 대상 리스트와 소스 리스트 확인
            if (strcmp(list_dest_name, "list0") == 0) {
                dest_list = &list0;
            } else if (strcmp(list_dest_name, "list1") == 0) {
                dest_list = &list1;
            }
            if (strcmp(list_src_name, "list0") == 0) {
                src_list = &list0;
            } else if (strcmp(list_src_name, "list1") == 0) {
                src_list = &list1;
            }
            //대상 위치(dest_index) 찾기
            struct list_elem *dest_elem = list_begin(dest_list);
            for (int i = 0; i < dest_index && dest_elem != list_end(dest_list); i++) {
                dest_elem = list_next(dest_elem);
            }
            //소스 범위(src_start ~ src_end 미만) 찾기
            struct list_elem *src_first = list_begin(src_list);
            for (int i = 0; i < src_start && src_first != list_end(src_list); i++) {
                src_first = list_next(src_first);
            }
            struct list_elem *src_last = src_first;
            for (int i = src_start; i < src_end && src_last != list_end(src_list); i++) {
                src_last = list_next(src_last);
            }
            list_splice(dest_elem, src_first, src_last);
        }
        else if (strcmp(command, "list_unique") == 0) {
            /*list_unique list0 list1
            list_unique list1 식으로 옴옴*/
            char line[256], list_name[10], dupl_name[10];
            struct list *list_ptr = NULL;
            struct list *dupl_ptr = NULL;
            int args;

            fgets(line, sizeof(line), stdin);
            args = sscanf(line, "%9s %9s", list_name, dupl_name);
        
            // list_name 유효성 검사
            if (strcmp(list_name, "list0") == 0) list_ptr = &list0;
            else if (strcmp(list_name, "list1") == 0) list_ptr = &list1;            
            if (args == 2) {
                if (strcmp(dupl_name, "list0") == 0) dupl_ptr = &list0;
                else if (strcmp(dupl_name, "list1") == 0) dupl_ptr = &list1;
            }
            if (args == 1 && list_ptr == &list1) list_sort(list_ptr, less_func, NULL);
        
            list_unique(list_ptr, dupl_ptr, less_func, NULL);
        }
        ///////////////////// hash /////////////////////////////
        else if (strcmp(command, "hash_insert") == 0) {
            char name[64]; 
            int value;  
            scanf("%s %d", name, &value); 
            struct hash_item *new_item = malloc(sizeof(struct hash_item));
            new_item->value = value;
            if (strcmp(name, "hash0") == 0) {
                hash_insert(&hash0, &new_item->hash_elem);
            } else if (strcmp(name, "hash1") == 0) {
                hash_insert(&hash1, &new_item->hash_elem);
            }
        }
        else if (strcmp(command, "hash_apply") == 0) {
            char name[64];
            char action_name[64];
            scanf("%63s %63s", name, action_name);  //버퍼 오버플로우 방지
            if (strcmp(name, "hash0") == 0) {
                if (strcmp(action_name, "square") == 0) {
                    hash_apply(&hash0, square);  // square 함수 적용
                } else if (strcmp(action_name, "triple") == 0) {
                    hash_apply(&hash0, triple);  // triple 함수 적용
                } 
            }
        }
        else if(strcmp(command, "hash_delete") == 0) {
            char name[64]; 
            int value;  
            scanf("%s %d", name, &value);
            struct hash_item *ditem = malloc(sizeof(struct hash_item));
            ditem->value = value;
            if (strcmp(name, "hash0") == 0) {
                hash_delete(&hash0, &ditem->hash_elem);
            } 
        }
        else if (strcmp(command, "hash_empty") == 0) {//비어있으면 true
            char name[64];
            scanf("%s", name);  
            if (strcmp(name, "hash0") == 0) {
                printf("%s\n", hash_empty(&hash0) ? "true" : "false");
            }
        }
        else if(strcmp(command, "hash_size")==0){
            char name[64];
            scanf("%s", name);
            if(strcmp(name, "hash0") == 0){
                //size_t의 출력 형식 -> zu
                printf("%zu\n", hash_size(&hash0));
            }
        }
        else if(strcmp(command, "hash_clear")==0){//hash 싹 비우기
            char name[64];
            scanf("%s", name);
            if(strcmp(name, "hash0") == 0){
                hash_clear(&hash0, NULL);//destructor를 NULL로 전달
            }
        }
        else if(strcmp(command, "hash_replace")==0){//10을 추가하되, 있으면 교체한다
            char name[64]; 
            int value;  
            scanf("%s %d", name, &value);
            struct hash_item *ritem = malloc(sizeof(struct hash_item));
            ritem->value = value;
            if (strcmp(name, "hash0") == 0) {
                hash_replace(&hash0, &ritem->hash_elem);
            } 
        }
        else if (strcmp(command, "hash_find") == 0) {//10이 있으면 10 출력, 없으면 출력x
            char name[64];
            int value;
            scanf("%s %d", name, &value);  
            struct hash_item *fitem = malloc(sizeof(struct hash_item));
            fitem->value = value;
            if (strcmp(name, "hash0") == 0) {
                struct hash_elem *found_elem = hash_find(&hash0, &fitem->hash_elem);
                if (found_elem != NULL) {
                    struct hash_item *item = hash_entry(found_elem, struct hash_item, hash_elem);
                    printf("%d\n", item->value);  
                }
            }
            free(fitem);  
        }
        /////////////////////// bitmap /////////////////
        else if (strcmp(command, "bitmap_size") == 0) {
            char name[64];
            scanf("%s", name);
            int index = get_bitmap(name);
            printf("%zu\n", bitmap_size(bitmaps[index]));
        }
        //bitmap_mark bm0 5		(5번째를 1로 바꿔라) 
        else if (strcmp(command, "bitmap_mark") == 0) {
            char name[64];
            size_t bit_idx;
            scanf("%s %zu", name, &bit_idx);
            int index = get_bitmap(name);
            bitmap_mark(bitmaps[index], bit_idx);
        }
        //bitmap_all bm0 7 3 	(index가 7부터 3개의 비트가	 1인지 True false) 
        else if (strcmp(command, "bitmap_all") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            scanf("%s %zu %zu", name, &start_idx, &cnt_idx);
            int index = get_bitmap(name);
            //bitmap_all(bitmaps[index], start_idx, cnt_idx);
            printf("%s\n", bitmap_all(bitmaps[index], start_idx, cnt_idx) ? "true" : "false");
        }
        //bitmap_any bm0 0 3 	(index가 0부터 3개의 비트가 적어도 1이 있는지) 
        else if (strcmp(command, "bitmap_any") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            scanf("%s %zu %zu", name, &start_idx, &cnt_idx);
            int index = get_bitmap(name);
            printf("%s\n", bitmap_any(bitmaps[index], start_idx, cnt_idx) ? "true" : "false");
        }

        //bitmap_contains bm0 0 2 true (true이면 bitmap_any, false이면 bitmap_all)
        else if (strcmp(command, "bitmap_contains") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %zu %zu %5s", name, &start_idx, &cnt_idx, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            printf("%s\n", bitmap_contains(bitmaps[index], start_idx, cnt_idx, value) ? "true" : "false");
        }
        //bitmap_count bm0 0 8 true	 (0번째부터 8개의 비트에서 true의 개수) 
        else if (strcmp(command, "bitmap_count") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %zu %zu %5s", name, &start_idx, &cnt_idx, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            printf("%ld\n", bitmap_count(bitmaps[index], start_idx, cnt_idx, value));
        }
        //bitmap_flip bm0 4		(4번째를 0이면 1, 1이면 0으로 바꿔라)
        else if (strcmp(command, "bitmap_flip") == 0) {
            char name[64];
            size_t bit_idx;
            scanf("%s %zu", name, &bit_idx);
            int index = get_bitmap(name);
            bitmap_flip(bitmaps[index], bit_idx);
        }
        //bitmap_none bm0 0 4  (0번째부터 4개의 비트 중에 1인 비트가 하나도 없는지)  
        else if (strcmp(command, "bitmap_none") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            scanf("%s %zu %zu", name, &start_idx, &cnt_idx);
            int index = get_bitmap(name);
            //bitmap_all(bitmaps[index], start_idx, cnt_idx);
            printf("%s\n", bitmap_none(bitmaps[index], start_idx, cnt_idx) ? "true" : "false");
        }
        //bitmap_reset bm0 5		(5번째가 0이면 그대로, 1이면 0으로 바꿔라)
        else if (strcmp(command, "bitmap_reset") == 0) {
            char name[64];
            size_t bit_idx;
            scanf("%s %zu", name, &bit_idx);
            int index = get_bitmap(name);
            bitmap_reset(bitmaps[index], bit_idx);
        }
        //bitmap_scan_and_flip bm0 0 2 true (1이 연속 2개인 곳을 0번째부터 스캔후, 반전)
        else if (strcmp(command, "bitmap_scan_and_flip") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %zu %zu %5s", name, &start_idx, &cnt_idx, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            size_t result = bitmap_scan_and_flip(bitmaps[index], start_idx, cnt_idx, value);
            printf("%zu\n", result);
        }
        //bitmap_scan bm0 1 3 true (1번째부터 1이 연속 3개인 곳을 스캔후 인덱스 반환)
        else if (strcmp(command, "bitmap_scan") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %zu %zu %5s", name, &start_idx, &cnt_idx, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            size_t result = bitmap_scan(bitmaps[index], start_idx, cnt_idx, value);
            printf("%zu\n", result);
        }
        //bitmap_set bm0 5 true (5번째를 1로 바꿔라)
        else if (strcmp(command, "bitmap_set") == 0) {
            char name[64];
            size_t idx;
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %zu %5s", name, &idx, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            bitmap_set(bitmaps[index], idx, value);
        }
        //bitmap_set_all bm0 true (true면 모두 1, false면 모두 0) 
        else if (strcmp(command, "bitmap_set_all") == 0) {
            char name[64];
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %5s", name, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            bitmap_set_all(bitmaps[index], value);
        }
        //bitmap_set_multiple bm0 0 4 true (0번쨰부터 4개의 bit를 1로 바꿔라)
        else if (strcmp(command, "bitmap_set_multiple") == 0) {
            char name[64];
            size_t start_idx, cnt_idx;
            char val_str[6];//true or false 들어오는 거
            bool value;
            scanf("%s %zu %zu %5s", name, &start_idx, &cnt_idx, val_str);
            int index = get_bitmap(name);
            if (strcmp(val_str, "true") == 0) {
                value = true;
            } else if (strcmp(val_str, "false") == 0) {
                value = false;
            }
            bitmap_set_multiple(bitmaps[index], start_idx, cnt_idx, value);
        }
        //bitmap_test bm0 4	(4번쨰 비트가 1이면 true, 0이면 false)
        else if (strcmp(command, "bitmap_test") == 0) {
            char name[64];
            size_t bit_idx;
            scanf("%s %zu", name, &bit_idx);
            int index = get_bitmap(name);
            printf("%s\n", bitmap_test(bitmaps[index], bit_idx) ? "true" : "false");
        }
        //bitmap_dump bm0 (16진수로 출력)
        else if (strcmp(command, "bitmap_dump") == 0) {
            char name[64];
            scanf("%s", name);
            int index = get_bitmap(name);
            bitmap_dump(bitmaps[index]);
        }
        //bitmap_expand bm0 2		(bm0 비트맵을 0을 더 붙여서 2칸 더 늘려라)
        else if (strcmp(command, "bitmap_expand") == 0) {
            char name[64];
            int extra_bits;
            scanf("%s %d", name, &extra_bits);
            int index = get_bitmap(name);
            struct bitmap *expanded_bitmap = bitmap_expand(bitmaps[index], extra_bits);
            bitmaps[index] = expanded_bitmap;
        }
        ///////////////////////////////////////////////    
        else if (strcmp(command, "delete") == 0) {
            char type[64];
            scanf("%s", type);
            ////////////list delete////////////
            if (strcmp(type, "list0") == 0) {
                delete_list(&list0);
            } else if (strcmp(type, "list1") == 0) {
                delete_list(&list1);
            }
            ////////hash delete////////////////
            else if (strcmp(type, "hash0") == 0) {
                hash_destroy(&hash0, NULL);
            } else if (strcmp(type, "hash1") == 0) {
                hash_destroy(&hash1, NULL);
            }        
            } else if (strcmp(command, "quit") == 0) {
            break;
        } 
    }
    return 0;
}
