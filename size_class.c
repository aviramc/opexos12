/*
 *
 *      This module implements functions to perform various operations at size class level
 *      such as insertions, removal and searches for superblocks from a given sizeclass
 *
 */

/* TODO: Re-ordering of superblocks when poping/push blocks! */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include "memory_allocator.h"
#include "assert_static.h"

/* return true if a superblock is the only one in the list */
static bool is_single(size_class_t *size_class, superblock_t *superblock);
/* init a superblock list with its first superblock for a size class */
static void init_superblock_list(size_class_t *sizeClass, superblock_t *first);
/* find the first block that is less full than fullness
   if there's no such block, this will return the head of the list
 */
static superblock_t *find_least_full_than(size_class_t *sizeClass, double fullness);
/* place a new superblock before another superblock in the list */
static void place_superblock(superblock_t *new, superblock_t *place_before);

/*
 * remove a superblock from the list
 * assuming the superblock belongs to the sizeclass
 */
void removeSuperBlock(size_class_t *sizeClass, superblock_t *superBlock) {
    superblock_t *previous = NULL;
    superblock_t *next = NULL;

    if (is_single(sizeClass, superBlock)) {
        sizeClass->_SBlkList._first = NULL;
        sizeClass->_SBlkList._length = 0;
    } else {
        assert(sizeClass->_SBlkList._length > 1);
        sizeClass->_SBlkList._length--;
        previous = superBlock->_meta._pPrvSblk;
        next = superBlock->_meta._pNxtSBlk;

        previous->_meta._pNxtSBlk = next;
        next->_meta._pPrvSblk = previous;

        superBlock->_meta._pPrvSblk = NULL;
        superBlock->_meta._pNxtSBlk = NULL;
    }
}

/*
 * insert a new superblock into the list
 *
 */
void insertSuperBlock(size_class_t *sizeClass, superblock_t *superBlock) {
    superblock_t * place_before = NULL;

    if (sizeClass->_SBlkList._first == NULL) {
        assert(sizeClass->_SBlkList._length == 0);
        init_superblock_list(sizeClass, superBlock);
        return;
    }

    place_before = find_least_full_than(sizeClass, getFullness(superBlock));
    assert(place_before != NULL);
    place_superblock(superBlock, place_before);
    sizeClass->_SBlkList._length++;
}

/* find available superblock */

superblock_t *findAvailableSuperblock(size_class_t *sizeClass) {
    /* we assume here that the size class is locked */
    superblock_t *superblock = NULL;
    unsigned int i = 0;

    for (i = 0, superblock = sizeClass->_SBlkList._first;
         i < sizeClass->_SBlkList._length;
         i++, superblock = superblock->_meta._pNxtSBlk) {

        if (superblock->_meta._NoFreeBlks > 0) {
            return superblock;
        }

    }

    return NULL;
}

/* find the mostly empty superblock in the size class */
superblock_t * findMostlyEmptySuperblockSizeClass(size_class_t *sizeClass)
{
    /* assuming heap/size class is locked */

    /* If there are no superblocks, return nothing. */
    if (sizeClass->_SBlkList._first == NULL) {
        return NULL;
    }

    /* Otherwise, since all superblocks are ordered, the least used should
       be the last one. The list is circular, so reaching the last one
       is easy :) */
    return sizeClass->_SBlkList._first->_meta._pPrvSblk;
}

void *allocateBlockFromSizeClass(size_class_t *sizeClass, superblock_t *superBlock)
{
    block_header_t *block = NULL;

    block = popBlock(superBlock);

    if (block != NULL) {
        /* Found this to be actually much more efficient than a function that
           moves the superblock further down the list
         */
        removeSuperBlock(sizeClass, superBlock);
        insertSuperBlock(sizeClass, superBlock);
        return block;
    }

    return NULL;
}

void freeBlockFromCurrentSizeClass(size_class_t *sizeClass, superblock_t *superBlock, block_header_t *block)
{
    assert(block->_pOwner == superBlock);
    pushBlock(superBlock, block);
    /* Found this to be actually much more efficient than a function that
       moves the superblock further up the list (if needed)
    */
    removeSuperBlock(sizeClass, superBlock);
    insertSuperBlock(sizeClass, superBlock);
}

void printSizeClass(size_class_t *sizeClass){
    int i;
    superblock_t *p=sizeClass->_SBlkList._first;
    printf("SizeClass [%d] # superblocks [%d]\n",sizeClass->_sizeClassBytes, sizeClass->_SBlkList._length);

    for(i=0;i< sizeClass->_SBlkList._length; i++, p=p->_meta._pNxtSBlk){
        printf("\n %d)  ",i);
        printSuperblock(p);
    }


}




size_t getSizeClassIndex(size_t size){
    double l=log(size)/log(2);
    return ceil(l);

}

size_class_t *getSizeClassForSuperblock(superblock_t *pSb){

    size_t i=getSizeClassIndex(pSb->_meta._sizeClassBytes);
    return &(pSb->_meta._pOwnerHeap->_sizeClasses[i]);
}

static bool is_single(size_class_t *size_class, superblock_t *superblock)
{
    if ((superblock->_meta._pPrvSblk == superblock->_meta._pNxtSBlk) &&
        (superblock->_meta._pPrvSblk == superblock)) {
        return true;
    }
    return false;
}

static void init_superblock_list(size_class_t *sizeClass, superblock_t *first)
{
    sizeClass->_SBlkList._first = first;
    sizeClass->_SBlkList._length = 1;

    /* Double-linked list is always circular */
    first->_meta._pNxtSBlk = first;
    first->_meta._pPrvSblk = first;
}

static superblock_t *find_least_full_than(size_class_t *sizeClass, double fullness)
{
    unsigned int i = 0;
    superblock_t *found = NULL;

    /* This is an ordered list, by the fullest block. Find before which
       block we sholud place the new block here */
    for (i = 0, found = sizeClass->_SBlkList._first;
         fullness < getFullness(found) && i < sizeClass->_SBlkList._length;
         i++, found = found->_meta._pNxtSBlk);

    return found;
}

static void place_superblock(superblock_t *new, superblock_t *place_before)
{
    superblock_t * place_after = place_before->_meta._pPrvSblk;

    assert(place_before->_meta._pOwnerHeap == place_after->_meta._pOwnerHeap);

    /* Insert before place_before, and after its predecessor in the list:
       Used to be:
          ... <--> place_after <--> place_before <-->
       Will be:
          ... <--> place_after <--> new <--> place_before <--> ...
       Note that this code will work even if there's only one block.
     */

    place_after->_meta._pNxtSBlk = new;
    new->_meta._pPrvSblk = place_after;
    new->_meta._pNxtSBlk = place_before;
    place_before->_meta._pPrvSblk = new;

    /* Verify that the new superblock belongs to the correct heap.
       This may be redundant, but good practice */
    new->_meta._pOwnerHeap = place_before->_meta._pOwnerHeap;
}
