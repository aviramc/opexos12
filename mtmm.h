/*
 * mtmm.h
 *
 */


#ifndef __MTMM__H__
#define __MTMM__H__
#include <stddef.h>
#include <pthread.h>


// The minimum allocation grain for a given object
#define SUPERBLOCK_SIZE 65536
#define NUMBER_OF_HEAPS 2
#define GEREAL_HEAP_IX 0
#define HOARD_K 0
#define HOARD_EMPTY_FRACTION 0.25
#define NUMBER_OF_SIZE_CLASSES 16

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
void * malloc (size_t sz);


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
void free (void * ptr) ;


/*

The realloc() function changes the size of the memory block pointed to by ptr to size bytes. 
The contents will be unchanged in the range from the start of the region up to the minimum 
of the old and new sizes. If the new size is larger than the old size, the added memory 
will not be initialized. If ptr is NULL, then the call is equivalent to malloc(size), 
for all values of size; if size is equal to zero, and ptr is not NULL, then the call 
is equivalent to free(ptr). Unless ptr is NULL, it must have been returned by an earlier 
call to malloc(), calloc() or realloc(). If the area pointed to was moved, a free(ptr) is done. 


1. allocate sz bytes
2. copy from old location to a new one
3. free old allocation
*/
void * realloc (void * ptr, size_t sz) ;


/*
 * allocates and zeros the allocated memory
 */
void *calloc(size_t nmemb, size_t size);









/************************************************************************************************************/
typedef struct block_header {
	struct block_header *_pNextBlk;
	struct superblock *_pOwner;
	size_t size;
} block_header_t;

typedef struct  {
	/*
	 * Number of blocks and number of free blocks
	 */
	unsigned int _NoBlks, _NoFreeBlks;

	/*
	 * size class in bytes
	 */
	size_t _sizeClassBytes;

	/* Doubly linked list pointers*/
	struct superblock *_pNxtSBlk, *_pPrvSblk;

	/*
	 * pointer to the owner heap
	 */
	struct cpuheap *_pOwnerHeap;

	/*
	 * LIFO stack of free blocks
	 */
	block_header_t *_pFreeBlkStack;

	/*
	 * lock to prevent race conditions while updating the metadata
	 */
	pthread_mutex_t _sbLock;

} sblk_metadata_t;

/*
 * superblock - allocated on heap
 */
typedef struct superblock {

	sblk_metadata_t _meta;
	/*
	 * actual allocated memory
	 */
	char _buff[SUPERBLOCK_SIZE];

} superblock_t;





/* *convenience structure for list head */
typedef struct {
    unsigned int _length;
    superblock_t  *_first;
} superblock_head_t;



/*
 * collection of superblocks of same size
 * allocated in data segment
 */
typedef struct  {

	/* the size class of objects in superblock list */
	size_t _sizeClassBytes;

	/* Doubly linked list of supoerblocks ordered from the fullest to emptiest
	 * Points to head of list
	 * The members of the list are allocated on the heap
	 * */

	superblock_head_t _SBlkList;

} size_class_t;



/* per CPU memory allocations struct
 * should be allocated in data segment
 */
typedef struct cpuheap{
	unsigned short _CpuId;

	/* u(i) and a(i) from hoard*/
	size_t _bytesUsed, _bytesAvailable;

	size_class_t _sizeClasses[NUMBER_OF_SIZE_CLASSES];

} cpuheap_t;



/* top level memory allocator struct
 * should be allocated in data segment
 */
typedef struct {
	cpuheap_t _heaps[NUMBER_OF_HEAPS+1];

} hoard_t;


#endif


