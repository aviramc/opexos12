/*
 *     This module implements functions that performs various operations at a single CPU heap level
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "memory_allocator.h"
#include "size_class.h"

static size_class_t * _get_superblock_size_class(cpuheap_t *heap, superblock_t *superblock);

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
    size_class_t *size_class = NULL;
    size_t old_bytes_used = 0;
    size_t new_bytes_used = 0;

    assert(NULL != heap);

    old_bytes_used = getBytesUsed(pSb);
    block = popBlock(pSb);
    new_bytes_used = getBytesUsed(pSb);

    assert(heap->_bytesUsed >= old_bytes_used);
    heap->_bytesUsed -= old_bytes_used;
    heap->_bytesUsed += new_bytes_used;

    if (block != NULL) {
        /* TODO: A correct reordering scheme should be used within the size class module */
        size_class = _get_superblock_size_class(heap, pSb);
        removeSuperBlock(size_class, pSb);
        insertSuperBlock(size_class, pSb);
        return ((void *) block) + sizeof(block_header_t);
    }

    return NULL;
}

void freeBlockFromCurrentHeap(block_header_t *pBlock) {
    superblock_t *superblock = pBlock->_pOwner;
    cpuheap_t *heap = NULL;
    size_t old_bytes_used = 0;
    size_t new_bytes_used = 0;
    size_class_t *size_class = NULL;

    assert(NULL != superblock);
    heap = superblock->_meta._pOwnerHeap;
    assert(NULL != heap);

    old_bytes_used = getBytesUsed(superblock);
    pushBlock(superblock, pBlock);
    new_bytes_used = getBytesUsed(superblock);

    assert(heap->_bytesUsed >= old_bytes_used);
    heap->_bytesUsed -= old_bytes_used;
    heap->_bytesUsed += new_bytes_used;

    /* TODO: A correct reordering scheme should be used within the size class module */
    size_class = _get_superblock_size_class(heap, superblock);
    removeSuperBlock(size_class, superblock);
    insertSuperBlock(size_class, superblock);
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
    /* Assuming the heap is locked */
    /* TODO: If size class locks are used, probably all of the size classes' locks
             should be locked prior to calling to this function (otherwise
             race conditions may occur)
     */
    return NULL;
    /* size_class_t *current_size_class = NULL; */
    /* superblock_t *current_superblock = NULL; */
    /* superblock_t *min_superblock = NULL; */
    /* unsigned int i = 0; */
    /* double min_fullness = 2.0; /\* More than 1 so that in the worst case a full superblock can be returned *\/ */
    /* double current_fullnesss = 0; */

    /* for (i = 0; i < NUMBER_OF_SIZE_CLASSES; i++) { */
    /*     current_size_class = &(pHeap->_sizeClasses[i]); */
    /*     current_superblock = findMostlyEmptySuperblockSizeClass(current_size_class); */

    /*     if (NULL != current_superblock) { */
    /*         current_fullnesss = getFullness(current_superblock); */
    /*         if (min_fullness > current_fullnesss) { */
    /*             min_superblock = current_superblock; */
    /*             min_fullness = current_fullnesss; */
    /*         } */
    /*     } */
    /* } */

    /* return min_superblock; */
}

static size_class_t * _get_superblock_size_class(cpuheap_t *heap, superblock_t *superblock)
{
    size_t size_class_index = NUMBER_OF_SIZE_CLASSES;

    assert(heap != NULL);
    assert(superblock != NULL);
    assert(heap == superblock->_meta._pOwnerHeap);

    size_class_index = getSizeClassIndex(superblock->_meta._sizeClassBytes);

    return &(heap->_sizeClasses[size_class_index]);
}
