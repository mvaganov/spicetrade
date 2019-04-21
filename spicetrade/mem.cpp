//#include <vld.h> // for finding memory leaks with the VS2010 plugin

#include "mem.h"

#ifdef USE_CUSTOM_MEMORY_MANAGEMENT

// this is used for memory debugging, to stop at a specific allocation
//#define WAITING_FOR_ALLOCATION_ID 2

#include <stdio.h>
#include <stdlib.h>

#define MEM_LINKED_LIST

#define VERIFY_INTEGRITY

// forward reference
struct MemPage;

#define SIZEOFFLAG 1
#ifdef ENVIRONMENT32
#define SIZEOFMEMBLOCKSIZE	31
#elif defined(ENVIRONMENT64)
#define SIZEOFMEMBLOCKSIZE	63
#endif


//#include "../scripting/tokenenclosure.h"
//extern TemplateVector<TokenRule*> * g_rules;
#define MEM_DEBUG_INFRASTRUCTURE	//	printf("[g_rules: 0x%08x]%s:%d\n", g_rules, "mem", __LINE__);


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
class MemBlock{
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
	inline void setupDebugInfo(const char *a_filename, size_t a_line, size_t a_allocID){
		signature = MEM_LEAK_DEBUG;
		filename = a_filename;
		line = a_line;
		allocID = a_allocID;
#ifdef WAITING_FOR_ALLOCATION_ID
		if(allocID == WAITING_FOR_ALLOCATION_ID){
			int i=0;i=1/i;
		}
#endif
	}
#endif
	/** @return true if this block is marked as free */
	inline bool isFree(){
		return (flag & MemBlock::FREE) != 0;
	}
	/** set this block to count as free (may be allocated by the allocator later) */
	inline void markFree(){
		flag |= MemBlock::FREE;
	}
	/** set this block as used (will not be allocated) */
	inline void markUsed(){
		flag &= ~MemBlock::FREE;
	}
	/** how many usable bytes this block manages. total size of the block is getSize()+sizeof(MemBlock) */
	inline size_t getSize(){
		return size;
	}
	/** sets the reported size of this block */
	inline void setSize(size_t a_size){
		size = a_size;
	}
	/** @return the memory block that comes after this one if this block were the given size (may go out of bounds of the current memory page!) */
	inline MemBlock * nextContiguousHeader(size_t givenSize){
		return (MemBlock*)(((ptrdiff_t)this)+(givenSize+sizeof(MemBlock)));
	}
	/** @return the memory block that comes after this one (may go out of bounds of the current memory page!) */
	inline MemBlock * nextContiguousBlock(){
		return (MemBlock*)(((ptrdiff_t)this)+(getSize()+sizeof(MemBlock)));
	}
	/** @return the allocated memory managed by this block */
	void * allocatedMemory(){
		return (void*)((ptrdiff_t)this+sizeof(*this));
	}
	static MemBlock * blockForAllocatedMemory(void * memory)
	{
		return (MemBlock*)(((ptrdiff_t)memory)-sizeof(MemBlock));
	}
};
int MEM::getAllocatedHere(void * a_memory)
{
	MemBlock * mb = MemBlock::blockForAllocatedMemory(a_memory);
	return mb->getSize();
}

/** a memory page, which is a big chunk of memory that is pooled, and allocated from */
struct MemPage{
	MemPage* next;
	size_t size;
	/** @return where the first memory block is in this memory page */
	inline MemBlock * firstBlock(){
		return (MemBlock*)(((ptrdiff_t)this)+sizeof(MemPage));
	}
};

