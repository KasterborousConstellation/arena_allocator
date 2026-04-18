#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#define KTBS_ALLOCATOR_BIT_BLOCK_SIZE 16
struct ktbs_occupancy{
    bool occupied;
    bool full;
    struct ktbs_occupancy* left;
    struct ktbs_occupancy* right;
    struct ktbs_occupancy* parent;
    size_t log_size;
    size_t offset;
};
typedef struct ktbs_occupancy ktbs_occupancy;

typedef struct ktbs_allocator{
    size_t log_size; // log2 of the total size of the memory
    void *memory;
    ktbs_occupancy* occupancy;
    char name[64]; 
} ktbs_allocator;
ktbs_allocator* ktbs_allocator_create(size_t log_size,const char* name);
void ktbs_allocator_destroy(ktbs_allocator* allocator);
struct ktbs_block{
    size_t size;
    void* ptr;
    ktbs_allocator* allocator;
};
typedef struct ktbs_block ktbs_block;
ktbs_block ktbs_allocator_alloc(ktbs_allocator* allocator, size_t size);
void free_block(ktbs_block block);
void block_info(ktbs_block block);
#ifdef KTBS_ALLOCATOR_IMPLEMENTATION
#define isleaf(occupancy) (occupancy->left == NULL && occupancy->right == NULL)
#define log_size_of(size) (max(leftbit(size) +1, KTBS_ALLOCATOR_BIT_BLOCK_SIZE))
void free_occupancy(ktbs_occupancy* occupancy){
    if(occupancy == NULL) return;
    free_occupancy(occupancy->left);
    free_occupancy(occupancy->right);
    free(occupancy);
}
int max(int a, int b){
    return a > b ? a : b;
}
void ktbs_allocator_destroy(ktbs_allocator* allocator){
    free(allocator->memory);
    free_occupancy(allocator->occupancy);   
    free(allocator);
}
void nameit(ktbs_allocator* allocator, const char* name){
    strncpy(allocator->name, name, 63);
    allocator->name[63] = '\0';
}
// TODO : find faster method
int leftbit(size_t n){
    int pos = 0;
    while(n !=0){
        n = n >>1;
        pos++;
    } 
    return pos-1;
}
void propagate_up(ktbs_occupancy* occupancy){
    if(occupancy == NULL) return;
    if(!isleaf(occupancy)){
        occupancy->occupied = occupancy->left->occupied || occupancy->right->occupied;
        occupancy->full = occupancy->left->full && occupancy->right->full;  
    }
    propagate_up(occupancy->parent);
}
ktbs_occupancy* create_occupancy(size_t log_size,size_t offset){
    if(log_size < KTBS_ALLOCATOR_BIT_BLOCK_SIZE){
        errno = EINVAL;
        char buffer[256];
        snprintf(buffer, 256, "log_size must be at least %d", KTBS_ALLOCATOR_BIT_BLOCK_SIZE);
        fprintf(stderr, "%s\n", buffer);
        return NULL;
    }
    ktbs_occupancy* occupancy =(ktbs_occupancy*) malloc(sizeof(ktbs_occupancy));
    occupancy->occupied = false;
    occupancy->full = false;
    occupancy->parent = NULL;
    if(log_size == KTBS_ALLOCATOR_BIT_BLOCK_SIZE){
        occupancy->left = NULL;
        occupancy->right = NULL;
        occupancy->log_size = log_size;
        occupancy->offset = offset;
        return occupancy;
    }else{
        
        occupancy->log_size = log_size;
        occupancy->left = create_occupancy(log_size -1, offset);
        occupancy->left->parent = occupancy;
        occupancy->right = create_occupancy(log_size -1, offset + (1 << (log_size -1)));
        occupancy->right->parent = occupancy;
        occupancy->offset = offset;
        return occupancy;
    }
}
ktbs_allocator* ktbs_allocator_create(size_t size, const char* name){
    int log_size = log_size_of(size);
    ktbs_allocator* allocator = malloc(sizeof(ktbs_allocator));
    void* memory = malloc(1 << log_size);
    allocator->memory = memory;
    allocator->log_size = log_size;
    nameit(allocator, name);
    allocator->occupancy = create_occupancy(log_size, 0);
    return allocator;
}
size_t size_of_allocator(ktbs_allocator* allocator){
    return 1 << allocator->log_size;
}
void propagate_down(ktbs_occupancy* occupancy, bool occupied){
    if(occupancy == NULL)return;
    occupancy->occupied = occupied;
    occupancy->full = occupied;
    propagate_down(occupancy->left, occupied);
    propagate_down(occupancy->right, occupied);
}
ktbs_block alloc_block_aux(ktbs_allocator* allocator, ktbs_occupancy* occupancy, size_t log_size){
    if(log_size ==occupancy->log_size && !(occupancy->occupied)){
        occupancy->occupied = true;
        propagate_down(occupancy, true);
        propagate_up(occupancy);
        return (ktbs_block){1 << occupancy->log_size, (void*)(allocator->memory+occupancy->offset),allocator};
    }
    if(log_size < occupancy->log_size){
        ktbs_block block_left = alloc_block_aux(allocator, occupancy->left, log_size);
        if(block_left.size !=0) return block_left;
        ktbs_block val = alloc_block_aux(allocator, occupancy->right, log_size);

        return val;
    }

    return  (ktbs_block){0,NULL,NULL};
}
ktbs_block ktbs_allocator_alloc(ktbs_allocator* allocator, size_t size){
    ktbs_block block = {0,NULL,NULL};
    if(size == 0 || size > size_of_allocator(allocator)){
        errno = EINVAL;
        fprintf(stderr, "Invalid size to allocate: %zu\n", size);
    }else{
        size_t log_size = log_size_of(size);
        block = alloc_block_aux(allocator, allocator->occupancy, log_size);
    }
    return block;
}
const char* reset_color = "\033[0m";
void show_occupancy_aux(ktbs_occupancy* occupancy, int depth){
     if(occupancy == NULL) return;
    for(int i=0; i<depth; i++) printf("  ");
    const char* color = occupancy->full ? "\033[31m" : "\033[32m";
    
    printf("-%sOffset: %zu, Log Size: %zu, Full: %s, Occupied: %s%s\n",color, occupancy->offset, occupancy->log_size, occupancy->full ? "Yes" : "No", occupancy->occupied ? "Yes" : "No", reset_color);
    show_occupancy_aux(occupancy->left, depth +1);
    show_occupancy_aux(occupancy->right, depth +1);
}
void show_occupancy(ktbs_occupancy* occupancy){
    printf("Occupancy Tree:\n");
    show_occupancy_aux(occupancy, 0);
}
ktbs_occupancy* find_block(ktbs_occupancy* occupancy, size_t offset,size_t log_size){
    if(occupancy == NULL) return NULL;
    if(occupancy->log_size == log_size){
        if(occupancy->offset == offset) return occupancy;
        else return NULL;
    }else{
        size_t mid = occupancy->offset + (1 << (occupancy->log_size -1));
        if(offset < mid){
            return find_block(occupancy->left, offset, log_size);
        }else{
            return find_block(occupancy->right, offset, log_size);
        }
    }
}

void free_block(ktbs_block block){
    if(block.ptr == NULL || block.size == 0){
        errno = EINVAL;
        fprintf(stderr, "Invalid block to free\n");
        return;
    }
    ktbs_allocator* allocator = block.allocator;
    //dichotomy to find the block in the occupancy tree
    size_t offset = (size_t)(block.ptr - allocator->memory);
    ktbs_occupancy* occupancy = find_block(allocator->occupancy, offset,log_size_of(block.size-1));
    if(occupancy == NULL){
        errno = EINVAL;
        fprintf(stderr, "Block doesn't exists or already freed\n");
        return;
    }
    propagate_down(occupancy, false);
    propagate_up(occupancy->parent);
}
#define WVALID_BLOCK(block) (block.ptr == NULL || block.size == 0 || block.allocator == NULL)
void block_info(ktbs_block block){
    if(WVALID_BLOCK(block)){
        printf("Block Info \n \033[31mInvalid Block\033[0m\n");
        return;
    }
    printf("Block Info \n  Size: %zu\n  Ptr: %p\n  Allocator Name: %s\n", block.size, block.ptr, block.allocator->name);
}
#endif // KTBS_ALLOCATOR_IMPLEMENTATION