/*
 * Copyright (C) 2015 Krzysztof Smiechowicz. All rights reserved.
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

#include "execallocator.h"
#include "arosbailout.h"

#include <proto/alib.h>
#include <exec/lists.h>
#include <aros/debug.h>
#include <stdlib.h>
#define DebugBase s_debugBase
static struct Library *s_debugBase;
#include <proto/debug.h>
#include <libraries/debug.h>
#include <proto/exec.h>

#undef PAGESIZE
#define PAGESIZE                (4096)
#define ALIGN(val, align)       (((IPTR)val + (IPTR)align - 1) & (~((IPTR)align - 1)))

static const ULONG BLOCKSIZES[] = {1024, 512, 256, 128, 64}; /* In pages */
static const ULONG BLOCKSIZESCOUNT = sizeof(BLOCKSIZES)/sizeof(BLOCKSIZES[0]);

class PageAllocator
{
public:
    PageAllocator();
    ~PageAllocator();
    void * getPages(size_t count, bool executable, bool mayFail = false);
    void * getPages(size_t count, size_t alignment, bool executable, bool mayFail = false);
    bool freePages(void * address, size_t count, void * caller);
    bool freePages(void * address, void * caller);
    size_t freePagesInBlockOf(void * address);   /* self-test only */
    size_t pagesToBlockEnd(void * address);      /* self-test only */
    int getAllocatedPagesCount();

private:
    struct List blocks;
    struct SignalSemaphore lock;

    struct PageBlock
    {
        struct Node pb_Node;
        IPTR        pb_AllocAddress;
        IPTR        pb_StartAddress;
        IPTR        pb_EndAddress;
        ULONG       pb_Alignment;
        ULONG       pb_Flags;
        WORD        *pb_PagesBitMap; /* 0 - free, -1 allocated, > 0 allocation size (only first page) */
        ULONG       pb_FreePages;
        ULONG       pb_TotalPages;
    };

    void * allocatePagesFromBlock(PageAllocator::PageBlock * block, size_t count, size_t alignment);
    PageAllocator::PageBlock * allocateNewBlock(size_t count, ULONG flags, bool mayFail);
    void freeBlock(PageAllocator::PageBlock * block);
    PageAllocator::PageBlock * findBlock(IPTR addr);
    void reportBlockUsage();
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

void PageAllocator::reportBlockUsage()
{
    void * n, *n1;
    ULONG totalpages = 0;

    bug("PageAllocator block usage\n");
    bug("-------------------------\n");
    ForeachNodeSafe(&blocks, n, n1)
    {
        PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;
        bug("Block: Size %d KB, Alignment %d KB, %%%d Available\n",
                block->pb_TotalPages * PAGESIZE / 1024,
                block->pb_Alignment / 1024,
                block->pb_FreePages * 100 / block->pb_TotalPages);
        totalpages += block->pb_TotalPages;
    }
    bug("Total allocated size: %d KB\n", totalpages * PAGESIZE / 1024);
    bug("-------------------------\n");
}

PageAllocator::PageBlock * PageAllocator::allocateNewBlock(size_t count, ULONG flags, bool mayFail)
{
    PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)AllocMem(sizeof(PageAllocator::PageBlock), MEMF_ANY);
    if (!block)
        return NULL;

    block->pb_Alignment = PAGESIZE;
    block->pb_Flags = flags;

    void * memoryblock = NULL;
    IPTR allocationsize = 0;

retry:
    ULONG i = 0;
    /* Try to allocation a range of decreasing block sizes */
    while (memoryblock == NULL && i < BLOCKSIZESCOUNT)
    {
        allocationsize = (BLOCKSIZES[i] > count ? BLOCKSIZES[i] : count) * PAGESIZE;
        memoryblock = AllocMem(allocationsize + block->pb_Alignment, block->pb_Flags);
        i++;
    }

