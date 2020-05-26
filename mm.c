/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


typedef struct header{
    int magic_num =0xbeef;
    struct header *next;
    struct super_header *owner;
} header_t;

typedef struct super_header {
    int usage;
    struct super_header *non_full_super;
    header_t *head_free;
    int index; 
} super_header_t;

typedef struct class_head{
    int capacity;
    super_header_t *super_start;
} class_head_t;

class_head_t class_head[7];

int num_4k = 0;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    for(int i=0 ; i<=6; i++)
    {
        class_head[i].capacity = (4096-sizeof(super_header_t))/(sizeof(header_t)+(1<<(i+4)));
    }
    return 0;
}

 int find_class(size_t size)
{
    int size_class = __builtin_clzl(size) + __builtin_ctzl(size)+1 == sizeof(size)*8 ? __builtin_ctzl(size) :  sizeof(size)*8 - __builtin_clzl(size);
    if (size_class < 4)
        return 0;
    return size_class-4;
}

void push_super_header(class_head_t *class_head,super_header_t *super_header)
{
    super_header->non_full_super = class_head->super_start;
    class_head->super_start = super_header;
}

void pop_super_header(class_head_t *class_head)
{
    class_head->super_start = class_head->super_start->non_full_super;
}

void push_header(super_header_t *super_header,header_t *header)
{
    header->next = super_header -> head_free;
    super_header -> head_free = header;
}

void pop_header(super_header_t *super_header)
{
    super_header->head_free = super_header->head_free->next;
}

void init_4k(super_header_t *super_header,int size_class)
{
    for (int i = 0; i < class_head[size_class].capacity; i++)
    {
        header_t *header = (header_t *)((void *)(super_header+1) + i*(sizeof(header_t)+(1<<(4+size_class))));
        push_header(super_header,header);
    }
}

void *alloc_4k(int size_class)
{
    super_header_t *super_header;
    void *p = (int *) sbrk(4096);
    if (p == (void *)-1)
        return NULL;
    else{
        super_header = p;
        init_4k(super_header,size_class);
        push_super_header(&class_head[size_class],super_header);
        super_header->index = ++num_4k;
        return super_header;
    }
}

 /* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;
    int size_class = find_class(size);
    if (class_head[size_class].super_start && class_head[size].super_start->usage < class_head[size].capacity) {
        header_t *block = class_head[size_class].super_start->head_free;
        pop_header(class_head[size_class].super_start);
        if (--class_head[size_class].super_start->usage ==0){
            pop_super_header(&class_head[size_class]);
        }
        return (void *)(block +1);
    }
    else{
        alloc_4k(size_class);
        header_t *block = class_head[size_class].super_start->head_free;
        pop_header(class_head[size_class].super_start);
        return (void *)(block +1);
    }
}


/*
 * mm_free - Freeing a block does nothing.
 */
 
void mm_free(void *ptr)
{
    header_t *header = ptr - sizeof(header_t);
    if (header->magic_num == 0xbeef){
        push_header(header->owner,header);
        if (++header->owner->usage == header->owner->)
        
    }
    else{
        printf("The input pointer is wrong!");
        exit(-1);
    }
}

/*int check_is_last_free(void)*/
/*{*/
/*    while (head)*/
/*    {*/
/*    */
/*    }*/
/*}*/

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
/*void *mm_realloc(void *ptr, size_t size)*/
/*{*/
/*    void *oldptr = ptr;*/
/*    void *newptr;*/
/*    size_t copySize;*/
/*    */
/*    newptr = mm_malloc(size);*/
/*    if (newptr == NULL)*/
/*      return NULL;*/
/*    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);*/
/*    if (size < copySize)*/
/*      copySize = size;*/
/*    memcpy(newptr, oldptr, copySize);*/
/*    mm_free(oldptr);*/
/*    return newptr;*/
/*}*/

int main()
{
    mm_init();
    int *p1 = mm_malloc(sizeof(int));
    printf("p1:%p",p1);
}













