/*
 * Copyright (C) 2012 Krzysztof Smiechowicz. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "OSAllocator.h"

#include <exec/lists.h>
#include <string.h>
#include <aros/debug.h>

#define EXECALLOCATOR   1
#define OWNALLOCATOR    0

#if EXECALLOCATOR
#include <bmalloc/execallocator.h>
#endif

namespace WTF {

#if OWNALLOCATOR

#undef PAGESIZE
#define PAGESIZE                (4096)
#define ALIGN(val, align)       ((val + align - 1) & (~(align - 1)))
#define DEFAULTBLOCKSIZE        (512)

class PageAllocator
{
private:
    struct List blocks;
    struct SignalSemaphore lock;

    struct PageBlock
    {
        struct Node pb_Node;
        IPTR        pb_StartAddress;
        IPTR        pb_EndAddress;
        BYTE        *pb_PagesBitMap;
        LONG        pb_FreePages;
        LONG        pb_TotalPages;
    };

    void * allocatePagesFromBlock(PageAllocator::PageBlock * block, int count);
    PageAllocator::PageBlock * allocateNewBlock(int size);
    void freeBlock(PageAllocator::PageBlock * block);
public:
    PageAllocator();
    ~PageAllocator();
    void * getPages(int count);
    void freePages(void * address, int count);
    int getAllocatedPagesCount();
};

PageAllocator::PageAllocator()
{
    NEWLIST(&blocks);
    InitSemaphore(&lock);
}

PageAllocator::~PageAllocator()
{
    void * n, *n1;

    ForeachNodeSafe(&blocks, n, n1)
    {
        PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;
        freeBlock(block);
    }
}

PageAllocator::PageBlock * PageAllocator::allocateNewBlock(int size)
{
    PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)AllocMem(sizeof(PageAllocator::PageBlock), MEMF_ANY);

    block->pb_TotalPages = size;
    block->pb_FreePages = size;

    int allocationsize = (block->pb_TotalPages + 1) * PAGESIZE;
    void * memoryblock = AllocMem(allocationsize, MEMF_ANY);

    block->pb_StartAddress = (IPTR)memoryblock;
    block->pb_EndAddress = (IPTR)memoryblock + (allocationsize);
    block->pb_PagesBitMap = (BYTE *)AllocMem(block->pb_TotalPages, MEMF_ANY | MEMF_CLEAR);

    AddTail(&blocks, (struct Node *)block);

    D(bug("Adding page block 0x%x, %d\n", block->pb_StartAddress, block->pb_TotalPages));
    return block;
}

void PageAllocator::freeBlock(PageAllocator::PageBlock * block)
{

    int allocationsize = (block->pb_TotalPages + 1) * PAGESIZE;

    D(bug("Removing page block 0x%x, %d\n", block->pb_StartAddress, block->pb_TotalPages));

    Remove((struct Node *)block);

    FreeMem((APTR)block->pb_StartAddress, allocationsize);
    FreeMem(block->pb_PagesBitMap, block->pb_TotalPages);
    FreeMem(block, sizeof(PageAllocator::PageBlock));
}

void * PageAllocator::allocatePagesFromBlock(PageAllocator::PageBlock * block, int count)
{
    if (!block)
        return NULL;

    if (block->pb_FreePages < count)
        return NULL;

    for (int i = 0; i < block->pb_TotalPages; i++)
    {
        if (block->pb_PagesBitMap[i] == 0)
        {
            bool spacefound = true;
            int j = 0;
            /* Check for continous space */
            for (; (j < count && (i + j) < block->pb_TotalPages); j++)
            {
                if (block->pb_PagesBitMap[i + j] == 1)
                {
                    spacefound = false;
                    break;
                }
            }

            if (j != count) /* We exited before count of pages could be checked */
            {
                spacefound = false;
            }

            if (spacefound)
            {
                /* Marks as used */
                for (int k = 0; k < count; k++)
                {
                    block->pb_PagesBitMap[i + k] = 1;
                }

                block->pb_FreePages -= count;

                D
                (
                IPTR pagesstart = ALIGN((block->pb_StartAddress + (i * PAGESIZE)), PAGESIZE);
                IPTR pagesend = ALIGN((block->pb_StartAddress + ((i + count) * PAGESIZE)), PAGESIZE);

                bug("ALLOC block (0x%x, 0x%x) pages(0x%x, 0x%x)\n", block->pb_StartAddress, block->pb_EndAddress, pagesstart, pagesend);
                );

                return (void *)ALIGN((block->pb_StartAddress + (i * PAGESIZE)), PAGESIZE);
            }
        }
    }

    return NULL;
}

