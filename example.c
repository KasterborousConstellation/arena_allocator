#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define KTBS_ALLOCATOR_IMPLEMENTATION
#include "ktbs_allocator.h"
#define N 1000
int main(){
    ktbs_allocator* allocator = ktbs_allocator_create(1 << 29,"Kappa");
    if(allocator == NULL){
        fprintf(stderr, "Failed to create allocator\n");
        return 1;
    }
    allocator_info(allocator);
    ktbs_block block = ktbs_allocator_alloc(allocator, sizeof(ktbs_block)*N);
    ktbs_block* block_array = (ktbs_block*) block.ptr;
    time_t start = time(NULL);
    for(int i=0; i<N; i++){
        block_array[i] = ktbs_allocator_alloc(allocator, 1 << 16);
    }
    for(int i=0; i<N/2; i++){
        free_block(block_array[i]);
    }
    for(int i=0;i<N/2; i++){
        block_array[i] = ktbs_allocator_alloc(allocator, 1 << 16);
    }
    for(int i=N/2; i<N; i++){
        free_block(block_array[i]);
    }
    time_t end = time(NULL);
    printf("Deallocation time: %ld seconds\n", end - start);
    ktbs_allocator_destroy(allocator);
    printf("Allocator destroyed\n");
    return 0;
}