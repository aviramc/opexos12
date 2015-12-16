/*
 * malloc_allocator.c
 *
 *      This is the central module of memory allocator implementing malloc, free and a few helper functions
 *
 */

#include "memory_allocator.h"
#include "assert_static.h"

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static hoard_t memory;
static pthread_mutex_t heapLocks[NUMBER_OF_HEAPS + 1];
static char isMutexInit;

/* Functions that wrap the pthread lock functions with asserts
   for return code verification. With verify with assert because
   we cannot handle such an error otherwise.
 */
static void _lock_mutex(pthread_mutex_t *mutex);
static void _unlock_mutex(pthread_mutex_t *mutex);


/*
 * calculate hashed heap ID - returns either 1 or 2
 */
int getHeapID() {
	int heapid;
	pthread_t self;
	self = pthread_self();
	heapid = (self % 7) % 2;
	heapid++; /* 0 is reserved for general heap so we add 1 */
	return heapid;

}

/*
 * initalize the heap level mutexes
 */
void initMutexes() {
	int i;
	for (i = 0; i < NUMBER_OF_HEAPS + 1; i++)
		if (pthread_mutex_init(&heapLocks[i], NULL) != 0) {
			printf("\n mutex init failed\n");
			exit(-1);
		}

	isMutexInit = 1; /* so that we won't init again */

}

/*

 The malloc() function allocates size bytes and returns a pointer to the allocated memory.
 The memory is not initialized. If size is 0, then malloc() returns either NULL, or a unique
 pointer value that can later be successfully passed to free().


 malloc (sz)
 1. If sz > S/2, allocate the superblock from the OS and return it.
 2. i ← hash(the current thread).
 3. Lock heap i.
 4. Scan heap i’s list of superblocks from most full to least (for the size class corresponding to sz).
 5. If there is no superblock with free space,
 6. 	Check heap 0 (the global heap) for a superblock.
 7. 	If there is none,
 8. 		Allocate S bytes as superblock s and set the owner to heap i.
 9. Else
 10. 	Transfer the superblock s to heap i.
 11. 	u 0 ← u 0 − s.u
 12. 	u i ← u i + s.u
 13. 	a 0 ← a 0 − S
 14.    a i ← a i + S
 15. u i ← u i + sz.
 16. s.u ← s.u + sz.
 17. Unlock heap i.
 18. Return a block from the superblock.
 */

void * malloc(size_t sz) {

	int heapIndex, sizeClassIndex;
	superblock_t *pSb;
	void *p;


	/* printf("TODO: remove this debugging output\n"); */

	/* #1 */
	if (sz > SUPERBLOCK_SIZE / 2) {
		/* in order to identify that this block is large when we free it,
		 * we add a header with the size
		 */

		/* allocate memory to satisfy the large request and overheads*/
		block_header_t *p = getCore(sz + sizeof(block_header_t));
		if (!p){
			/* memory allocation failed*/
			return NULL;
		}
		/* the block header goes first, so p++*/
		p->size = sz;
		p++;
		return (void*) p;
	}

	if (!isMutexInit)
		initMutexes();

	/* #2 */
	heapIndex = getHeapID();



	/* #3 */
	_lock_mutex(&heapLocks[heapIndex]);



	/* #4 */
	sizeClassIndex = getSizeClassIndex(sz);

	/* look in heap i to see if a superblock of relevant size class is found in a private heap*/
	pSb = findAvailableSuperblock(
			&(memory._heaps[heapIndex]._sizeClasses[sizeClassIndex]));


	/* #5 && #6 */
	if (!pSb
			&& (pSb =
					findAvailableSuperblock(/* search in general heap */
							&(memory._heaps[GEREAL_HEAP_IX]._sizeClasses[sizeClassIndex])))) {

		/* superblock of relevant size class was found in general heap
		 * relocate it to private heap step #10
		 */

		/* #11 #13 */
        _lock_mutex(&(pSb->_meta._sbLock));
		removeSuperblockFromHeap(&(memory._heaps[GEREAL_HEAP_IX]),
				sizeClassIndex, pSb);

		/* #12 #14 */
		addSuperblockToHeap(&(memory._heaps[heapIndex]), sizeClassIndex, pSb);
		_unlock_mutex(&(pSb->_meta._sbLock));

	}

	/* #7 */
	if (!pSb) {
		/* superblock of relevant size not found anywhere
		 * generate it
		 */
		pSb = makeSuperblock(pow(2.0, sizeClassIndex));

		/*#8*/
		addSuperblockToHeap(&(memory._heaps[heapIndex]), sizeClassIndex, pSb);


	}

	/* #15, #16 */
	/* this is redundant, but there is no init for heap */
	if (memory._heaps[heapIndex]._CpuId != heapIndex)
		memory._heaps[heapIndex]._CpuId = heapIndex;

	p = allocateBlockFromCurrentHeap(pSb);

	_unlock_mutex(&heapLocks[heapIndex]);

	return p;

}