void * PageAllocator::getPages(int count)
{
    void * n; void * _return = NULL;

    ObtainSemaphore(&lock);

    ForeachNode(&blocks, n)
    {
        PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;
        if (block->pb_FreePages < count)
            continue;

        /* Found block with enough free pages, let's check if they are continous */
        _return = allocatePagesFromBlock(block, count);
        if (_return != NULL)
        {
            ReleaseSemaphore(&lock);
            return _return;
        }
    }

    /* If we are here, it means none of the blocks was big enough */
    PageAllocator::PageBlock * block = allocateNewBlock((count > DEFAULTBLOCKSIZE) ? count : DEFAULTBLOCKSIZE);
    _return = allocatePagesFromBlock(block, count);
    ReleaseSemaphore(&lock);
    return _return;
}

void PageAllocator::freePages(void * address, int count)
{
   void * n; IPTR addr = (IPTR)address;

   ObtainSemaphore(&lock);

   ForeachNode(&blocks, n)
   {
       PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;
       if (addr >= block->pb_StartAddress && addr <= block->pb_EndAddress)
       {
           IPTR relative = addr - block->pb_StartAddress;
           int pos = relative / PAGESIZE;
           for (int i = 0; i < count; i++)
               block->pb_PagesBitMap[i + pos] = 0;
           block->pb_FreePages += count;

           /* Check if block can be freed */
           if (block->pb_FreePages == block->pb_TotalPages)
               freeBlock(block);

           ReleaseSemaphore(&lock);
           return;
       }
   }

   bug("Address 0x%x not part of any block!", addr);
   ReleaseSemaphore(&lock);
}

int PageAllocator::getAllocatedPagesCount()
{
    void * n; int count = 0;

    ObtainSemaphore(&lock);

    ForeachNode(&blocks, n)
    {
        PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;
        count += block->pb_TotalPages;
    }

    ReleaseSemaphore(&lock);

    return count;
}

static PageAllocator allocator;
#endif

/*************************************************************************************************/
/* Debug code */
D(
static int reserved[5] = {0};
static int opcount = 0;

static void showStats()
{
    opcount++;
    if (opcount % 250 == 0)
    {
        bug("UnknownPages - Reserved: %d\n", reserved[0]);
        bug("FastMallocPages - Reserved: %d\n", reserved[1]);
        bug("JSGCHeapPages - Reserved: %d\n", reserved[2]);
        bug("JSJITCodePages - Reserved: %d\n", reserved[3]);
        bug("JSVMStackPages - Reserved: %d\n", reserved[4]);
        bug("Total reserved: %d\n", (allocator.getAllocatedPagesCount() * 4096));
    }
}

static void reservedAdd(size_t bytes, OSAllocator::Usage u)
{
    int i = (int)u;

    if (i < 0) i = 0;

    reserved[i] += bytes;

    showStats();
}
);

/*************************************************************************************************/

#if OWNALLOCATOR
static inline int getPageCount(size_t bytes)
{
    /* Round up size to next page size */
    return (int)((bytes + PAGESIZE - 1) / PAGESIZE);
}
#endif

#if EXECALLOCATOR
#define likely(x)   __builtin_expect(!!(x), 1)

extern "C"
{
void aros_bailout_jump();
int  aros_is_memory_bailout();
}

static void* allocateWithCheck(size_t bytes)
{
    void * ptr = bmalloc::allocator_getmem_page_aligned(bytes);
    if (likely(ptr))
        return ptr;

    if (aros_is_memory_bailout())
        aros_bailout_jump();

    return nullptr;
}
#endif

void* OSAllocator::reserveUncommitted(size_t bytes, Usage u, bool, bool, bool)
{
    (void)u;
#if OWNALLOCATOR
    int pagecount = getPageCount(bytes);
    void * ptr = allocator.getPages(pagecount);

    D(bug("ALLOC 0x%x -> pagecount %d \n", ptr, pagecount));

    D(reservedAdd(bytes, u););

    return ptr;
#endif
#if EXECALLOCATOR
    return allocateWithCheck(bytes);
#endif
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage u, bool, bool, bool)
{
    (void)u;
#if OWNALLOCATOR
    int pagecount = getPageCount(bytes);
    void * ptr = allocator.getPages(pagecount);

    D(bug("ALLOC&C 0x%x -> pagecount %d \n", ptr, pagecount));

    if(ptr)
        memset(ptr, 0, bytes);

    D(reservedAdd(bytes, u););

    return ptr;
#endif
#if EXECALLOCATOR
    return allocateWithCheck(bytes);
#endif
}

void OSAllocator::commit(void* address, size_t bytes, bool, bool)
{
    memset(address, 0, bytes);
}

void OSAllocator::decommit(void* address, size_t bytes)
{
    (void)address;
    (void)bytes;
    D
    (
    int pagecount = getPageCount(bytes);
    bug("DECOMMIT 0x%x -> pagecount %d \n", address, pagecount);
    )
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
#if OWNALLOCATOR
    int pagecount = getPageCount(bytes);
    D(bug("DEALLOC 0x%x -> pagecount %d \n", address, pagecount));

    allocator.freePages(address, pagecount);
#endif
#if EXECALLOCATOR
    bmalloc::allocator_freemem(address, bytes);
#endif
}

} // namespace WTF
