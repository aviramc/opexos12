#ifndef _SIZE_CLASS_H_
#define _SIZE_CLASS_H_
#include "mtmm.h"

void removeSuperBlock(size_class_t *sizeClass, superblock_t *superBlock);
void insertSuperBlock(size_class_t *sizeClass, superblock_t *superBlock);
superblock_t * findMostlyEmptySuperblockSizeClass(size_class_t *sizeClass);
void *allocateBlockFromSizeClass(size_class_t *sizeClass, superblock_t *superBlock);
void freeBlockFromCurrentSizeClass(size_class_t *sizeClass, superblock_t *superBlock, block_header_t *block);

size_t size_class_used_bytes(size_class_t *sizeClass);

#endif /* _SIZE_CLASS_H_ */