/*

 The free() function frees the memory space pointed to by ptr, which must have been returned
 by a previous call to malloc(), calloc() or realloc(). Otherwise, or if free(ptr) has already
 been called before, undefined behavior occurs. If ptr is NULL, no operation is performed.


 free (ptr)
 1. If the block is “large”,
 2. 	Free the superblock to the operating system and return.
 3. Find the superblock s this block comes from and lock it.
 4. Lock heap i, the superblock’s owner.
 5. Deallocate the block from the superblock.
 6. u i ← u i − block size.
 7. s.u ← s.u − block size.
 8. If i = 0, unlock heap i and the superblock and return.
 9. If u i < a i − K ∗ S and u i < (1 − f) ∗ a i,
 10. 	Transfer a mostly-empty superblock s1 to heap 0 (the global heap).
 11. 	u 0 ← u 0 + s1.u, u i ← u i − s1.u
 12. 	a 0 ← a 0 + S, a i ← a i − S
 13. Unlock heap i and the superblock.
 */
void free(void *ptr) {

	cpuheap_t *pHeap;
	block_header_t *pBlock;


	if (!ptr){
		return;
	}

	pBlock= getBlockHeaderForPtr(ptr);



	/* #1 */
	if (pBlock->size > SUPERBLOCK_SIZE / 2) {
		freeCore((void*) pBlock, (pBlock->size + sizeof(block_header_t)));
		return;
	}

	superblock_t *pSb = pBlock->_pOwner;
	_lock_mutex(&(pSb->_meta._sbLock));

	/* #3 */
	pHeap = pSb->_meta._pOwnerHeap;


	/* #4 */

	_unlock_mutex(&(pSb->_meta._sbLock));
	_lock_mutex(&heapLocks[pHeap->_CpuId]);

	if (pHeap!= pSb->_meta._pOwnerHeap){
		/* we've locked the wrong heap - the superblock has moved
		 * unlock and relock the uptodate heap*/
		_unlock_mutex(&heapLocks[pHeap->_CpuId]);
		_lock_mutex(&(pSb->_meta._sbLock));
		pHeap=pSb->_meta._pOwnerHeap;
		_unlock_mutex(&(pSb->_meta._sbLock));
		_lock_mutex(&heapLocks[pHeap->_CpuId]);

	}





	/* #5, #6, #7 */
	freeBlockFromCurrentHeap(pBlock);


	/* #8 */
	if (pHeap->_CpuId == GEREAL_HEAP_IX) {
		_unlock_mutex(&heapLocks[pHeap->_CpuId]);
		return;
	}
	/* #9 */

	if (isHeapUnderUtilized(pHeap)) {
		superblock_t *pSbToRelocate = findMostlyEmptySuperblock(pHeap);


		/* #10 */
		if (pSbToRelocate ) {
			size_t sizeClassIndex = getSizeClassIndex(
					pSbToRelocate->_meta._sizeClassBytes);


			/* #11 #12 */
			_lock_mutex(&(pSbToRelocate->_meta._sbLock));
			removeSuperblockFromHeap(pHeap, sizeClassIndex, pSbToRelocate);

			/* #11 #12 */

			addSuperblockToHeap(&(memory._heaps[GEREAL_HEAP_IX]),
					sizeClassIndex, pSbToRelocate);
			_unlock_mutex(&(pSbToRelocate->_meta._sbLock));
			memory._heaps[GEREAL_HEAP_IX]._CpuId=0;


		}
	}

	/* #13 */
	_unlock_mutex(&heapLocks[pHeap->_CpuId]);
	return;

}


