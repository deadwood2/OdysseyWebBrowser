/*
 * Copyright (C) 2009 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this deque of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this deque of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TransferSharedPtr_h
#define TransferSharedPtr_h

template<typename T> class SharedPtr;
template<typename T> class TransferSharedPtr;

template<typename T> class TransferSharedPtr {
public:
    TransferSharedPtr() : m_ptr(0) {}
    TransferSharedPtr(T* ptr) : m_ptr(ptr) { if (ptr) ptr->ref(); }
    TransferSharedPtr(const TransferSharedPtr& o) : m_ptr(o.releaseRef()) {}
    TransferSharedPtr(const SharedPtr<T>& o) : m_ptr(o.get()) { if (T* ptr = m_ptr) ptr->ref(); }
    template <typename U> TransferSharedPtr(const TransferSharedPtr<U>& o) : m_ptr(o.releaseRef()) { }
    template <typename U> TransferSharedPtr(const SharedPtr<U>& o) : m_ptr(o.get()) { if (T* ptr = m_ptr) ptr->ref(); }

    ~TransferSharedPtr() { if (m_ptr) m_ptr->deref(); }

    T* get() const { return m_ptr; }

    T* releaseRef() const { T* tmp = m_ptr; m_ptr = 0; return tmp; }

    // operator* and operator& are disabled because of the STL
    T* operator->() const { return m_ptr; }

    bool operator!() const { return !m_ptr; }

    operator bool() const { return m_ptr; }

    TransferSharedPtr& operator=(T* optr)
    {
       if (optr)
           optr->ref();
        T* ptr = m_ptr;
        m_ptr = optr;
        if (ptr)
            ptr->deref();
        return *this; 
    }

    TransferSharedPtr& operator=(const TransferSharedPtr& o)
    {
        T* ptr = m_ptr;
        m_ptr = o.releaseRef();
        if (ptr)
            ptr->deref();
        return *this;
    }

    template <typename U> TransferSharedPtr& operator=(const TransferSharedPtr<U>& o)
    {
        T* ptr = m_ptr;
        m_ptr = o.releaseRef();
        if (ptr)
            ptr->deref();
        return *this;
    }

    template <typename U> TransferSharedPtr& operator=(const SharedPtr<U>& o)
    {
        T* optr = o.get();
        if (optr)
            optr->ref();
        T* ptr = m_ptr;
        m_ptr = optr;
        if (ptr)
            ptr->deref();
        return *this;
    }

private:
    mutable T* m_ptr;
};

template <typename T> inline bool operator==(const TransferSharedPtr<T>& a, const TransferSharedPtr<T>& b)
{
    return a.get() == b.get();
}

template <typename T> inline bool operator==(const TransferSharedPtr<T>& a, const SharedPtr<T>& b)
{
    return a.get() == b.get();
}

template <typename T> inline bool operator==(const SharedPtr<T>& a, const TransferSharedPtr<T>& b)
{
    return a.get() == b.get();
}

template <typename T> inline bool operator==(const TransferSharedPtr<T>& a, T* b)
{
    return a.get() == b;
}

template <typename T> inline bool operator==(T* a, const TransferSharedPtr<T>& b)
{
    return a == b.get();
}

template <typename T> inline bool operator!=(const TransferSharedPtr<T>& a, const TransferSharedPtr<T>& b)
{
    return a.get() != b.get();
}

template <typename T> inline bool operator!=(const TransferSharedPtr<T>& a, const SharedPtr<T>& b)
{
    return a.get() != b.get();
}

template <typename T> inline bool operator!=(const SharedPtr<T>& a, const TransferSharedPtr<T>& b)
{
    return a.get() != b.get();
}

template <typename T> inline bool operator!=(const TransferSharedPtr<T>& a, T* b)
{
    return a.get() != b;
}

template <typename T> inline bool operator!=(T* a, const TransferSharedPtr<T>& b)
{
    return a != b.get();
}

#endif // TransferSharedPtr_h
