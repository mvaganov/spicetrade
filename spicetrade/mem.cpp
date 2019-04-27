//#include <vld.h> // for finding memory leaks with the VS2010 plugin

#include "mem.h"
#include "platform_conio.h"
#ifdef USE_CUSTOM_MEMORY_MANAGEMENT

#include <stdio.h>
#include <stdlib.h>

//#define MEM_LINKED_LIST

#define VERIFY_INTEGRITY

// forward reference
struct MemPage;

#define SIZEOFFLAG 1
#ifdef ENVIRONMENT32
#define SIZEOFMEMBLOCKSIZE	31
#elif defined(ENVIRONMENT64)
#define SIZEOFMEMBLOCKSIZE	63
#endif


void* operator new(size_t num_bytes) __NEWTHROW {
	// if a memory leak message brought you here, try replacing your 'new' operators with NEWMEM(type)
	return operator new(num_bytes, __FILE__, __LINE__);
}
void* operator new[](size_t num_bytes) __NEWTHROW {
	// if a memory leak message brought you here, try replacing your 'new' operators with NEWMEM_ARR(type, size)
	return operator new(num_bytes, __FILE__, __LINE__);
}

#define __failIf(failcondition, message, ...)	if(failcondition) {\
		printf("fail if %s\n", #failcondition);\
		printf(message, ##__VA_ARGS__);\
		int i=0;i=1/i; }

/** ptrTo could be used for the same purpose, but this does it with less pointer math! */
template<typename T>
bool linkedList_contains(T* whichList, T* block) {
	for(T* m = whichList; m; m = m->next) {
		if(m == block) {
			// printf("found %zx\n", (size_t)m);
			return true;
		}
	}
	return false;
}
/** @return a pointer to the pointer-pointing-at-the-given-pointer */
template<typename T>
T ** linkedList_findPtrTo(T* &whichListToSearch, T* whichBlock) {
	T ** ptr = &whichListToSearch;
#ifdef VERIFY_INTEGRITY
	int loopguard = 0;
#endif
	// printf("(%zx) [ ", (size_t)whichBlock); fflush(stdout);
	while(ptr && ((*ptr) != whichBlock)) {
		// printf("%zx ", (size_t)*ptr); fflush(stdout);
		if(!(*ptr) || !(*ptr)->next) {  // if the next block is NULL, stop before trying to get NULL's ->next property.
			ptr = NULL;
			break;
		}
		ptr = &((*ptr)->next); // point at the pointer-to the next block
#ifdef VERIFY_INTEGRITY
		__failIf((loopguard++ > (1 << 10)), "list recursion looking for pointer");
#endif
	}
	// printf("] %zx   %d\n", (size_t)ptr, (ptr && *ptr == whichBlock));
	return ptr; // will return NULL if whichBlock is not in the list whichListToSearch
}
template<typename T>
void linkedList_print(T* headPtr) {
	printf("[ "); fflush(stdout);
	for(T* m=headPtr;m;m=m->next) { printf("%zx ", (size_t)m); fflush(stdout); }
	printf("]\n");
}

template<typename T>
int linkedList_size(T* headPtr) {
	int count = 0; for(T* m=headPtr;m;m=m->next){++count;}
	return count;
}
template<typename T>
bool linkedList_remove(T* &headPtr, T* b) {
	T** ptr = linkedList_findPtrTo(headPtr, b);
	if(ptr && *ptr == b) {
		// printf("~~~~ removed from list 0x%016zx, 0x%016zx, 0x%016zx\n", (size_t)b, (size_t)ptr, (size_t)*ptr);
		(*ptr) = b->next;
		b->next = NULL;
		return true;
	}
	return false;
}
template<typename T>
void linkedList_push(T* &headPtr, T* b) {
#ifdef MEM_LEAK_DEBUG
	bool blockAlreadyInList = linkedList_contains(headPtr, b);
	__failIf(blockAlreadyInList, "pushing a block which is already in the list.");
#endif
	b->next = headPtr; headPtr = b;
}

/** a memory block, which is managed by a memory page */
class MemBlock {
	/** this is a free block, it can be given out to callers who ask for memory */
	static const int FREE = 1 << 0;
	/** how many bytes this header is in front of (the size of the allocation), only 31 bits given. value does not include the header's size */
	size_t size:SIZEOFMEMBLOCKSIZE;
	/** whether or not this memory block is free or not, only 1 bit given */
	size_t flag:SIZEOFFLAG;
public:
	/** next block in a sequence (active/free block lists) */
	MemBlock * next;
#ifdef MEM_LEAK_DEBUG
	size_t signature;
	const char *filename;
	size_t line;
	size_t allocID;	// which allocation this is
	inline void setupDebugInfo(const char *a_filename, size_t a_line, size_t a_allocID) {
		signature = MEM_LEAK_DEBUG;
		filename = a_filename;
		line = a_line;
		allocID = a_allocID;
	}
#endif
	/** @return true if this block is marked as free */
	inline bool isFree() { return (flag & MemBlock::FREE) != 0; }
	/** set this block to count as free (may be allocated by the allocator later) */
	inline void markFree() { flag |= MemBlock::FREE; }
	/** set this block as used (will not be allocated) */
	inline void markUsed() { flag &= ~MemBlock::FREE; }
	/** how many usable bytes this block manages. total size of the block is getSize()+sizeof(MemBlock) */
	inline size_t getSize() { return size; }
	/** sets the reported size of this block */
	inline void setSize(size_t a_size) { size = a_size; }
	/** @return the memory block that comes after this one if this block were the given size (may go out of bounds of the current memory page!) */
	inline MemBlock * nextContiguousHeader(size_t givenSize) {
		return (MemBlock*)(((ptrdiff_t)this)+(givenSize+sizeof(MemBlock)));
	}
	/** @return the memory block that comes after this one (may go out of bounds of the current memory page!) */
	inline MemBlock * nextContiguousBlock() {
		return (MemBlock*)(((ptrdiff_t)this)+(getSize()+sizeof(MemBlock)));
	}
	/** @return the allocated memory managed by this block */
	void * allocatedMemory() { return (void*)((ptrdiff_t)this+sizeof(*this)); }
	static MemBlock * blockForAllocatedMemory(void * memory) {
		return (MemBlock*)(((ptrdiff_t)memory)-sizeof(MemBlock));
	}
};

void linkedList_print(MemBlock* headPtr) {
	printf("[ "); fflush(stdout);
	for(MemBlock* m=headPtr;m;m=m->next) { printf("%zx(%zi) ", (size_t)m, m->getSize()); fflush(stdout); }
	printf("]\n");
}

int MEM::getAllocatedHere(void * a_memory) {
	MemBlock * mb = MemBlock::blockForAllocatedMemory(a_memory);
	return mb->getSize();
}

/** a memory page, which is a big chunk of memory that is pooled, and allocated from */
struct MemPage {
	MemPage* next;
	size_t size;
	/** @return where the first memory block is in this memory page */
	inline MemBlock * firstBlock() {
		return (MemBlock*)(((ptrdiff_t)this)+sizeof(MemPage));
	}
};

/** manages memory */
struct MemManager {
	/** the first memory page, which links to subsequent pages like a linked list */
	MemPage* mem;
	/** the default page size */
	size_t defaultPageSize;
//#ifdef MEM_LINKED_LIST
	/** singly-linked list of used memory */
	MemBlock * usedList;
	/** singly-linked list of free memory */
	MemBlock * freeList;
//#endif
#ifdef MEM_LEAK_DEBUG
	unsigned int largestRequest;
	unsigned int smallRequestSize;
	int numSmallRequests;
	int numAllocations;
#endif
	MemManager():mem(0),defaultPageSize(PAGE_SIZE_DEFAULT)
// #ifdef MEM_LINKED_LIST
		,usedList(0),freeList(0)
// #endif
#ifdef MEM_LEAK_DEBUG
		,largestRequest(0),smallRequestSize(32),numSmallRequests(0),numAllocations(0)
#endif
	{}

	/** create a new memory page to be managed (uses malloc) */
	MemPage* newPage(size_t pagesize) {
		MemPage * m = (MemPage*)malloc(pagesize);
		__failIf(!m, "could not allocate a page of memory");
		m->next = 0;
		m->size = pagesize;
		MemBlock * memoryUnit = m->firstBlock();
		// printf("~~~~ newpage\n");
		setFree(memoryUnit);
		memoryUnit->setSize(((signed)m->size)-(signed)sizeof(MemPage)-(signed)sizeof(MemBlock));
#ifdef MEM_LEAK_DEBUG
		// replace __FILE__ and __LINE__ with hex for "newBlock","-unused-" or "newB", "lock" for 32 bit
		memoryUnit->setupDebugInfo(__FILE__, __LINE__, numAllocations++);
#endif
		return m;
	}

	bool isFree(MemBlock* b) {
		bool freedom = //b->isFree();
			linkedList_contains(freeList, b);
		return freedom;
	}

	void setFree(MemBlock* b) {
		linkedList_remove(usedList, b);
		linkedList_push(freeList, b);
		// b->markFree();
	}

	void setUsed(MemBlock * b) {
		linkedList_remove(freeList, b);
		linkedList_push(usedList, b);
		// b->markUsed();
	}

	/** @return a page whose size is at least 'defaultPageSize' big, but could be 'requestedAllocation' big, if that is larger. */
	MemPage* newPageAtLeastBigEnoughFor(size_t requestedAllocation) {
		size_t pageSize = defaultPageSize;
		if(pageSize < requestedAllocation+sizeof(MemBlock)+sizeof(MemPage)) {
			// allocate the larger amount
			pageSize = requestedAllocation+sizeof(MemBlock)+sizeof(MemPage);
		}
		MemPage* result = newPage(pageSize);
#ifdef VERIFY_INTEGRITY
		if(!result) {
			verifyIntegrity("Allocation fail");
		}
#endif			
		return result;
	}
	static ptrdiff_t endOfPage(MemPage * page) {
		return ((ptrdiff_t)page)+page->size;
	}
	MemPage* addPage(size_t size) {
		MemPage* mp = newPage(size);
		mp->next = mem;
		mem = mp;
		return mp;
	}
	
	MemPage* addPageAtLeastBigEnoughFor(size_t size){
		MemPage* mp = newPageAtLeastBigEnoughFor(size);
		if(mp) { mp->next = mem; mem = mp; }
		return mp;
	}

#ifdef MEM_LEAK_DEBUG
#ifdef VERIFY_INTEGRITY
	bool verifyBlockIntegrity(MemBlock* block, const char * failMessage,
		const ptrdiff_t firstHeaderLoc, MemBlock* endOfThisPage,
		const int pages, int& usedSectors, int& usedBytes, int& freeSectors, int& freeBytes, int& missingFree) {
		if(block->signature != MEM_LEAK_DEBUG) {
			const char * errMsg =
#ifdef ENVIRONMENT32
			"\n%s\n\nintegrity failure at page %d, header %d (%d/%d)\n\n\n";
#else
			"\n%s\n\nintegrity failure at page %lu, header %lu (%lu/%lu)\n\n\n";
#endif
			printf(errMsg, failMessage, pages, usedSectors+freeSectors, ((ptrdiff_t)block)-firstHeaderLoc, ((ptrdiff_t)endOfThisPage)-firstHeaderLoc);
			return false;
		}
		if(!isFree(block)) {
			usedSectors++;
			usedBytes += block->getSize();
		}
		else {
			freeSectors++;
			freeBytes += block->getSize();
#ifdef MEM_LINKED_LIST
			if(!linkedList_contains(freeList, block)) {
				printf("[[0x%08zx]] ", (size_t)block);
				for(MemBlock * b = freeList; b; b=b->next) {
					printf("0x%08zx ", (size_t)b);
				}
				missingFree++;
				const char * errMsg =
#	ifdef ENVIRONMENT32
				"\n%s\n\nintegrity failure at page %d, header %d (%d/%d): marked free but not in freelist\n\n\n";
#	else
				"\n%s\n\nintegrity failure at page %lu, header %lu (%lu/%lu): marked free but not in freelist\n\n\n";
#	endif
				printf(errMsg, failMessage, pages, usedSectors+freeSectors, ((ptrdiff_t)block)-firstHeaderLoc, ((ptrdiff_t)endOfThisPage)-firstHeaderLoc);
				return false;
			}
#endif
		}
		return true;
	}

	bool verifyIntegrity(const char *failMessage) {
		if(!mem) return true;
		MemPage * current = mem;
		ptrdiff_t firstHeaderLoc;
		int pages = 0;
		int freeSectors = 0;
		int freeBytes = 0;
		int usedSectors = 0;
		int usedBytes = 0;
		int missingFree = 0;
		do {
			MemBlock* block = current->firstBlock();
			firstHeaderLoc = (ptrdiff_t)block;
			MemBlock* endOfThisPage = (MemBlock*)endOfPage(current);
			MemBlock * last;
			// verify going forwards
			do {
				last = block;
				if(!verifyBlockIntegrity(block, failMessage, firstHeaderLoc, endOfThisPage, 
				pages, usedSectors, usedBytes, freeSectors, freeBytes, missingFree)) { return false; }
				block = block->nextContiguousBlock();
			}
			while((ptrdiff_t)block < (ptrdiff_t)endOfThisPage);
			block = last;
			current = current->next;
			pages++;
		}
		while(current);
		return true;
	}
#endif
#endif

	/**
	 * will allocate memory from the memory system
	 * @param bytesNeeded how many bytes are being asked for
	 * @param filename/line where, in code, the memory is requested from, used if MEM_LEAK_DEBUG defined
	 */
	void * allocate(size_t num_bytes, const char *filename, size_t line) {
#ifdef MEM_LEAK_DEBUG
		if(num_bytes > largestRequest)   { largestRequest = num_bytes; }
		if(num_bytes < smallRequestSize) { numSmallRequests++; }
#endif
		// allocated memory should be size_t aligned
		size_t bytesNeeded = num_bytes;
		if((bytesNeeded & ((signed)sizeof(ptrdiff_t)-1)) != 0){
			bytesNeeded += (signed)sizeof(ptrdiff_t) - (num_bytes % sizeof(ptrdiff_t));
		}
//#ifdef MEM_LINKED_LIST
		return allocate_ll(num_bytes, filename, line);
//#else
//		return allocate_easy(num_bytes, filename, line);
//#endif
	}

	void mergeWithNextFreeBlocks(MemBlock * block, size_t endOfThisPage) {
		MemBlock * nextBlock = block->nextContiguousBlock();
		while((size_t)nextBlock < endOfThisPage && isFree(nextBlock)) {
			linkedList_remove(freeList, nextBlock);
			#ifdef MEM_LEAK_DEBUG
			__failIf(nextBlock->signature != MEM_LEAK_DEBUG, "memory corruption... can't traverse as expected!");
			#endif
			#ifdef MEM_CLEARED_HEADER
			size_t* imem = (size_t*)nextBlock;
			#endif
			nextBlock = nextBlock->nextContiguousBlock();
			#ifdef MEM_CLEARED_HEADER
			size_t numClears = sizeof(MemBlock)/sizeof(ptrdiff_t);
			for(int i = 0; i < numClears; ++i) {
				imem[i]=MEM_CLEARED_HEADER;
			}
			#endif
		}
		block->setSize(((size_t)nextBlock-(size_t)block)-sizeof(MemBlock));
	}

	MemBlock* splitBlock(MemBlock* block, const size_t& bytesNeeded) {
		// printf("~~~~ splitting block %zx\n", (size_t)block);
		// mark another free block where this one will end
		MemBlock * next = block->nextContiguousHeader(bytesNeeded);
		next->setSize(block->getSize() - (bytesNeeded+sizeof(MemBlock)));
//		setFree(next);
		linkedList_push(freeList, next);
		// printf("~~~~ pushed next %zx\n", (size_t)next);
		#ifdef MEM_LEAK_DEBUG
		next->setupDebugInfo("<split>", 0, numAllocations);
		#endif
		block->setSize(bytesNeeded); // mark this block exactly the size needed
		return next;
	}

	void* initNewAllocation(MemBlock* block, const char * filename, const int line) {
		void* allocatedMemory = block->allocatedMemory();
		#ifdef MEM_ALLOCATED
		ptrdiff_t* imem = (ptrdiff_t*)allocatedMemory;
		size_t numints = block->getSize()/sizeof(ptrdiff_t);
		for(size_t i = 0; i < numints; ++i) {
			imem[i] = MEM_ALLOCATED;
		}
		#endif
		#ifdef MEM_LEAK_DEBUG
		block->setupDebugInfo(filename, line, numAllocations++);
		#endif
		#ifdef VERIFY_INTEGRITY
		verifyIntegrity("allocation");
		#endif
		return allocatedMemory;
	}

	void wipeOldAllocation(MemBlock* header, void* memory) {
		#ifdef MEM_CLEARED
		ptrdiff_t* imem = (ptrdiff_t*)memory;
		size_t numInts = header->getSize()/sizeof(ptrdiff_t);
		for(size_t i = 0; i < numInts; ++i){
			imem[i]=MEM_CLEARED;
		}
		#endif
		#ifdef VERIFY_INTEGRITY
		verifyIntegrity("deallocation");
		#endif
	}

	void* allocate_easy(size_t bytesNeeded, const char *filename, size_t line) {
		MemPage * page = mem;  // which memory page is being searched
		size_t endOfThisPage;  // where this memory page ends
		MemBlock * block = 0;  // which memory block is being looked at
		do {
			if(!page) {
				page = addPageAtLeastBigEnoughFor(bytesNeeded);
				if(!page) break;
			}
			// if no valid block is being checked, grab the first block of this page
			if(!block) {
				block = page->firstBlock();      // memory will have to be traversed from the first block at the beginning of the page
				endOfThisPage = endOfPage(page); // keep track of where the page ends
			}
			if(isFree(block)) {
				mergeWithNextFreeBlocks(block, endOfThisPage);
				if(block->getSize() > bytesNeeded+sizeof(MemBlock)) {
					splitBlock(block, bytesNeeded);// split big blocks
				}
				if(block->getSize() >= bytesNeeded) {
					setUsed(block);
					return initNewAllocation(block, filename, line); // grab the section of memory that is being requested
				}
			}
			block = block->nextContiguousBlock(); // check the next block! maybe it's free?
			if((size_t)block >= endOfThisPage) {  // if out of bounds, go to the next page
				page = page->next;
				block = 0;                        // check next page from the beginning	
			}
		}
		while(true); // while memory hasn't been found
		return NULL;
	}

	void * allocate_ll(size_t bytesNeeded, const char *filename, size_t line) {
		MemBlock * block = freeList;
		do {
			// if there are no valid free blocks left, create a new one
			if(!block) {
				addPageAtLeastBigEnoughFor(bytesNeeded); // should push block to freeList
				if(!freeList) { break; }
				block = freeList;
			}
			// large-enough blocks should be split up
			if(block->getSize() > bytesNeeded+sizeof(MemBlock)) {
				MemBlock * next = splitBlock(block, bytesNeeded); // also adds to free list.
#ifdef VERIFY_INTEGRITY
				__failIf(!linkedList_contains(freeList, block) && !linkedList_contains(freeList, next),"new blocks are not in the free list!\n");
#endif
			}
			if(block->getSize() >= bytesNeeded) {
				linkedList_remove(freeList, block); // remove block from the free list. possibly sets freelist to null.
				linkedList_push(usedList, block);
				void* allocatedMemory = initNewAllocation(block, filename, line); // grab the section of memory that is being requested
				return allocatedMemory;
			}
			block = block->next;
		}
		while(true);	// keep looking. if no free blocks exist, we will malloc another
		return NULL;	// the loop will break if we can't malloc another
	}
//#endif

	static MemBlock ** findPtrToContiguousBlockBefore(MemBlock* &whichListToSearch, MemBlock* whichBlock) {
		MemBlock ** ptr = &whichListToSearch;
		while(ptr) { // don't search if there is no list
			if(!(*ptr)) { return NULL; } // don't search if the next node is NULL
			if((*ptr)->nextContiguousBlock() == whichBlock) { break; } // stop if the next node is what we're looking for, the one right-before whichBlock in contiguous memory
			ptr = &((*ptr)->next); // point at the pointer-to the next block
		}
		return ptr; // will return NULL if whichBlock is not in the list whichListToSearch
	}
	static MemBlock ** findPtrToContiguousBlockAfter(MemBlock* &whichListToSearch, MemBlock* whichBlock) {
		MemBlock ** ptr = &whichListToSearch;
		MemBlock * nextone = whichBlock->nextContiguousBlock();
		while(ptr) {
			if(!(*ptr)) { return NULL; }
			if((*ptr) == nextone) { break; }
			ptr = &((*ptr)->next); // point at the pointer-to the next block
		}
		return ptr; // will return NULL if whichBlock is not in the list whichListToSearch
	}

	void blockEnvelopesNext(MemBlock * b, MemBlock * endBlock) {
		size_t endOfNext = (size_t)(endBlock->nextContiguousBlock());
		size_t startOfBlockData = (size_t)(b->allocatedMemory());
		b->setSize(endOfNext - startOfBlockData);
	}

	void deallocate(void * memory) {
		MemBlock * header = MemBlock::blockForAllocatedMemory(memory);//(MemBlock*)(((ptrdiff_t)memory)-sizeof(MemBlock));
#ifdef MEM_LEAK_DEBUG
		bool isUsed = linkedList_contains(usedList, header);
		// printf("free pre delete: "); linkedList_print(freeList);
		// printf("used pre delete: "); linkedList_print(usedList);
		if(!isUsed) return;
		__failIf(!isUsed, "deleting unmanaged memory: 0x%016zx\n", (size_t)header);
#endif
		__failIf(isFree(header), "double-deleting memory: 0x%016zx\n", (size_t)header);
		// printf("~~deallocate");
		linkedList_remove(usedList, header);

		// find the pointer-pointing-at-BEF (right before 'header' in memory)
		MemBlock ** ptrToBEF = findPtrToContiguousBlockBefore(freeList, header);
		// find the pointer-pointing-at-AFT (right after 'header' in memory)
		MemBlock ** ptrToAFT = findPtrToContiguousBlockAfter(freeList, header);
		// if there is an AFT and no BEF. [used] [deleting] [free]
		if(!ptrToBEF && ptrToAFT) {
			// printf("~~~ need to extend forward. %zx, %li bytes into %zx, %li bytes\n",
			// 	(size_t)header, header->getSize(), (size_t)(*ptrToAFT), (*ptrToAFT)->getSize());
			blockEnvelopesNext(header, *ptrToAFT); // envelop the next block
			header->next = (*ptrToAFT)->next;     // keep pointing where the next block pointed
			(*ptrToAFT) = header;                // replace the enveloped block in the free list
		}
		// if there is a BEF and no AFT. [free] [deleting] [used]
		else if(ptrToBEF && !ptrToAFT) {
			// printf("~~~ need to have previous node extend forward\n");
			blockEnvelopesNext(*ptrToBEF, header); // have the previous block envelop this one
		}
		// if there is an AFT and a BEF (3 way merge). [free] [deleting] [free]
		else if(ptrToBEF && ptrToAFT) {
			// printf("~~~ need to remove last one in series and extend forward across it\n");
			blockEnvelopesNext(*ptrToBEF, *ptrToAFT); // envelop across both free blocks
			// take pointer-pointing-at-AFT, and point it to AFT's next (essentially taking AFT out of the list)
			(*ptrToAFT) = (*ptrToAFT)->next;
		}
		else {
			linkedList_push(freeList, header);
			wipeOldAllocation(header, memory);
		}
		// printf("used postdelete: "); linkedList_print(usedList);
		// printf("free postdelete: "); linkedList_print(freeList);
	}

	void reportBlock(MemBlock* block, bool verbose, int& usedSectors, int& usedBytes, int& freeSectors, int& freeBytes) {
		if(!isFree(block)) {
			if(verbose) {
				printf("#%d: %d bytes from %s:%d\n",
				(int)block->allocID, (int)block->getSize(), block->filename, (int)block->line);
			}
			usedSectors++;
			usedBytes += block->getSize();
		}
		else {
			if(verbose) {
				printf("%d bytes free\n", (int)block->getSize());
			}
			freeSectors++;
			freeBytes += block->getSize();
		}
	}

	void reportMemory(bool verbose = false) {
#ifdef MEM_LEAK_DEBUG
		verbose = true;
#endif
		if(!mem) {
			printf("memory clean\n");
			return;
		}
		MemPage * current = mem;
		int pages = 0;
		int freeSectors = 0;
		int freeBytes = 0;
		int usedSectors = 0;
		int usedBytes = 0;
		do {
			MemBlock* block = current->firstBlock();
			MemBlock* endOfThisPage = (MemBlock*)endOfPage(current);
			printf("page %d [0x%08lx -> 0x%08lx) (%lu bytes)\n", pages, (long)current, (long)endOfThisPage, ((size_t)endOfThisPage-(size_t)current));
			do {
				reportBlock(block, verbose, usedSectors, usedBytes, freeSectors, freeBytes);
				block = block->nextContiguousBlock();
			}
			while((ptrdiff_t)block < (ptrdiff_t)endOfThisPage);
			current = current->next;
			pages++;
		}
		while(current);
		printf("%d pages (%d bytes of overhead)\n", (int)pages, (int)(pages*sizeof(MemPage)));
		printf("      blocks  useableBytes total\n");
		int sizeOfBlockHeader = sizeof(MemBlock);
		printf("free : %6d %12d %d\n", freeSectors, freeBytes, freeSectors*sizeOfBlockHeader+freeBytes);
		printf("used : %6d %12d %d\n", usedSectors, usedBytes, usedSectors*sizeOfBlockHeader+usedBytes);
#ifdef MEM_LEAK_DEBUG
		printf("largest request: %d\n", largestRequest);
		printf("%d requests less than %d bytes\n", numSmallRequests, smallRequestSize);
#endif
	}

	int reportLeakingBlocks(MemPage* mem) {
		int leaks=0;
		MemBlock* block = mem->firstBlock();
		MemBlock* endOfThisPage = (MemBlock*)endOfPage(mem);
		do {
			if(!isFree(block)) {
				printf("memory leak, %d bytes! %s:%d\n",
				(int)block->getSize(), block->filename, (int)block->line);
				leaks++;
			}
			block = block->nextContiguousBlock();
		}
		while((ptrdiff_t)block < (ptrdiff_t)endOfThisPage);
		return leaks;
	}

	int release() {
#ifdef MEM_LEAK_DEBUG
		int leaks = 0;
#endif
		if(mem) {
			MemPage * next;
			do {
#ifdef MEM_LEAK_DEBUG
				leaks+= reportLeakingBlocks(mem);
#endif
				next = mem->next;
				free(mem);
				mem = next;
			}
			while(mem);
#ifdef MEM_LEAK_DEBUG
			if(leaks) {
				printf("found %d memory leaks!\n", leaks);
			}
#endif
		}
		mem = 0;
#ifdef MEM_LEAK_DEBUG
		return leaks;
#else
		return 0;
#endif
	}
	~MemManager() { release(); }
};

static MemManager s_memory;

static ptrdiff_t g_stackbegin, g_stackend;
/**
 * @param ptr where to mark the stack as starting
 * @param stackSize how much stack space to assume
 */
void MEM::markAsStack(void * ptr, size_t stackSize)
{
	g_stackbegin = (ptrdiff_t)ptr;
	g_stackend = g_stackbegin + stackSize;
}

/** @return how many bytes expected to be valid beyond this memory address. markAsStack should be called prior to this */
size_t MEM::validBytesAt(void * ptr)
{
	ptrdiff_t start = g_stackbegin, end = g_stackend, thisone = (ptrdiff_t)ptr;
	// if this pointer is on the stack
	if(thisone >= start && thisone < end){
		// return the amount of bytes from this pointer to the perceived end of the stack
		return end-thisone;
	}
	// iterate through all memory pages, checking if this pointer is in paged memory
	MemPage * cursor = s_memory.mem;
	while(cursor) {
		start = (ptrdiff_t)cursor;
		end = start+cursor->size;
		if(thisone >= start && thisone < end){
			return end-thisone;
		}
		cursor = cursor->next;
	}
	return 0;
}

int MEM::RELEASE_MEMORY() {
	return s_memory.release();
}

void MEM::REPORT_MEMORY(bool verbose) {
	s_memory.reportMemory(verbose);
}

/** overrides where the source of memory allocation should be documented as */
const char * __NEWMEM_FILE_NAME=0;
int __NEWMEM_FILE_LINE=0;

void* operator new( size_t num_bytes, const char *filename, int line) __NEWTHROW
{
	if(__NEWMEM_FILE_NAME){
		filename = __NEWMEM_FILE_NAME;
		line = __NEWMEM_FILE_LINE;
	}
	__NEWMEM_FILE_NAME=0;
	__NEWMEM_FILE_LINE=0;
//	printf("alloc %10d   %s:%d\n", num_bytes, filename, line);
	void * resultMemory = s_memory.allocate(num_bytes, filename, line);
	return resultMemory;
}

void* operator new[]( size_t num_bytes, const char *filename, int line) __NEWTHROW
{
	return operator new(num_bytes, filename, line);
}

void operator delete(void* data, const char *filename, int line) throw()
{
	return s_memory.deallocate(data);
}
void operator delete[](void* data, const char *filename, int line) throw()
{
	return operator delete(data, filename, line);
}

void operator delete(void* data) throw()
{
	return s_memory.deallocate(data);
}
void operator delete[](void* data) throw()
{
	return operator delete(data);
}
#endif
