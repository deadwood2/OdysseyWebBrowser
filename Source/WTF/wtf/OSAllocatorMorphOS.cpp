/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include <wtf/FastMalloc.h>
#include <OSAllocator.h>

#include <string.h>
#include <proto/exec.h>
#include <clib/debug_protos.h>
#define D(x) do { if(memory_debug) { x; } } while(0)
#define DT(x) do { if(memory_debug && getenv("OWB_DEBUG_JSC_MEMORY_STACKFRAME")) { x; } } while(0)

extern int morphos_crash(size_t);

static char* memory_debug = getenv("OWB_DEBUG_JSC_MEMORY");

namespace WTF {

char * usages[]
{
	"Unknown",
	"FastMallocPages",
	"JSGCHeapPages",
	"JSVMStackPages",
	"JSJITCodePages",
	NULL
};

int memory_consumption = 0;

char *getUsage(OSAllocator::Usage usage)
{
	switch(usage)
	{
		default:
		case OSAllocator::UnknownUsage:
			return usages[0];
		case OSAllocator::FastMallocPages:
			return usages[1];
		case OSAllocator::JSGCHeapPages:
			return usages[2];
		case OSAllocator::JSVMStackPages:
			return usages[3];
		case OSAllocator::JSJITCodePages:
			return usages[4];
	}
	
	return NULL;
}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage usage, bool, bool, bool)
{
	ULONG *mem = NULL;
	
	//D(kprintf("OSAllocator::reserveUncommitted(%lu)\n", bytes));
	
	bytes += 12;
	
	mem = (ULONG *) AllocVecTaskPooled(bytes);
	
	if (mem)
	{
		*mem++ = bytes;
		if ((ULONG)mem & 4)
			*mem++ = 0;
		*mem++ = (ULONG) usage;
	}
	
	D(kprintf("%p -> [%s] [%lu bytes] OSAllocator::reserveUncommitted(%lu) [allocated %lu]\n", mem, getUsage(usage), bytes, bytes, memory_consumption));
	DT(DumpTaskState(FindTask(NULL)));

	memory_consumption += bytes;
	
	if(mem == NULL)
	{
		morphos_crash(bytes);
	}
	
	return mem;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage usage, bool a, bool b, bool c)
{
	//D(kprintf("OSAllocator::reserveAndCommit(%lu)\n", bytes));
	return reserveUncommitted(bytes, usage, a, b, c);
}

void OSAllocator::commit(void* address, size_t bytes, bool, bool)
{
	//D(kprintf("OSAllocator::commit(%p, %lu)\n", address, bytes));
}

void OSAllocator::decommit(void* address, size_t bytes)
{
	//D(kprintf("OSAllocator::decommit(%p, %lu)\n", address, bytes));
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
	if(address)
	{
		ULONG *mem = (ULONG *) address;
		ULONG size;
		Usage usage;

		mem--;
		usage = (Usage) *mem;
		size = *--mem;
		
		if (size == 0)
			size = *--mem;

		memory_consumption -= size;
	
		D(kprintf("%p <- [%s]  [%lu bytes] OSAllocator::releaseDecommitted(%p, %lu) [allocated %lu]\n", address, getUsage(usage), size, address, bytes, memory_consumption));
		DT(DumpTaskState(FindTask(NULL)));

		FreeVecTaskPooled(mem);
	}
}

} // namespace WTF
