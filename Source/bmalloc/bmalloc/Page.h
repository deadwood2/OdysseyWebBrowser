/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef Page_h
#define Page_h

#include "BAssert.h"
#include "Mutex.h"
#include "VMAllocate.h"
#include <mutex>

namespace bmalloc {

template<typename Traits>
class Page {
public:
    typedef typename Traits::ChunkType Chunk;
    typedef typename Traits::LineType Line;

    static const size_t lineSize = Traits::lineSize;
    static const size_t lineCount = vmPageSize / lineSize;

    static const unsigned char maxRefCount = std::numeric_limits<unsigned char>::max();
    static_assert(lineCount < maxRefCount, "maximum line count must fit in Page");
    
    static Page* get(Line*);

    void ref(std::lock_guard<StaticMutex>&);
    bool deref(std::lock_guard<StaticMutex>&);
    unsigned refCount(std::lock_guard<StaticMutex>&) { return m_refCount; }
    
    size_t sizeClass() { return m_sizeClass; }
    void setSizeClass(size_t sizeClass) { m_sizeClass = sizeClass; }
    
    Line* begin();
    Line* end();

private:
    unsigned char m_refCount;
    unsigned char m_sizeClass;
};

template<typename Traits>
inline void Page<Traits>::ref(std::lock_guard<StaticMutex>&)
{
    BASSERT(m_refCount < maxRefCount);
    ++m_refCount;
}

template<typename Traits>
inline bool Page<Traits>::deref(std::lock_guard<StaticMutex>&)
{
    BASSERT(m_refCount);
    --m_refCount;
    return !m_refCount;
}

template<typename Traits>
inline auto Page<Traits>::get(Line* line) -> Page*
{
    Chunk* chunk = Chunk::get(line);
    size_t lineNumber = line - chunk->lines();
    size_t pageNumber = lineNumber * lineSize / vmPageSize;
    return &chunk->pages()[pageNumber];
}

template<typename Traits>
inline auto Page<Traits>::begin() -> Line*
{
    Chunk* chunk = Chunk::get(this);
    size_t pageNumber = this - chunk->pages();
    size_t lineNumber = pageNumber * lineCount;
    return &chunk->lines()[lineNumber];
}

template<typename Traits>
inline auto Page<Traits>::end() -> Line*
{
    return begin() + lineCount;
}

} // namespace bmalloc

#endif // Page_h