/** manages memory */
struct MemManager{
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
	MemBlock ** ptrTolastBlock(MemBlock * & whichList){
		MemBlock ** ptr = &whichList;
		while(*ptr){
			ptr = &((*ptr)->next);
#ifdef MEM_LEAK_DEBUG
			if(*ptr == whichList){
				printf("singly-linked list became circular...");
				int i=0;i=1/i;
			}
#endif
		}
		return ptr;
	}
#endif
	/** create a new memory page to be managed (uses malloc) */
	MemPage* newPage(size_t pagesize){
		MemPage * m = (MemPage*)malloc(pagesize);
		if(!m){
			// could not allocate a page of memory
			int i=0;i=1/i;
		}
		m->next = 0;
		m->size = pagesize;
		MemBlock * memoryUnit = m->firstBlock();
		memoryUnit->markFree();
		memoryUnit->setSize(((signed)m->size)-(signed)sizeof(MemPage)-(signed)sizeof(MemBlock));
#ifdef MEM_LINKED_LIST
		memoryUnit->next = 0;
#endif
#ifdef MEM_LEAK_DEBUG
		// replace __FILE__ and __LINE__ with hex for "newBlock","-unused-" or "newB", "lock" for 32 bit
		memoryUnit->setupDebugInfo(__FILE__, __LINE__, numAllocations++);
#endif
		return m;
	}
	/** @return a page whose size is at least 'defaultPageSize' big, but could be 'requestedAllocation' big, if that is larger. */
	MemPage* newPageAtLeastBigEnoughFor(size_t requestedAllocation){
		size_t pageSize = defaultPageSize;
		if(pageSize < requestedAllocation+sizeof(MemBlock)+sizeof(MemPage)){
			// allocate the larger amount
			pageSize = requestedAllocation+sizeof(MemBlock)+sizeof(MemPage);
		}
		return newPage(pageSize);
	}
	static ptrdiff_t endOfPage(MemPage * page){
		return ((ptrdiff_t)page)+page->size;
	}
	/** @return the address of the _pointer to_ the last page, not the address of the last page. Will never be NULL. */
	MemPage** lastPage(){
		MemPage** cursor = &mem;
		while(*cursor){
			cursor = &((*cursor)->next);
		}
		return cursor;
	}
	MemPage* addPage(size_t size){
		MemPage** lastP = lastPage();
		*lastP = newPage(size);
#ifdef MEM_LINKED_LIST
//		printf("addPage\n");
		MemBlock ** ptrToLast = ptrTolastBlock(freeList);
		(*ptrToLast) = (*lastP)->firstBlock();
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
#endif
		return *lastP;
	}

#ifdef MEM_LEAK_DEBUG
#ifdef VERIFY_INTEGRITY
	bool verifyIntegrity(const char *failMessage){
		MemPage * current = mem;
		if(current){
			size_t pages = 0;
			size_t freeSectors = 0;
			size_t freeBytes = 0;
			size_t usedSectors = 0;
			size_t usedBytes = 0;
			ptrdiff_t firstHeaderLoc;
			do{
				MemBlock* block = current->firstBlock();
				firstHeaderLoc = (ptrdiff_t)block;
				MemBlock* endOfThisPage = (MemBlock*)endOfPage(current);
				MemBlock * last;
				// verify going forwards
				do{
					last = block;
					if(block->signature != MEM_LEAK_DEBUG){
						const char * errMsg;
#ifdef ENVIRONMENT32
						errMsg = "\n%s\n\nintegrity failure at page %d, header %d (%d/%d)\n\n\n";
#else
						errMsg = "\n%s\n\nintegrity failure at page %lu, header %lu (%lu/%lu)\n\n\n";
#endif
						printf(errMsg, failMessage, pages, usedSectors+freeSectors, ((ptrdiff_t)block)-firstHeaderLoc, ((ptrdiff_t)endOfThisPage)-firstHeaderLoc);
						// TODO memviewer right here...
//						printMem();
//						_getch();
						return false;
					}
					if(!block->isFree()){
						usedSectors++;
						usedBytes += block->getSize();
					}else{
						freeSectors++;
						freeBytes += block->getSize();
					}
					block = block->nextContiguousBlock();
				}while((ptrdiff_t)block < (ptrdiff_t)endOfThisPage);
				block = last;
				current = current->next;
				pages++;
			}while(current);
		}
		return true;
	}
#endif
#endif
	/**
	 * will allocate memory from the memory system
	 * @param num_bytes how many bytes are being asked for
	 * @param filename/line where, in code, the memory is requested from, used if MEM_LEAK_DEBUG defined
	 */
	void * allocate(size_t num_bytes, const char *filename, size_t line){
MEM_DEBUG_INFRASTRUCTURE
#ifdef MEM_LEAK_DEBUG
		if(num_bytes > largestRequest){
			largestRequest = num_bytes;
//			printf("largest request: %d\n", largestRequest);
		}
		if(num_bytes < smallRequestSize){
			numSmallRequests++;
		}
#endif
		// allocated memory should be size_t aligned
		size_t bytesNeeded = num_bytes;
		if((bytesNeeded & ((signed)sizeof(ptrdiff_t)-1)) != 0){
			bytesNeeded += (signed)sizeof(ptrdiff_t) - (num_bytes % sizeof(ptrdiff_t));
		}
		// which memory page is being searched
		MemPage * page = mem;
		// where this memory page ends
		//int endOfThisPage;
		// which memory block is being searched
		MemBlock * block = 0;
#ifdef MEM_LINKED_LIST
		MemBlock ** prevNextPtr;
#endif
MEM_DEBUG_INFRASTRUCTURE
		do{
			// if there is no page
			if(!page){
MEM_DEBUG_INFRASTRUCTURE
				// try to make one
				page = addPageAtLeastBigEnoughFor(bytesNeeded);
				// if there is _still_ no page
MEM_DEBUG_INFRASTRUCTURE
				if(!page){
					// fail time.
#ifdef VERIFY_INTEGRITY
					verifyIntegrity("Allocation fail");
#endif
					return 0;
				}
			}
			// if no valid block is being checed at the moment
			if(!block){
#ifdef MEM_LINKED_LIST
				prevNextPtr = &freeList;
				block = freeList;
#else
				// memory will have to be traversed from the first block at the beginning of the page
				block = page->firstBlock();
				// keep track of where the page ends
				endOfThisPage = endOfPage(page);
#endif
			}
#ifndef MEM_LINKED_LIST
			// check if the current memory block is free
			if(block->isFree()){
				// extended this free block as far as possible (till the block right after isn't free)
				MemBlock * nextNode = block->nextContiguousBlock();
				while((PTR_VAL)nextNode < endOfThisPage && nextNode->isFree()){
#ifdef MEM_LEAK_DEBUG
					if(nextNode->signature != MEM_LEAK_DEBUG){
						// memory corruption... can't traverse as expected!
						int i = 0; i = 1/i;
					}
#endif
#ifdef MEM_CLEARED_HEADER
					PTR_VAL* imem = (PTR_VAL*)nextNode;
#endif
					nextNode = nextNode->nextContiguousBlock();
#ifdef MEM_CLEARED_HEADER
					PTR_VAL numInts = sizeof(MemBlock)/sizeof(ptrdiff_t);
					for(int i = 0; i < numInts; ++i){
						imem[i]=MEM_CLEARED_HEADER;
					}
#endif
				}
				block->setSize(((PTR_VAL)nextNode-(PTR_VAL)block)-sizeof(MemBlock));
#endif
MEM_DEBUG_INFRASTRUCTURE
				// if it has enough space to be spliced into 2 blocks
				if(block->getSize() > bytesNeeded+sizeof(MemBlock))
				{
					// mark another free block where this one will end
					MemBlock * next = block->nextContiguousHeader(bytesNeeded);
					next->setSize(block->getSize() - (bytesNeeded+sizeof(MemBlock)));
					next->markFree();
#ifdef MEM_LINKED_LIST
					next->next = block->next;
#endif
#ifdef MEM_LEAK_DEBUG
					next->setupDebugInfo((const char *)0x454C4946, 0x454E494C, numAllocations++);
					//next->filename = (const char *)0x454C4946;	// little-endian 'file'
					//next->line = 0x454E494C;			// little-endian 'line'
#endif
					// and make this block exactly the size needed
					block->setSize(bytesNeeded);
#ifdef MEM_LINKED_LIST
					// maintain free list integrity
					block->next = next;
#endif
				}
MEM_DEBUG_INFRASTRUCTURE
				// if the current block has enough space for this allocation
				if(block->getSize() >= bytesNeeded){
					// grab the section of memory that is being requested
					void* allocatedMemory = block->allocatedMemory();
MEM_DEBUG_INFRASTRUCTURE
#ifdef MEM_ALLOCATED
					ptrdiff_t* imem = (ptrdiff_t*)allocatedMemory;
					size_t numints = block->getSize()/sizeof(ptrdiff_t);
					for(size_t i = 0; i < numints; ++i){
						imem[i] = MEM_ALLOCATED;
					}
MEM_DEBUG_INFRASTRUCTURE
#endif
#ifdef MEM_LEAK_DEBUG
					block->setupDebugInfo(filename, line, numAllocations++);
MEM_DEBUG_INFRASTRUCTURE
#ifdef VERIFY_INTEGRITY
					verifyIntegrity("allocation");
MEM_DEBUG_INFRASTRUCTURE
#endif
#endif
					// mark it as allocated
					block->markUsed();
MEM_DEBUG_INFRASTRUCTURE
#ifdef MEM_LINKED_LIST
					// remove this mem block from the free list
					*prevNextPtr = block->next;
					// push this on the list
					block->next = usedList;
					usedList = block;
#endif
MEM_DEBUG_INFRASTRUCTURE
					// return the memory!
					return allocatedMemory;
				}
MEM_DEBUG_INFRASTRUCTURE
#ifndef MEM_LINKED_LIST
			}
			// if the process is still going, memory has not been found yet.
			// check the next block!
			block = block->nextContiguousBlock();
			// if the "next block" in this page is out of bounds
			if((PTR_VAL)block >= endOfThisPage){
				// go to the next page
				page = page->next;
				// start checking from the beginning
				block = 0;
			}
#else
			// if there are no more free blocks
			if(!block->next){
				// add another free page, which will append a free block to this free list
				addPageAtLeastBigEnoughFor(bytesNeeded);
			}
			// hold a reference to the reference that references the next memory block
			prevNextPtr = &block->next;
			// the next memory block is now the current memory block
			block = block->next;
#endif
MEM_DEBUG_INFRASTRUCTURE
		}while(true);	// while memory hasn't been found
MEM_DEBUG_INFRASTRUCTURE
		return 0;	// should never return here.
	}