    if (!memoryblock)
    {
        /* Callers of the try* allocators handle a null result themselves
         * (JSC shrinks its structure heap, the JIT pool is optional): no
         * requester for them. */
        if (mayFail)
        {
            FreeMem(block, sizeof(PageAllocator::PageBlock));
            return NULL;
        }

        if (aros_memory_allocation_error(allocationsize, block->pb_Alignment) == 2)
            return NULL; /* quit */

        goto retry; /* retry */
    }

    block->pb_TotalPages = allocationsize / PAGESIZE;
    block->pb_FreePages = allocationsize / PAGESIZE;

    block->pb_AllocAddress = (IPTR)memoryblock;
    block->pb_StartAddress = (IPTR)ALIGN(block->pb_AllocAddress, block->pb_Alignment);
    block->pb_EndAddress = block->pb_StartAddress + allocationsize;
    block->pb_PagesBitMap = (WORD *)AllocMem(block->pb_TotalPages * sizeof(WORD), MEMF_ANY | MEMF_CLEAR);

    AddTail(&blocks, (struct Node *)block);

    D(bug("Adding page block 0x%lx, %d\n", block->pb_StartAddress, block->pb_TotalPages));
    D(reportBlockUsage());
    return block;
}

void PageAllocator::freeBlock(PageAllocator::PageBlock * block)
{

    IPTR allocationsize = block->pb_TotalPages * PAGESIZE;

    D(bug("Removing page block 0x%x, %d\n", block->pb_StartAddress, block->pb_TotalPages));

    Remove((struct Node *)block);

    FreeMem((APTR)block->pb_AllocAddress, allocationsize + block->pb_Alignment);
    FreeMem(block->pb_PagesBitMap, block->pb_TotalPages * sizeof(WORD));
    FreeMem(block, sizeof(PageAllocator::PageBlock));
    D(reportBlockUsage());
}

