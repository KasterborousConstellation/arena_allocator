#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KTBS_ALLOCATOR_IMPLEMENTATION
#include "ktbs_allocator.h"
int main(){
    ktbs_allocator* allocator = ktbs_allocator_create(1 << 18,"Kappa");
    if(allocator == NULL){
        fprintf(stderr, "Failed to create allocator\n");
        return 1;
    }
    printf("Allocator created with size: %zu\n", size_of_allocator(allocator));
    ktbs_block block2 = ktbs_allocator_alloc(allocator, 130000);
    show_occupancy(allocator->occupancy);
    ktbs_block block3 = ktbs_allocator_alloc(allocator, 130000);
    show_occupancy(allocator->occupancy);
    ktbs_block block4 = ktbs_allocator_alloc(allocator, 130000);
    show_occupancy(allocator->occupancy);
    ktbs_block block5 = ktbs_allocator_alloc(allocator, 263000);
    show_occupancy(allocator->occupancy);
    ktbs_block block6 = ktbs_allocator_alloc(allocator, 130000);
    show_occupancy(allocator->occupancy);
    block_info(block2);
    block_info(block3);
    block_info(block4);
    block_info(block5);
    block_info(block6);
    free_block(block3);
    printf("Freed block 3\n");
    
    
    ktbs_allocator_destroy(allocator);
    printf("Allocator destroyed\n");
    return 0;
}