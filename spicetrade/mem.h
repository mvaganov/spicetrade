#pragma once

#define __need_ptrdiff_t // TODO remove this line? not needed in windows...
#include <stdlib.h>	// for size_t and ptrdiff_t

// TODO discover why this does not work on the linux box (?).

/**
 * Simply comment the line below to use the standard C++ new allocator.
 *
 * After changing this line's visibility, clean and recompile!
 *
 * If using the custom memory allocator, it's good practice to use
 * NEWMEM and NEWMEM_ARR for object and array allocations, and
 * NEWMEM_SOURCE_TRACE(expression) around highly tested and stable allocations
 */

// memory allocator needs some work. Windows works, but POSIX seems less forgiving with this implementaiton.
#ifdef _WIN32
#define USE_CUSTOM_MEMORY_MANAGEMENT
#endif

#ifdef USE_CUSTOM_MEMORY_MANAGEMENT

/** defined if the custom memory manager should be using debug infrastructure */
#define USE_CUSTOM_MEMORY_MANAGEMENT_DEBUG

#include <new>		// to redefine new

#ifdef _WIN32
#define __NEWTHROW	//throw(std::bad_alloc)
#define __DELTHROW	throw()
#else
#define __NEWTHROW
#define __DELTHROW
#endif

// the CPU register size changes things...
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#elif __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// max: 0xffff
#define PAGE_SIZE_DEFAULT	(32768)

#ifdef USE_CUSTOM_MEMORY_MANAGEMENT_DEBUG
// helps debug memory issues with
#define _GLIBCXX_DEBUG
#define _GLIBCXX_DEBUG_PEDANTIC

#ifdef ENVIRONMENT64
/** turns on memory debugging. enables memory leak test output. increases memory header size! */
#define MEM_LEAK_DEBUG		0x9d9285848185889b	// [HEADER]	0x5d5245444145485b
/** clears allocated memory with this character (when enabled) */
#define MEM_ALLOCATED		0x9d8d858d97858e9b	// [NEWMEM]	0x5d4d454d57454e5b
/** clears deallocated memory (that was once a header) with this character (when enabled) */
#define MEM_CLEARED_HEADER	0xfddfd4d1d5d8dffb	// [_HEAD_]	0x5d485241454c435b
/** clears deallocated memory with this character (when enabled) */
#define MEM_CLEARED			0xfddfdfdfdfdfdffb	// [CLEARM]	0x5d4d5241454c435b
#elif defined ENVIRONMENT32
/** turns on memory debugging. enables memory leak test output. increases memory header size! */
#define MEM_LEAK_DEBUG		0x84818588	// HEAD
/** clears allocated memory with this character (when enabled) */
#define MEM_ALLOCATED		0xcdd7c5ce	// NEWM
/** clears deallocated memory (that was once a header) with this character (when enabled) */
#define MEM_CLEARED_HEADER	0xfdd3d8fb	// [HD]
/** clears deallocated memory with this character (when enabled) */
#define MEM_CLEARED			0xfddfdffb	// [__]
#endif
#endif

extern const char * __NEWMEM_FILE_NAME;
extern int __NEWMEM_FILE_LINE;

#define	NEWMEM_SOURCE_DELEGATED	{if(!__NEWMEM_FILE_NAME){__NEWMEM_FILE_NAME=__FILE__;__NEWMEM_FILE_LINE=__LINE__;}}
#define	NEWMEM_SOURCE_DELEGATED_CLEAR	{__NEWMEM_FILE_NAME=0;}
#define NEWMEM_SOURCE_TRACE(EXPRESSION)	{const char *__f=__NEWMEM_FILE_NAME;int __l=__NEWMEM_FILE_LINE;if(!__NEWMEM_FILE_NAME){__NEWMEM_FILE_NAME=const_cast<const char *>(__FILE__);__NEWMEM_FILE_LINE=__LINE__;}	EXPRESSION;		if(!__NEWMEM_FILE_NAME){__NEWMEM_FILE_NAME=__f;__NEWMEM_FILE_LINE=__l;}}

#	define NEWMEM(T)	new (const_cast<const char *>(__FILE__), __LINE__) T
#	define NEWMEM_ARR(T, COUNT)	new (const_cast<const char *>(__FILE__), __LINE__) T[COUNT]
#	define DELMEM(ptr)	delete ptr
#	define DELMEM_ARR(ptr)	delete [] ptr

namespace MEM
{
	/** @return how many bytes expected to be valid beyond this memory address */
	size_t validBytesAt(void * ptr);

	/**
	 * @param ptr where to mark the stack as starting
	 * @param stackSize how much stack space to assume
	 */
	void markAsStack(void * ptr, size_t stackSize);

	/** @return shows, and returns the number of memory leaks */
	int RELEASE_MEMORY();

	/** reports current memory usage */
	void REPORT_MEMORY();

	/** @return how much memory is allocated at this valid location (if the memory manager is being used) */
	int getAllocatedHere(void * a_location);
}
#undef new
	void* operator new(size_t num_bytes) __NEWTHROW;
	void* operator new[](size_t num_bytes) __NEWTHROW;
	void* operator new(size_t num_bytes, const char *filename, int line) __NEWTHROW;
	void* operator new[](size_t num_bytes, const char *filename, int line) __NEWTHROW;

	// used in case of failed memory allocation
	void operator delete(void* data, const char *filename, int line) throw();
	void operator delete[](void* data, const char *filename, int line) throw();

	void operator delete(void* data) throw();
	void operator delete[](void* data) throw();

	void operator delete(void* data, void*) throw();

#else// the macros are just alternate routes to new/delete
#	define NEWMEM(T)	new T
#	define NEWMEM_ARR(T, COUNT)	new T[COUNT]

#	define DELMEM(ptr)	delete ptr
#	define DELMEM_ARR(ptr)	delete [] ptr

	// used to assist in memory traces for memory allocated by data structures
#	define NEWMEM_SOURCE_TRACE(EXPRESSION)	EXPRESSION

namespace MEM
{
	/** @return does not return the number of memory leaks */
	inline int RELEASE_MEMORY(){return 0;}

	/** does not report of current memory usage */
	inline void REPORT_MEMORY(){}

	/** @return how many bytes expected to be valid beyond this memory address */
	inline size_t validBytesAt(void * ptr){return 0;}

	/**
	 * @param ptr where to mark the stack as starting
	 * @param stackSize how much stack space to assume
	 */
	inline void markAsStack(void * ptr, size_t stackSize){}

	/** @return how much memory is allocated at this valid location (if the memory manager is being used) */
	inline int getAllocatedHere(void * a_location){return -1;}
}
#endif

#define DELMEM_CLEAN(ptr)		{if(ptr){DELMEM(ptr);ptr=0;}}
#define DELMEM_CLEAN_ARR(ptr)	{if(ptr){DELMEM_ARR(ptr);ptr=0;}}
