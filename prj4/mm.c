/*
 * This allocator is based on segregated free lists with size classes,
 * which improves allocation and free performance by reducing search time.
 * Each free block is inserted into the appropriate list according to its size.
 *
 * The allocator supports coalescing of adjacent free blocks to minimize fragmentation.
 * Allocation uses a best-fit strategy within each list to improve utilization.
 *
 * The realloc function attempts to expand the block in place by merging with
 * the next block or the previous block if possible. If not, it falls back to allocating
 * a new block and copying the data.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "mm.h"
#include "memlib.h"

#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE  (1<<12)
#define OVERHEAD    8

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)  ((size) | (alloc))
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PREV_FREE(bp) (*(void **)(bp))
#define NEXT_FREE(bp) (*(void **)((char *)(bp) + WSIZE))
#define LIST_LIMIT 20
void **segregated_lists = NULL;

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20231632",
    /* Your full name*/
    "Alua Jumagul",
    /* Your email address */
    "alua@sogang.ac.kr",
};

static char *heap_listp = 0;

static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void insert_node(void *bp, size_t size);
static void remove_node(void *bp);

//return index in seg list based on block size
int get_list_index(size_t size) {
    int idx = 0;
    while ((idx < LIST_LIMIT - 1) && (size > 1)) {
        size >>= 1;
        idx++;
    }
    return idx;
}

/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void) {
    //포인터 공간 확보
    segregated_lists = (void **)mem_sbrk(sizeof(void *) * LIST_LIMIT);
    if (segregated_lists == (void *)-1)
        return -1;
    
    for (int i = 0; i < LIST_LIMIT; i++) {
        segregated_lists[i] = NULL;
    }

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                                
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));     
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));    
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));         
    heap_listp += 2 * WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    char *bp;

    if (heap_listp == 0) mm_init();

    if (size == 0) return NULL;
    //adjust block size to include overhead and aligment
    if (size <= DSIZE) 
        asize = 2 * DSIZE;
    else 
        asize = ALIGN(size + DSIZE);
    //try to find a fit
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    //no fit found, extend the heap
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    if (!bp)
        return;

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (ptr == NULL)
        return mm_malloc(size);

    size_t oldsize = GET_SIZE(HDRP(ptr));
    size_t newsize = ALIGN(size + DSIZE);

    if (newsize <= oldsize) {
        return ptr;
    }

    void *next = NEXT_BLKP(ptr);

    //try to expand into next free block
    if (GET_SIZE(HDRP(next)) != 0 && !GET_ALLOC(HDRP(next))) {
        size_t next_size = GET_SIZE(HDRP(next));
        size_t combined_size = oldsize + next_size;

        if (combined_size >= newsize) {
            remove_node(next); 
            PUT(HDRP(ptr), PACK(combined_size, 1));
            PUT(FTRP(ptr), PACK(combined_size, 1));
            return ptr;
        }
    }
    //allocate new block and copy
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    memcpy(newptr, ptr, oldsize - DSIZE);
    mm_free(ptr);
    return newptr;
}

//extend heap with a new free block
static void *extend_heap(size_t words) {
    char *bp;
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size, 0));//free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));//new epilogue
    return coalesce(bp);
}

//insert a block into appropriate seg free list
static void insert_node(void *bp, size_t size) {
    int idx = get_list_index(size);
    void *head = segregated_lists[idx];
    NEXT_FREE(bp) = head;
    PREV_FREE(bp) = NULL;
    if (head != NULL) PREV_FREE(head) = bp;
    segregated_lists[idx] = bp;
}

//remove a block from seg free list
static void remove_node(void *bp) {
    int idx = get_list_index(GET_SIZE(HDRP(bp)));
    void *prev = PREV_FREE(bp);
    void *next = NEXT_FREE(bp);
    if (prev != NULL) NEXT_FREE(prev) = next;
    else segregated_lists[idx] = next;
    if (next != NULL) PREV_FREE(next) = prev;
}

//coalesce adjacent free blocks
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (!prev_alloc) remove_node(PREV_BLKP(bp));
    if (!next_alloc) remove_node(NEXT_BLKP(bp));

    if (!prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
    } else if (!prev_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
    } else if (!next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_node(bp, size);
    return bp;
}

//place allocated block, splitting if needed
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    remove_node(bp);

    if ((csize - asize) >= (2 * DSIZE)) {
        //split block
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *next = NEXT_BLKP(bp);
        PUT(HDRP(next), PACK(csize - asize, 0));
        PUT(FTRP(next), PACK(csize - asize, 0));
        insert_node(next, csize - asize);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

//find best-fit free block in seg list
static void *find_fit(size_t asize) {
    int idx = get_list_index(asize);
    for (int i = idx; i < LIST_LIMIT; i++) {
        void *bp = segregated_lists[i];
        void *best_fit = NULL;
        size_t min_diff = (size_t)-1;
        while (bp != NULL) {
            size_t bsize = GET_SIZE(HDRP(bp));
            if (bsize >= asize && (bsize - asize) < min_diff) {
                best_fit = bp;
                min_diff = bsize - asize;
                if (min_diff == 0) break;
            }
            bp = NEXT_FREE(bp);
        }
        if (best_fit) return best_fit;
    }
    return NULL;
}