/*t
 1. allocate sz bytes
 2. copy from old location to a new one
 3. free old allocation
 */
void *realloc(void *ptr, size_t sz) {
	void *p = malloc(sz);
	if (!p) {
		perror("realloc failed\n");
		return NULL;
	}
	if (!ptr)
		return p;

	if (!sz){
		free(ptr);
		return NULL;
	}
	block_header_t *pHeader = ptr - sizeof(block_header_t);
	unsigned int size = pHeader->size < sz ? pHeader->size : sz;

	memcpy(p, ptr, size);
	free(ptr);
	return p;
}


void *calloc(size_t nmemb, size_t size) {
	void *p = malloc(nmemb * size);
	if (p)
		memset(p,0,size);
	return p;
}


/*********************************************************************************************************/

superblock_t* makeSuperblock(size_t sizeClassBytes) {

    block_header_t *p, *pPrev = NULL;
    size_t netSuperblockSize = SUPERBLOCK_SIZE;
    size_t normSuperblockSize = netSuperblockSize / sizeof(block_header_t);

    /* the offset between subsequent blocks in units of block_header_t */
    size_t blockOffset = getBlockActualSizeInHeaders(sizeClassBytes);

    /* the number of blocks that we'll generate in this superblock */
    size_t numberOfBlocks = normSuperblockSize / blockOffset;
    int i;

    /* call system to allocate memory */
    superblock_t *pSb = (superblock_t*) getCore(SUPERBLOCK_SIZE + sizeof(sblk_metadata_t));

    if (NULL == pSb) {
        return NULL;
    }

    pSb->_meta._sizeClassBytes = sizeClassBytes;
    pSb->_meta._NoBlks = pSb->_meta._NoFreeBlks = numberOfBlocks;
    pSb->_meta._pNxtSBlk = pSb->_meta._pPrvSblk = NULL;

    /* initialize the working pointer to the address where allocated buffer begins */
    p = (block_header_t*) pSb->_buff;

    /* initialize the stack pointer to the first element in the list
     * it will later be regarded as "top of the stack"
     */
    pSb->_meta._pFreeBlkStack = p;
    p->_pOwner = pSb;
    p->size = sizeClassBytes;

    /* create the initial free blocks stack inside the allocated memory buffer */
    for (i = 0; i < numberOfBlocks - 1; i++) {

        pPrev = p;
        p += blockOffset;
        pPrev->_pNextBlk = p;
        p->_pOwner = pSb;
        p->size = sizeClassBytes;
        p->_pNextBlk = NULL;
    }

    pthread_mutex_init(&(pSb->_meta._sbLock),NULL);

    return pSb;
}

/**
 * pop a block from the top of the stack.
 * caller must call the relocateSuperBlockBack on the owning heap to update the superblock's position
 */
block_header_t *popBlock(superblock_t *pSb) {

	if (!pSb->_meta._NoFreeBlks)
		return NULL; /* no free blocks */

	/* get tail block */
	block_header_t *pTail = pSb->_meta._pFreeBlkStack;

	/* advance superblock tail */
	pSb->_meta._pFreeBlkStack = pSb->_meta._pFreeBlkStack->_pNextBlk;
	pSb->_meta._NoFreeBlks--;

	/* disconnect from stack - but leave the owner for when the user wants to free it*/
	pTail->_pNextBlk = NULL;

	return pTail;

}

