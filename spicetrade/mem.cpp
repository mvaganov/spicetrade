//#include <vld.h> // for finding memory leaks with the VS2010 plugin

#include "mem.h"

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


void* operator new(size_t num_bytes) __NEWTHROW
{
	// if a memory leak message brought you here, try replacing your 'new' operators with NEWMEM(type)
	return operator new(num_bytes, __FILE__, __LINE__);
}
void* operator new[](size_t num_bytes) __NEWTHROW
{
	// if a memory leak message brought you here, try replacing your 'new' operators with NEWMEM_ARR(type, size)
	return operator new(num_bytes, __FILE__, __LINE__);
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
#ifdef MEM_LINKED_LIST
	/** when blocks are in a sequence (active/free block lists) */
	MemBlock * next;
#endif
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

static void __failIf(bool condition, const char * message) {
	if(condition) {
		printf("%s", message);
		int i=0;i=1/i;
	}
}

/** manages memory */
struct MemManager {
	/** the first memory page, which links to subsequent pages like a linked list */
	MemPage* mem;
	/** the default page size */
	size_t defaultPageSize;
#ifdef MEM_LINKED_LIST
	/** singly-linked list of used memory */
	MemBlock * usedList;
	/** singly-linked list of free memory */
	MemBlock * freeList;
#endif
#ifdef MEM_LEAK_DEBUG
	unsigned int largestRequest;
	unsigned int smallRequestSize;
	int numSmallRequests;
	int numAllocations;
#endif
	MemManager():mem(0),defaultPageSize(PAGE_SIZE_DEFAULT)
#ifdef MEM_LINKED_LIST
		,usedList(0),freeList(0)
#endif
#ifdef MEM_LEAK_DEBUG
		,largestRequest(0),smallRequestSize(32),numSmallRequests(0),numAllocations(0)
#endif
	{}

#ifdef MEM_LINKED_LIST
	/** @return a pointer to the pointer that points at the last block */
	MemBlock ** ptrTolastBlock(MemBlock * & whichList) {
		MemBlock ** ptr = &whichList;
		while(*ptr) {
			ptr = &((*ptr)->next);
#ifdef MEM_LEAK_DEBUG
			__failIf(*ptr == whichList, "singly-linked list became circular...");
#endif
		}
		return ptr;
	}
	bool blockInList(MemBlock* block, MemBlock* whichList) {
		for(MemBlock * m = freeList; m; m = m->next) {
#ifdef MEM_LEAK_DEBUG
			__failIf(m->next == whichList, "singly-linked list became circular...");
#endif			
			if(m == block) return true;
		}
		return false;
	}
#endif
	/** create a new memory page to be managed (uses malloc) */
	MemPage* newPage(size_t pagesize) {
		MemPage * m = (MemPage*)malloc(pagesize);
		__failIf(!m, "could not allocate a page of memory");
		m->next = 0;
		m->size = pagesize;
		MemBlock * memoryUnit = m->firstBlock();
#ifdef MEM_LINKED_LIST
		memoryUnit->next = 0;
#endif
		memoryUnit->markFree();
		memoryUnit->setSize(((signed)m->size)-(signed)sizeof(MemPage)-(signed)sizeof(MemBlock));
#ifdef MEM_LEAK_DEBUG
		// replace __FILE__ and __LINE__ with hex for "newBlock","-unused-" or "newB", "lock" for 32 bit
		memoryUnit->setupDebugInfo(__FILE__, __LINE__, numAllocations++);
#endif
		return m;
	}
	/** @return a page whose size is at least 'defaultPageSize' big, but could be 'requestedAllocation' big, if that is larger. */
	MemPage* newPageAtLeastBigEnoughFor(size_t requestedAllocation) {
		size_t pageSize = defaultPageSize;
		if(pageSize < requestedAllocation+sizeof(MemBlock)+sizeof(MemPage)){
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
	/** @return the address of the _pointer to_ the last page, not the address of the last page. Will never be NULL. */
	MemPage** lastPage() {
		MemPage** cursor = &mem;
		while(*cursor) {
			cursor = &((*cursor)->next);
		}
		return cursor;
	}
	MemPage* addPage(size_t size) {
		MemPage** lastP = lastPage();
		*lastP = newPage(size);
#ifdef MEM_LINKED_LIST
//		printf("addPage\n");
		MemBlock ** ptrToLast = ptrTolastBlock(freeList);
		(*ptrToLast) = (*lastP)->firstBlock();
		// if(freeList == NULL) { printf("~~~~~~ ADDPAGE %lu\n", size); }
#endif
		return *lastP;
	}
	
	MemPage* addPageAtLeastBigEnoughFor(size_t size){
		MemPage** lastP = lastPage();
		*lastP = newPageAtLeastBigEnoughFor(size);
#ifdef MEM_LINKED_LIST
//		printf("addPageAtLeastBigEnoughFor\n");
		MemBlock ** ptrToLast = ptrTolastBlock(freeList);
		(*ptrToLast) = (*lastP)->firstBlock();
		// if(freeList == NULL) { printf("~~~~~~ addPageAtLeastBigEnoughFor %lu\n", size);}
#endif
		return *lastP;
	}

#ifdef MEM_LEAK_DEBUG
#ifdef VERIFY_INTEGRITY
	bool verifyBlockIntegrity(MemBlock* block, const char * failMessage,
		const ptrdiff_t firstHeaderLoc, MemBlock* endOfThisPage,
		const int pages, int& usedSectors, int& usedBytes, int& freeSectors, int& freeBytes, int& missingFreeSectors) {
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
		if(!block->isFree()) {
			usedSectors++;
			usedBytes += block->getSize();
		}
		else {
			freeSectors++;
			freeBytes += block->getSize();
#ifdef MEM_LINKED_LIST
			if(!blockInList(block, freeList)) {
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

	void * allocate(size_t num_bytes, const char *filename, size_t line) {
#ifdef MEM_LINKED_LIST
		return allocate_ll(num_bytes, filename, line);
#else
		return allocate_easy(num_bytes, filename, line);
#endif
	}

	void mergeWithNextFreeBlocks(MemBlock * block, size_t endOfThisPage) {
		MemBlock * nextBlock = block->nextContiguousBlock();
		while((size_t)nextBlock < endOfThisPage && nextBlock->isFree()) {
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
		// mark another free block where this one will end
		MemBlock * next = block->nextContiguousHeader(bytesNeeded);
		next->setSize(block->getSize() - (bytesNeeded+sizeof(MemBlock)));
		next->markFree();
#ifdef MEM_LEAK_DEBUG
		// 0x454C4946 = 'file' (little end), 0x454E494C = 'line' (little end)
		next->setupDebugInfo((const char *)0x454C4946, 0x454E494C, numAllocations++);
#endif
		block->setSize(bytesNeeded);// mark this block exactly the size needed
		return next;
	}

	void* markNewlyAllocatedBlock(MemBlock* block, const char * filename, const int line) {
		void* allocatedMemory = block->allocatedMemory();
#ifdef MEM_ALLOCATED
		ptrdiff_t* imem = (ptrdiff_t*)allocatedMemory;
		size_t numints = block->getSize()/sizeof(ptrdiff_t);
		for(size_t i = 0; i < numints; ++i){
			imem[i] = MEM_ALLOCATED;
		}
#endif
#ifdef MEM_LEAK_DEBUG
		block->setupDebugInfo(filename, line, numAllocations++);
#endif
#ifdef VERIFY_INTEGRITY
		verifyIntegrity("allocation");
#endif
		block->markUsed();       // mark it as allocated
		return allocatedMemory;
	}

	void markNewlyDeallocatedBlock(MemBlock* header, void* memory) {
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
		header->markFree();
	}

	void* allocate_easy(size_t num_bytes, const char *filename, size_t line) {
#ifdef MEM_LEAK_DEBUG
		if(num_bytes > largestRequest)   { largestRequest = num_bytes; }
		if(num_bytes < smallRequestSize) { numSmallRequests++; }
#endif
		// allocated memory should be size_t aligned
		size_t bytesNeeded = num_bytes;
		if((bytesNeeded & ((signed)sizeof(ptrdiff_t)-1)) != 0) {
			bytesNeeded += (signed)sizeof(ptrdiff_t) - (num_bytes % sizeof(ptrdiff_t));
		}
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
			if(block->isFree()) {
				mergeWithNextFreeBlocks(block, endOfThisPage);
				if(block->getSize() > bytesNeeded+sizeof(MemBlock)) {
					splitBlock(block, bytesNeeded);// split big blocks
				}
				if(block->getSize() >= bytesNeeded) {
					return markNewlyAllocatedBlock(block, filename, line); // grab the section of memory that is being requested
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
#ifdef MEM_LINKED_LIST
	/**
	 * will allocate memory from the memory system
	 * @param num_bytes how many bytes are being asked for
	 * @param filename/line where, in code, the memory is requested from, used if MEM_LEAK_DEBUG defined
	 */
	void * allocate_ll(size_t num_bytes, const char *filename, size_t line) {
#ifdef MEM_LEAK_DEBUG
		if(num_bytes > largestRequest)   { largestRequest = num_bytes; }
		if(num_bytes < smallRequestSize) { numSmallRequests++; }
#endif
		// allocated memory should be size_t aligned
		size_t bytesNeeded = num_bytes;
		if((bytesNeeded & ((signed)sizeof(ptrdiff_t)-1)) != 0){
			bytesNeeded += (signed)sizeof(ptrdiff_t) - (num_bytes % sizeof(ptrdiff_t));
		}
		// which memory page is being searched
		MemPage * page = mem;
		// where this memory page ends
		int endOfThisPage;
		// which memory block is being searched
		MemBlock * block = 0;
		MemBlock ** prevNextPtr = NULL;
		do {
			if(!page) {
				page = addPageAtLeastBigEnoughFor(bytesNeeded);
				// printf("~~~~~~~ created a page that can store %lu bytes (%lu)\n", bytesNeeded, page->size);
			}
			// if no valid block is being checked at the moment, grab a block
			if(!block) {
				if(freeList == NULL) {
					// printf("~~~~~ no free blocks. Need to create a new page");
					page = addPageAtLeastBigEnoughFor(bytesNeeded);
					MemBlock* newBlock = page->firstBlock();
					freeList = newBlock;
				}
				prevNextPtr = &freeList;
				block = freeList;
			}
			// if it has enough space to be spliced into 2 blocks
			if(block->getSize() > bytesNeeded+sizeof(MemBlock)) {
				// printf("~~~~~~ splitting block %lu bytes (need %lu)", block->getSize(), bytesNeeded+sizeof(MemBlock));
				MemBlock * next = splitBlock(block, bytesNeeded);
				// printf("~~~~~~ now there are 2 blocks, %lu and %lu\n", block->getSize(), next->getSize());
				block->next = next; // point at the new block
			}
			// else if(block->getSize() == bytesNeeded+sizeof(MemBlock)) {
			// 	printf("~~~~~~ block has exactly as much as needed! %lu\n", block->getSize());
			// }
			// else {
			// 	printf("~~~~~~ block needs more space. has %lu, needs %lu!\n", block->getSize(), bytesNeeded+sizeof(MemBlock));
			// }
			// if the current block has enough space for this allocation
			if(block->getSize() >= bytesNeeded) {
				void* allocatedMemory = markNewlyAllocatedBlock(block, filename, line); // grab the section of memory that is being requested
				*prevNextPtr = block->next; // remove block from the free list. possibly sets freelist to null.
				block->next = usedList; usedList = block; // push block on to the used list
				return allocatedMemory;
			}
			// if there are no more free blocks
			if(!block->next) {
				// add another free page, which will append a free block to this free list
				addPageAtLeastBigEnoughFor(bytesNeeded);
			}
			// hold a reference to the reference that references the next memory block
			prevNextPtr = &block->next;
			// the next memory block is now the current memory block
			block = block->next;
		}
		while(true);	// while memory hasn't been found
		return 0;	// should never return here.
	}
#endif

	void deallocate(void * memory) {
		MemBlock * header = MemBlock::blockForAllocatedMemory(memory);//(MemBlock*)(((ptrdiff_t)memory)-sizeof(MemBlock));
		__failIf(header->isFree(), "double-deleting memory...");
		markNewlyDeallocatedBlock(header, memory);
#ifdef MEM_LINKED_LIST
		// used memory is not being listed. only free memory is important.
		// if it becomes important to keep track of used memory, the MemBlock should be a double linked list.
		//// remove this mem block from it's current list (not managing used memory)
		//{
		//	MemBlock ** p = &freeList;
		//	while((*p)->next != header){
		//		p = &(p->next);
		//	}
		//	*p = header->next;
		//	header->next = 0;
		//}
// all of this code is used to merge a singly-linked list of memory blocks when they are contiguous... sheesh.
		bool merged = false;
		// what mem block would go right after this?
		MemBlock * nextContBlock = header->nextContiguousBlock();
		// this *might* be a valid free block... we'll find that out soon
		MemBlock ** ptrToNextBlock = 0;
		// traverse the entire free list
		MemBlock * p = freeList;
		// (we'll be inserting and removing, so keep a back-reference-reference)
		MemBlock ** ptrToP = &freeList;
		// which block will back-merge at the end
		MemBlock * originBlock = header;
		while(p){
			// if the currently found mem block header is the expected next block
			if(p == nextContBlock){
				// then the next block is a free block, and it can be merged (done at the end)
				ptrToNextBlock = ptrToP;
			}
			// if this current block should merge with the currently free-ing block
			if(!merged){
				MemBlock * tempNextContBlock = p->nextContiguousBlock();
				if(tempNextContBlock == header){
	//				// put this block's next as the newly free'd block's next
	//				header->next = p->next;
					// extend this block over the newly free'd block
					p->setSize(p->getSize()+sizeof(MemBlock)+header->getSize());
					originBlock = p;
					merged = true;
				}
			}
			// go to the next free block
			ptrToP = &p->next;
			p = p->next;
		}
		// if this memory can merge with the next block
		if(ptrToNextBlock){
			// remove the next mem block from the old list
			if(merged){
				// if it was merged earlier, just reference the next guy.
				*ptrToNextBlock = nextContBlock->next;
			}
			else {
				// if it wasn't merged, merge it into the list now
				*ptrToNextBlock = originBlock;
				originBlock->next = nextContBlock->next;
			}
			// forward merge the next mem block from the previous mem block
			originBlock->setSize(originBlock->getSize()+sizeof(MemBlock)+nextContBlock->getSize());
			merged = true;
		}
		// if this mem block could not be merged into an existing mem block
		if(!merged){
			// push it on the list.
			header->next = freeList;
			freeList = header;	// push it real good.
			// if(freeList == NULL) { printf("~~~~~~~~~ THIS SHOULD BE IMPOSSIBLE!\n"); }
		}
#endif
	}

	void reportBlock(MemBlock* block, bool verbose, int& usedSectors, int& usedBytes, int& freeSectors, int& freeBytes) {
		if(!block->isFree()) {
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
//		headersTraversed=0;
		do {
			if(!block->isFree()) {
				printf("memory leak, %d bytes! %s:%d\n",
				(int)block->getSize(), block->filename, (int)block->line);
				leaks++;
			}
			block = block->nextContiguousBlock();
//			headersTraversed++;
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
//			int pagesTraversed=0, headersTraversed=0;
			do {
#ifdef MEM_LEAK_DEBUG
				leaks+= reportLeakingBlocks(mem);
#endif
				next = mem->next;
				free(mem);
				mem = next;
//				pagesTraversed++;
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
