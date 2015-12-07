/*
 *     This module implements functions that performs various operations at a single CPU heap level
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "memory_allocator.h"
#include "size_class.h"

/* remove a superblock from a given heap and sizeclass index and update heap level stats */
void removeSuperblockFromHeap(cpuheap_t *heap, int sizeClass_ix, superblock_t *pSb){
    /* Assuming the heap is locked */
    /* TODO: when size class lock is available, it should be locked here or on the calling side */
    size_class_t *size_class = &(heap->_sizeClasses[sizeClass_ix]);
    size_t superblock_bytes_used = getBytesUsed(pSb);

    assert(pSb->_meta._pOwnerHeap == heap);
    assert(heap->_bytesAvailable >= SUPERBLOCK_SIZE);
    assert(heap->_bytesUsed >= superblock_bytes_used);

    removeSuperBlock(size_class, pSb);
    pSb->_meta._pOwnerHeap = NULL;

     /* TODO: Should this be replaced with something more accurate? */
    heap->_bytesAvailable -= SUPERBLOCK_SIZE;
    heap->_bytesUsed -= superblock_bytes_used;
}

/* add a superblock to a given heap and sizeclass index and update heap level stats */
void addSuperblockToHeap(cpuheap_t *heap, int sizeClass_ix, superblock_t *pSb){
    /* Assuming the heap is locked */
    /* TODO: when size class lock is available, it should be locked here or on the calling side */
    size_class_t *size_class = &(heap->_sizeClasses[sizeClass_ix]);

    insertSuperBlock(size_class, pSb);
    pSb->_meta._pOwnerHeap = heap;

     /* TODO: Should this be replaced with something more accurate? */
    heap->_bytesAvailable += SUPERBLOCK_SIZE;
    heap->_bytesUsed += getBytesUsed(pSb);
}

void *allocateBlockFromCurrentHeap(superblock_t *pSb) {
    block_header_t *block = NULL;
    cpuheap_t *heap = pSb->_meta._pOwnerHeap;
    size_t old_bytes_used = 0;
    size_t new_bytes_used = 0;

    assert(NULL != heap);

    old_bytes_used = getBytesUsed(pSb);
    block = popBlock(pSb);
    new_bytes_used = getBytesUsed(pSb);

    assert(heap->_bytesUsed >= old_bytes_used);
    heap->_bytesUsed -= old_bytes_used;
    heap->_bytesUsed += new_bytes_used;

    return ((void *) block) + sizeof(block_header_t);
}

void freeBlockFromCurrentHeap(block_header_t *pBlock) {
    superblock_t *superblock = pBlock->_pOwner;
    cpuheap_t *heap = NULL;
    size_t old_bytes_used = 0;
    size_t new_bytes_used = 0;

    assert(NULL != superblock);
    heap = superblock->_meta._pOwnerHeap;
    assert(NULL != heap);

    old_bytes_used = getBytesUsed(superblock);
    pushBlock(superblock, pBlock);
    new_bytes_used = getBytesUsed(superblock);

    assert(heap->_bytesUsed >= old_bytes_used);
    heap->_bytesUsed -= old_bytes_used;
    heap->_bytesUsed += new_bytes_used;
}

/* this is a boolean function to check the condition
 * to transfer superblocks to general heap
 */
bool isHeapUnderUtilized(cpuheap_t *pHeap) {
    assert(pHeap->_bytesAvailable >= (HOARD_K * SUPERBLOCK_SIZE));
    return ((double) pHeap->_bytesUsed < ((double) pHeap->_bytesAvailable) * (1 - HOARD_EMPTY_FRACTION)) &&
        (pHeap->_bytesUsed < pHeap->_bytesAvailable - (HOARD_K * SUPERBLOCK_SIZE));
}



superblock_t *findMostlyEmptySuperblock(cpuheap_t *pHeap){
    /* TODO: Do! */
    return NULL;
}