	void deallocate(void * memory){
		MemBlock * header = MemBlock::blockForAllocatedMemory(memory);//(MemBlock*)(((ptrdiff_t)memory)-sizeof(MemBlock));
		if(header->isFree())
		{
			int i=0;i= 1/i;// if this is happening, you are double-deleting memory...
		}
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
			}else{
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
		}
#endif
	}

	void reportMemory(){
		MemPage * current = mem;
		if(current){
			int pages = 0;
			int freeSectors = 0;
			int freeBytes = 0;
			int usedSectors = 0;
			int usedBytes = 0;
			do{
				MemBlock* block = current->firstBlock();
				MemBlock* endOfThisPage = (MemBlock*)endOfPage(current);
				do{
					if(!block->isFree()){
#ifdef MEM_LEAK_DEBUG
						printf("%d bytes from #%d, %s:%d\n",
							(int)block->getSize(), (int)block->allocID, block->filename, (int)block->line);
#endif
						usedSectors++;
						usedBytes += block->getSize();
					}else{
						freeSectors++;
						freeBytes += block->getSize();
					}
					block = block->nextContiguousBlock();
				}while((ptrdiff_t)block < (ptrdiff_t)endOfThisPage);
				current = current->next;
				pages++;
			}while(current);
			printf("%d pages (%d bytes of overhead)\n", (int)pages, (int)(pages*sizeof(MemPage)));
			printf("      blocks  useableBytes total\n");
			int sizeOfBlockHeader = sizeof(MemBlock);
			printf("free : %6d %12d %d\n", freeSectors, freeBytes, freeSectors*sizeOfBlockHeader+freeBytes);
			printf("used : %6d %12d %d\n", usedSectors, usedBytes, usedSectors*sizeOfBlockHeader+usedBytes);
#ifdef MEM_LEAK_DEBUG
			printf("largest request: %d\n", largestRequest);
			printf("%d requests less than %d bytes\n", numSmallRequests, smallRequestSize);
#endif
		}else{
			printf("memory clean\n");
		}
	}

	int release(){
#ifdef MEM_LEAK_DEBUG
		int leaks = 0;
#endif
		if(mem){
			MemPage * next;
//			int pagesTraversed=0, headersTraversed=0;
			do{
#ifdef MEM_LEAK_DEBUG
				MemBlock* block = mem->firstBlock();
				MemBlock* endOfThisPage = (MemBlock*)endOfPage(mem);
//				headersTraversed=0;
				do{
					if(!block->isFree()){
						printf("memory leak, %d bytes! %s:%d\n",
							(int)block->getSize(), block->filename, (int)block->line);
						leaks++;
					}
					block = block->nextContiguousBlock();
//					headersTraversed++;
				}while((ptrdiff_t)block < (ptrdiff_t)endOfThisPage);
#endif
				next = mem->next;
				free(mem);
				mem = next;
//				pagesTraversed++;
			}while(mem);
#ifdef MEM_LEAK_DEBUG
			if(leaks){
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
	~MemManager(){release();}
};

MemManager memory;

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
	MemPage * cursor = memory.mem;
	while(cursor)
	{
		start = (ptrdiff_t)cursor;
		end = start+cursor->size;
		if(thisone >= start && thisone < end){
			return end-thisone;
		}
		cursor = cursor->next;
	}
	return 0;
}


int MEM::RELEASE_MEMORY(){
	return memory.release();
}

void MEM::REPORT_MEMORY(){
	memory.reportMemory();
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
MEM_DEBUG_INFRASTRUCTURE
	void * resultMemory = memory.allocate(num_bytes, filename, line);
MEM_DEBUG_INFRASTRUCTURE
	return resultMemory;
}

void* operator new[]( size_t num_bytes, const char *filename, int line) __NEWTHROW
{
	return operator new(num_bytes, filename, line);
}

void operator delete(void* data, const char *filename, int line) throw()
{
	return memory.deallocate(data);
}
void operator delete[](void* data, const char *filename, int line) throw()
{
	return operator delete(data, filename, line);
}

void operator delete(void* data) throw()
{
	return memory.deallocate(data);
}
void operator delete[](void* data) throw()
{
	return operator delete(data);
}
#endif