void * PageAllocator::allocatePagesFromBlock(PageAllocator::PageBlock * block,
        size_t count, size_t alignment)
{
    const ULONG increment = alignment / PAGESIZE; /* Jump over that many pages */
    IPTR alignedblockaddress;
    ULONG i;

    if (!block)
        return NULL;

    if (block->pb_FreePages < count)
        return NULL;

    /* Calculate initial index:
     * The resulting address at i must be aligned to alignment */

    alignedblockaddress = ALIGN(block->pb_StartAddress, alignment);
    i = (alignedblockaddress - (block->pb_StartAddress)) / PAGESIZE;

    /* i increments to point to next aligned address */
    for (; i < block->pb_TotalPages; i += increment)
    {
        if (block->pb_PagesBitMap[i] == 0)
        {
            bool spacefound = true;
            size_t j = 0;
            /* Check for continous space */
            for (; (j < count && (i + j) < block->pb_TotalPages); j++)
            {
                if (block->pb_PagesBitMap[i + j] != 0)
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
                block->pb_PagesBitMap[i] = count; /* Store allocation size */
                for (size_t k = 1; k < count; k++)
                {
                    block->pb_PagesBitMap[i + k] = -1;
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

void * PageAllocator::getPages(size_t count, bool executable, bool mayFail)
{
    return getPages(count, PAGESIZE, executable, mayFail);
}

void * PageAllocator::getPages(size_t count, size_t alignment, bool executable, bool mayFail)
{
    void * n; void * _return = NULL;
    ULONG flags = MEMF_ANY;

    /* The allocation size lives in the head page's WORD of the bitmap. */
    if (count >= 0x8000)
        return NULL;

    if (executable) flags = MEMF_EXECUTABLE;

    ObtainSemaphore(&lock);

    ForeachNode(&blocks, n)
    {
        PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;

        if (block->pb_Flags != flags)
            continue;

        if (block->pb_FreePages < count)
            continue;

        /* Found block with enough free pages, let's check if they are continous */
        _return = allocatePagesFromBlock(block, count, alignment);
        if (_return != NULL)
        {
            ReleaseSemaphore(&lock);
            return _return;
        }
    }

    /* If we are here, it means none of the blocks was big enough. A new
     * block starts at an arbitrary page, so for an alignment above one page
     * it needs room for an aligned run of count pages: up to
     * alignment / PAGESIZE - 1 pages before it (they stay usable for other
     * allocations). JSC's structure heap needs 32 MB aligned to 32 MB. */
    size_t needed = count;
    if (alignment > PAGESIZE)
        needed += alignment / PAGESIZE - 1;
    PageAllocator::PageBlock * block = allocateNewBlock(needed, flags, mayFail);
    if (block)
        _return = allocatePagesFromBlock(block, count, alignment);
    ReleaseSemaphore(&lock);
    return _return;
}

/* Freeing contract (AROS port):
 * - freePages(address, count): every page in [address, address + count pages)
 *   must be inside one block and currently allocated. A sub-range of an
 *   allocation is legal (partial release: X = getPages(A); freePages(X + Z, A - Z);
 *   freePages(X, Z)).
 * - freePages(address): address must be the first page of a live allocation;
 *   its stored size is released, stopping at pages already released.
 * A call that breaks the contract (unaligned address, address in no block,
 * range beyond its block, a page that is not allocated, address that is not
 * an allocation start) changes nothing and is reported. Unchecked, such calls
 * write past pb_PagesBitMap or over-count pb_FreePages, so freeBlock() would
 * FreeMem() a block still in use.
 *
 * Reports are made after the allocator semaphore is released: under the lock
 * only an Anomaly record is filled in (no allocation, no library calls). */
struct Anomaly
{
    const char *what;
    IPTR addr;
    size_t count;
    void *caller;
};

static int s_anomalies;
static void reportAnomaly(const Anomaly &a)
{
    if (!a.what || s_anomalies++ >= 64)
        return;
    STRPTR module = NULL, symbol = NULL;
    APTR symbolStart = NULL;
    if (!s_debugBase)
        s_debugBase = OpenLibrary((CONST_STRPTR)"debug.library", 1);
    if (s_debugBase)
        DecodeLocation(a.caller, DL_ModuleName, (IPTR)&module, DL_SymbolName, (IPTR)&symbol,
                       DL_SymbolStart, (IPTR)&symbolStart, TAG_DONE);
    bug("[ExecAllocator] %s: address 0x%p pages %lu caller 0x%p %s %s+0x%lx (anomaly %d)\n",
        a.what, (APTR)a.addr, (unsigned long)a.count, a.caller, module ? (char *)module : "?",
        symbol ? (char *)symbol : "?", (unsigned long)((IPTR)a.caller - (IPTR)symbolStart), s_anomalies);
}

PageAllocator::PageBlock * PageAllocator::findBlock(IPTR addr)
{
    void * n;
    ForeachNode(&blocks, n)
    {
        PageAllocator::PageBlock * block = (PageAllocator::PageBlock *)n;
        if (addr >= block->pb_StartAddress && addr < block->pb_EndAddress)
            return block;
    }
    return NULL;
}

bool PageAllocator::freePages(void * address, size_t count, void * caller)
{
    IPTR addr = (IPTR)address;
    Anomaly anomaly = { NULL, addr, count, caller };
    bool accepted = false;

    ObtainSemaphore(&lock);
    PageAllocator::PageBlock * block = findBlock(addr);
    if (!block)
        anomaly.what = "free of an address in no block";
    else
    {
        IPTR relative = addr - block->pb_StartAddress;
        size_t pos = relative / PAGESIZE;         /* < pb_TotalPages: addr < pb_EndAddress */
        size_t available = block->pb_TotalPages - pos;
        if (relative % PAGESIZE)
            anomaly.what = "free of an unaligned address";
        else if (count > available)                /* no pos + count: it can wrap */
            anomaly.what = "free beyond the end of its block";
        else
        {
            size_t notAllocated = 0;
            for (size_t i = 0; i < count; i++)
                if (block->pb_PagesBitMap[pos + i] == 0)
                    notAllocated++;
            if (notAllocated)
            {
                anomaly.what = "free of pages that are not allocated (double free?)";
                anomaly.count = notAllocated;
            }
            else
            {
                for (size_t i = 0; i < count; i++)
                    block->pb_PagesBitMap[pos + i] = 0;
                block->pb_FreePages += count;
                accepted = true;
                if (block->pb_FreePages == block->pb_TotalPages)
                    freeBlock(block);
            }
        }
    }
    ReleaseSemaphore(&lock);

    reportAnomaly(anomaly);
    return accepted;
}

bool PageAllocator::freePages(void * address, void * caller)
{
    IPTR addr = (IPTR)address;
    Anomaly anomaly = { NULL, addr, 0, caller };
    bool accepted = false;

    ObtainSemaphore(&lock);
    PageAllocator::PageBlock * block = findBlock(addr);
    if (!block)
        anomaly.what = "free of an address in no block";
    else
    {
        IPTR relative = addr - block->pb_StartAddress;
        size_t pos = relative / PAGESIZE;
        if (relative % PAGESIZE)
            anomaly.what = "free of an unaligned address";
        else if (block->pb_PagesBitMap[pos] == 0)
            anomaly.what = "double free (allocation already free)";
        else if (block->pb_PagesBitMap[pos] < 0)
            anomaly.what = "free of an address inside an allocation";
        else
        {
            size_t count = (size_t)block->pb_PagesBitMap[pos];
            size_t available = block->pb_TotalPages - pos;
            if (count > available)
                count = available;
            /* Head page plus its continuation pages (-1); stop at a released
             * page or another allocation's head (stale size after a legal
             * partial release). */
            size_t freed = 1;
            block->pb_PagesBitMap[pos] = 0;
            while (freed < count && block->pb_PagesBitMap[pos + freed] == -1)
                block->pb_PagesBitMap[pos + freed++] = 0;
            block->pb_FreePages += freed;
            accepted = true;
            if (block->pb_FreePages == block->pb_TotalPages)
                freeBlock(block);
        }
    }
    ReleaseSemaphore(&lock);

    reportAnomaly(anomaly);
    return accepted;
}

size_t PageAllocator::freePagesInBlockOf(void * address)
{
    ObtainSemaphore(&lock);
    PageAllocator::PageBlock * block = findBlock((IPTR)address);
    size_t n = block ? block->pb_FreePages : (size_t)-1;
    ReleaseSemaphore(&lock);
    return n;
}

size_t PageAllocator::pagesToBlockEnd(void * address)
{
    ObtainSemaphore(&lock);
    PageAllocator::PageBlock * block = findBlock((IPTR)address);
    size_t n = block ? (block->pb_EndAddress - (IPTR)address) / PAGESIZE : 0;
    ReleaseSemaphore(&lock);
    return n;
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

/*************************************************************************************************/

static inline int getPageCount(size_t bytes)
{
    /* Round up size to next page size */
    return (int)((bytes + PAGESIZE - 1) / PAGESIZE);
}

/* OWB_ALLOC_SELFTEST: once, at the first allocation, exercise the freeing
 * contract on a private 8-page allocation and print PASS/FAIL per case. The
 * anomaly reports expected from the invalid cases follow each line. */
static void selfTestOnce()
{
    static int done;
    if (done)
        return;
    done = 1;
    if (!getenv("OWB_ALLOC_SELFTEST"))
        return;
    void *caller = __builtin_return_address(0);
    char *p = (char *)allocator.getPages(8, false);
    size_t before = allocator.freePagesInBlockOf(p);
    int fails = 0;
#define CHECK(name, cond) do { bool ok_ = (cond); fails += !ok_; bug("[ExecAllocator] self-test %s: %s\n", ok_ ? "PASS" : "FAIL", name); } while (0)
    CHECK("unaligned address rejected", !allocator.freePages(p + 100, 1, caller) && allocator.freePagesInBlockOf(p) == before);
    CHECK("range beyond the block rejected", !allocator.freePages(p + 6 * PAGESIZE, allocator.pagesToBlockEnd(p + 6 * PAGESIZE) + 1, caller) && allocator.freePagesInBlockOf(p) == before);
    CHECK("huge count rejected (no wrap)", !allocator.freePages(p + 6 * PAGESIZE, (size_t)-1, caller) && allocator.freePagesInBlockOf(p) == before);
    CHECK("legal partial free of the tail", allocator.freePages(p + 6 * PAGESIZE, 2, caller) && allocator.freePagesInBlockOf(p) == before + 2);
    CHECK("double free of the tail rejected", !allocator.freePages(p + 6 * PAGESIZE, 2, caller) && allocator.freePagesInBlockOf(p) == before + 2);
    CHECK("address inside the allocation rejected", !allocator.freePages(p + 2 * PAGESIZE, caller) && allocator.freePagesInBlockOf(p) == before + 2);
    size_t mid = allocator.freePagesInBlockOf(p);
    bool headOk = allocator.freePages(p, caller);
    size_t after = allocator.freePagesInBlockOf(p);
    CHECK("free by stored size after partial free releases 6 pages", headOk && (after == mid + 6 || after == (size_t)-1));
    CHECK("double free by stored size rejected", !allocator.freePages(p, caller));
#undef CHECK
    bug("[ExecAllocator] self-test done: %d failure(s)\n", fails);
}

void * allocator_getmem_page_aligned(size_t bytes, bool executable)
{
    selfTestOnce();
    int pagecount = getPageCount(bytes);
    void * ptr = allocator.getPages(pagecount, executable);

    D(bug("A:getmem_page_aligned 0x%x -> pagecount %d \n", ptr, pagecount));

    if(ptr)
        memset(ptr, 0, bytes);

    D(reservedAdd(bytes, u););


    return ptr;
}

/* As allocator_getmem_page_aligned() and allocator_getmem_aligned(), but a
 * failed AllocMem returns NULL instead of opening the retry/quit requester. */
void * allocator_getmem_page_aligned_try(size_t bytes, bool executable)
{
    selfTestOnce();
    void * ptr = allocator.getPages(getPageCount(bytes), executable, true);
    if (ptr)
        memset(ptr, 0, bytes);
    return ptr;
}

void * allocator_getmem_aligned_try(size_t bytes, size_t alignment)
{
    selfTestOnce();
    if (alignment < PAGESIZE) alignment = PAGESIZE;
    void * ptr = allocator.getPages(getPageCount(bytes), alignment, false, true);
    if (ptr)
        memset(ptr, 0, bytes);
    return ptr;
}

void * allocator_getmem_aligned(size_t bytes, size_t alignment)
{
    selfTestOnce();
    D(bug("A:getmem_aligned bytes %d, alignment %d \n", bytes, alignment));

    /* Can only allocate full pages */
    if (alignment < PAGESIZE) alignment = PAGESIZE;

    int pagecount = getPageCount(bytes);
    void * ptr = allocator.getPages(pagecount, alignment, false);

    if(ptr)
        memset(ptr, 0, bytes);

    return ptr;
}

void allocator_freemem(void * address, size_t bytes)
{
    D(bug("A:freemem 0x%x bytes %d \n", address, bytes));

    int pagecount = getPageCount(bytes);
    allocator.freePages(address, pagecount, __builtin_return_address(0));
}

void allocator_freemem_from(void * address, size_t bytes, void * caller)
{
    allocator.freePages(address, getPageCount(bytes), caller);
}

void allocator_freemem_from(void * address, void * caller)
{
    if (address)
        allocator.freePages(address, caller);
}

void allocator_freemem(void * address)
{
    D(bug("A:freemem 0x%x \n", address));

    if (address)
        allocator.freePages(address, __builtin_return_address(0));
}
