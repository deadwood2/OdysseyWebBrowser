/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef Heap_h
#define Heap_h

#include "BumpRange.h"
#include "Environment.h"
#include "LineMetadata.h"
#include "MediumChunk.h"
#include "MediumLine.h"
#include "MediumPage.h"
#include "Mutex.h"
#include "SegregatedFreeList.h"
#include "SmallChunk.h"
#include "SmallLine.h"
#include "SmallPage.h"
#include "VMHeap.h"
#include "Vector.h"
#include <array>
#include <mutex>

namespace bmalloc {

class BeginTag;
class EndTag;

class Heap {
public:
    Heap(std::lock_guard<StaticMutex>&);
    
    Environment& environment() { return m_environment; }

    void refillSmallBumpRangeCache(std::lock_guard<StaticMutex>&, size_t sizeClass, BumpRangeCache&);
    void derefSmallLine(std::lock_guard<StaticMutex>&, SmallLine*);

    void refillMediumBumpRangeCache(std::lock_guard<StaticMutex>&, size_t sizeClass, BumpRangeCache&);
    void derefMediumLine(std::lock_guard<StaticMutex>&, MediumLine*);

    void* allocateLarge(std::lock_guard<StaticMutex>&, size_t);
    void* allocateLarge(std::lock_guard<StaticMutex>&, size_t alignment, size_t, size_t unalignedSize);
    void deallocateLarge(std::lock_guard<StaticMutex>&, void*);

    void* allocateXLarge(std::lock_guard<StaticMutex>&, size_t);
    void* allocateXLarge(std::lock_guard<StaticMutex>&, size_t alignment, size_t);
    void* tryAllocateXLarge(std::lock_guard<StaticMutex>&, size_t alignment, size_t);
    Range& findXLarge(std::unique_lock<StaticMutex>&, void*);
    void deallocateXLarge(std::unique_lock<StaticMutex>&, void*);

    void scavenge(std::unique_lock<StaticMutex>&, std::chrono::milliseconds sleepDuration);

private:
    ~Heap() = delete;
    
    void initializeLineMetadata();

    SmallPage* allocateSmallPage(std::lock_guard<StaticMutex>&, size_t sizeClass);
    MediumPage* allocateMediumPage(std::lock_guard<StaticMutex>&, size_t sizeClass);

    void deallocateSmallLine(std::lock_guard<StaticMutex>&, SmallLine*);
    void deallocateMediumLine(std::lock_guard<StaticMutex>&, MediumLine*);

    void* allocateLarge(std::lock_guard<StaticMutex>&, LargeObject&, size_t);
    void deallocateLarge(std::lock_guard<StaticMutex>&, const LargeObject&);

    void splitLarge(BeginTag*, size_t, EndTag*&, Range&);
    void mergeLarge(BeginTag*&, EndTag*&, Range&);
    void mergeLargeLeft(EndTag*&, BeginTag*&, Range&, bool& inVMHeap);
    void mergeLargeRight(EndTag*&, BeginTag*&, Range&, bool& inVMHeap);
    
    void concurrentScavenge();
    void scavengeSmallPages(std::unique_lock<StaticMutex>&, std::chrono::milliseconds);
    void scavengeMediumPages(std::unique_lock<StaticMutex>&, std::chrono::milliseconds);
    void scavengeLargeObjects(std::unique_lock<StaticMutex>&, std::chrono::milliseconds);

    std::array<std::array<LineMetadata, SmallPage::lineCount>, smallMax / alignment> m_smallLineMetadata;
    std::array<std::array<LineMetadata, MediumPage::lineCount>, mediumMax / alignment> m_mediumLineMetadata;

    std::array<Vector<SmallPage*>, smallMax / alignment> m_smallPagesWithFreeLines;
    std::array<Vector<MediumPage*>, mediumMax / alignment> m_mediumPagesWithFreeLines;

    Vector<SmallPage*> m_smallPages;
    Vector<MediumPage*> m_mediumPages;

    SegregatedFreeList m_largeObjects;
    Vector<Range> m_xLargeObjects;

    bool m_isAllocatingPages;

    Environment m_environment;

    VMHeap m_vmHeap;
    AsyncTask<Heap, decltype(&Heap::concurrentScavenge)> m_scavenger;
};

inline void Heap::derefSmallLine(std::lock_guard<StaticMutex>& lock, SmallLine* line)
{
    if (!line->deref(lock))
        return;
    deallocateSmallLine(lock, line);
}

inline void Heap::derefMediumLine(std::lock_guard<StaticMutex>& lock, MediumLine* line)
{
    if (!line->deref(lock))
        return;
    deallocateMediumLine(lock, line);
}

} // namespace bmalloc

#endif // Heap_h
