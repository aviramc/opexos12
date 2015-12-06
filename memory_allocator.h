/*
 *
 *      function prototypes
 */


#ifndef _MEN_ALLOC_H
#define _MEM_ALLOC_H

#include <stdbool.h>

#include "mtmm.h"



size_t getBlockActualSizeInHeaders(size_t sizeClassBytes);
size_t getBlockActualSizeInBytes(size_t sizeClassBytes);

void *getCore(size_t size);
void freeCore(void *p, size_t length);

superblock_t* makeSuperblock(size_t sizeClassBytes);
block_header_t *popBlock(superblock_t *pSb);
superblock_t *pushBlock(superblock_t *pSb, block_header_t *pBlk);
unsigned short getFullness(superblock_t *pSb);
void printSuperblock(superblock_t *pSb);
size_t getBytesUsed(const superblock_t *pSb);
block_header_t *getBlockHeaderForPtr(void *ptr) ;


void removeSuperblockFromHeap(cpuheap_t *heap, int sizeClass_ix, superblock_t *pSb);
void addSuperblockToHeap(cpuheap_t *heap, int sizeClass_ix, superblock_t *pSb);
void *allocateBlockFromCurrentHeap( superblock_t *pSb);
void freeBlockFromCurrentHeap( block_header_t *pBlock);
bool isHeapUnderUtilized(cpuheap_t *pHeap);

superblock_t *findMostlyEmptySuperblock(cpuheap_t *pHeap);

superblock_t *findAvailableSuperblock(size_class_t *sizeClass);


void relocateSuperBlockBack(size_class_t *sizeClass, superblock_t *superBlock);

void printSizeClass(size_class_t *sizeClass);

size_t getSizeClassIndex(size_t size);
size_class_t *getSizeClassForSuperblock(superblock_t *pSb);
void *allocateFromSuperblock(superblock_t *pSb);





#endif /* MEORY_ALLOCATOR_H_ */