void *allocateFromSuperblock(superblock_t *pSb) {
	block_header_t *block = popBlock(pSb);

	// TODO: fill here - relocate super block if needed

	return (void*) (block + 1);

}
/**
 * push a block to the top of the stack.
 * caller must call the relocateSuperBlockAhead on the owning heap to update the superblock's position
 */
superblock_t *pushBlock(superblock_t *pSb, block_header_t *pBlk) {

	if (pSb->_meta._NoFreeBlks == pSb->_meta._NoBlks)
		return NULL; /* stack full */

	/* new block's next is current tail */
	pBlk->_pNextBlk = pSb->_meta._pFreeBlkStack;

	/* stack tail points to new block */
	pSb->_meta._pFreeBlkStack = pBlk;

	pSb->_meta._NoFreeBlks++;

	return pSb;

}

/* reruns the percentage (0-100) of used blocks out of total blocks */
unsigned short getFullness(superblock_t *pSb) {

	double freeness = ((double) (pSb->_meta._NoFreeBlks))
			/ ((double) (pSb->_meta._NoBlks));

	double fullness = 1 - freeness;

	return ((unsigned short) (fullness * 100));

}

/**
 * utility function for printing a superblock
 */
void printSuperblock(superblock_t *pSb) {
	unsigned int i;
	block_header_t *p = pSb->_meta._pFreeBlkStack;
	printf("  Superblock: [%p] blocks: [%u] free [%u] used bytes [%u]\n", pSb,
			pSb->_meta._NoBlks, pSb->_meta._NoFreeBlks, getBytesUsed(pSb));
	printf("	[%p]<----prev    next---->[%p]\n", pSb->_meta._pPrvSblk,
			pSb->_meta._pNxtSBlk);
	printf("	====================================\n");

	for (i = 0; i < pSb->_meta._NoFreeBlks; i++, p = p->_pNextBlk) {
		printf("		free block %u) [%p]\n", i, p);
	}

}

/* returns the size in bytes of blocks used in superblock */
size_t getBytesUsed(const superblock_t *pSb) {
	size_t usedBlocks = pSb->_meta._NoBlks - pSb->_meta._NoFreeBlks;
	return usedBlocks * ( getBlockActualSizeInBytes(pSb->_meta._sizeClassBytes));
}

block_header_t *getBlockHeaderForPtr(void *ptr) {
	block_header_t *pBlock = (block_header_t *)( ptr - sizeof(block_header_t));
	return pBlock;
}

superblock_t *getSuperblockForPtr(void *ptr) {
	block_header_t *pHeader = getBlockHeaderForPtr(ptr);
	return pHeader->_pOwner;
}

/* for use with large allocations that do not use Hoard */
superblock_t* makeDummySuperblock(superblock_t *pSb, size_t sizeClassBytes) {
	pSb->_meta._sizeClassBytes = sizeClassBytes;
	return pSb;
}

void freeBlockFromSuperBlock(superblock_t *pSb, block_header_t *pBlock) {

	if (!pushBlock(pSb, pBlock))
		printf("Error freeing memory!\n");

}
/* the offset between subsequent blocks in units of block_header_t */
size_t getBlockActualSizeInHeaders(size_t sizeClassBytes){
	return (sizeClassBytes + 2 * sizeof(block_header_t) - 1)
			/ sizeof(block_header_t);

}

size_t getBlockActualSizeInBytes(size_t sizeClassBytes){
	return getBlockActualSizeInHeaders(sizeClassBytes)*sizeof(block_header_t);
}

static void _lock_mutex(pthread_mutex_t *mutex)
{
    assert(pthread_mutex_lock(mutex) == 0);
}

static void _unlock_mutex(pthread_mutex_t *mutex)
{
    assert(pthread_mutex_unlock(mutex) == 0);
}
