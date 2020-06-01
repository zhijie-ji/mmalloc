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
    int magic_num;
    struct header *next;
    struct super_header *owner;
} header_t;

typedef struct super_header {
    struct super_header *non_full_super;
    struct super_header *next_free_4k;
    header_t *head_free;
    int usage;
    int index;
    int size_class;
    int pad;
} __attribute__ ((packed, aligned(8))) super_header_t;

typedef struct class_head{
    int capacity;
    super_header_t *super_start;
} class_head_t;

class_head_t class_head[15];

int num_4k = 0;

super_header_t free_4k_start = {};

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    for(int i=0 ; i<=14; i++)
    {
        class_head[i].capacity = (4096*32 -sizeof(super_header_t))/(sizeof(header_t)+(1<<(i+4)));
/*        printf("size%i:%i\n",sizeof(header_t)+(1<<(i+4)),class_head[i].capacity );*/
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
    header -> next = super_header -> head_free;
    super_header -> head_free = header;
}

void pop_header(super_header_t *super_header)
{
/*    printf("pop_header\n");*/
/*    printf("head_free: %p\n",super_header->head_free);*/
    
    if ((super_header->head_free = super_header->head_free->next) == NULL){
        int size_class = super_header->size_class;
        pop_super_header(&class_head[size_class]);
        }
}

void insert_free_4k(super_header_t *free_4k)
{
    if (free_4k_start.next_free_4k == NULL || free_4k->index > free_4k_start.next_free_4k->index)
    {
        free_4k = free_4k_start.next_free_4k;
        free_4k_start.next_free_4k = free_4k;
    }
    else
    {
        super_header_t *current = free_4k_start.next_free_4k;
        while (current ->next_free_4k != NULL || free_4k->index < current->next_free_4k->index)
        {
            current = current ->next_free_4k;
        }
        free_4k->next_free_4k = current->next_free_4k;
        current ->next_free_4k = free_4k;
    }
}

void remove_free_4k(super_header_t *free_4k)
{
    if (free_4k_start.next_free_4k == free_4k)
    {
        free_4k_start.next_free_4k = NULL;
    }
    super_header_t *prev = &free_4k_start;
    super_header_t *next = free_4k_start.next_free_4k;    
    while(1)
    {
        if (next == free_4k)
        {
            prev->next_free_4k = next;
            free_4k->next_free_4k = NULL;
        }
    }
}

void free_4k()
{
/*    printf("free_4k\n");*/
    super_header_t *block = &free_4k_start;
    while(block && block->index == num_4k)
    {
        mem_brk -= sizeof(super_header_t)+4096*32;
        num_4k--;
        block = block->next_free_4k;
    }
}

super_header_t *find_last_4k()
{
    if (free_4k_start.next_free_4k !=NULL){
        super_header_t *current = free_4k_start.next_free_4k; 
        while(current ->next_free_4k != NULL)
        {
            current = current->next_free_4k;
        }
        return current;
    }
    else
    {
        return NULL;
    }
    
}



void init_4k(super_header_t *super_header,int size_class)
{
/*    printf("start initialize 4k\n class = %i\n",size_class);*/
/*    printf("capacity:%i\n",class_head[size_class].capacity);*/
    for (int i = 0; i < class_head[size_class].capacity; i++)
    {
        header_t *header = (header_t *)((void *)(super_header+1) + i*(sizeof(header_t)+(1<<(4+size_class))));
        header -> owner = super_header;
        push_header(super_header,header);
        header->magic_num = 0xbeef;
    }
}

void *alloc_4k(int size_class)
{
    super_header_t *super_header;
    if (free_4k_start.next_free_4k)
    {
/*        printf("*used \n");*/
        super_header = find_last_4k();
    }
    else
    {
/*        printf("allocate new 4k\n");*/
        void *p = mem_sbrk(4096 * 32);
        if (p == (void *)-1)
        {

            return NULL;
        }/*            printf("full\n");*/
        super_header = (super_header_t *)(p);
    }
    init_4k(super_header,size_class);
    push_super_header(&class_head[size_class],super_header);
    super_header->index = ++num_4k;
    super_header->size_class = size_class;
    return super_header;
    
}

 /* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size)
{
/*    printf("***malloc size:%i\n",size);*/
    if (size == 0)
        return NULL;
    int size_class = find_class(size);
/*    printf("sizeclass:%i\n",size_class);*/
    if (class_head[size_class].super_start != NULL) {
/*        printf("**Suse existed\n");*/
        header_t *block = class_head[size_class].super_start->head_free;
        pop_header(class_head[size_class].super_start);
        return (void *)(block +1);
        }
        
    else{
/*        printf("**use new\n");*/
        alloc_4k(size_class);
/*        printf("head_ free: %p\n",class_head[size_class].super_start->head_free);*/
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
    if (header->magic_num == 0xbeef)
    {
        super_header_t *super_header = header->owner;
        push_header(super_header,header);
        int size_class = super_header->size_class;
        --super_header->usage;
/*        printf("usage:%i",super_header->usage);*/
        
/*if the 4k block is totally free*/
        if(super_header->usage == 0)
        {
/*            printf("here");*/
            pop_super_header(&class_head[size_class]);
            insert_free_4k(super_header);
            free_4k();
        }
/*if the 4k block was full*/
        if (super_header->usage == class_head[size_class].capacity-1)
        {
            push_super_header(&class_head[size_class],super_header);
        }
    }
    
    else{
/*        printf("The input pointer is wrong!");*/
        exit(-1);
    }
}


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
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/*int main()*/
/*{*/
/*    mm_init();*/
/*    void *p = mm_malloc(2040);*/
/*}*/















