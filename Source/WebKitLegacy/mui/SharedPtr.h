/*
 *  Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef SharedPtr_h
#define SharedPtr_h

template <typename T> class TransferSharedPtr;

template <typename T> class SharedPtr {
public:
    SharedPtr() : m_ptr(0) { }
    SharedPtr(T* ptr) : m_ptr(ptr) { if (ptr) ptr->ref(); }
    SharedPtr(const SharedPtr& o) : m_ptr(o.m_ptr) { if (T* ptr = m_ptr) ptr->ref(); }
    SharedPtr(const TransferSharedPtr<T>& o) : m_ptr(o.releaseRef()) { }
    template <typename U> SharedPtr(const TransferSharedPtr<U>&o) : m_ptr(o.releaseRef()) { }

    ~SharedPtr() { if (T* ptr = m_ptr) ptr->deref(); }

    T* get() const { return m_ptr; }

    // operator* and operator& are disabled because of the STL
    T* operator->() const { return m_ptr; }

    bool operator!() const { return !m_ptr; }

    operator bool() const { return m_ptr; }

    SharedPtr& operator=(const SharedPtr& o)
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

    SharedPtr& operator=(T* optr)
    {
        if (optr)
            optr->ref();
        T* ptr = m_ptr;
        m_ptr = optr;
        if (ptr)
            ptr->deref();
        return *this;
    }

    SharedPtr& operator=(const TransferSharedPtr<T>& o)
    {
        T* ptr = m_ptr;
        m_ptr = o.releaseRef();
        if (ptr)
            ptr->deref();
        return *this;
    }

    template <typename U> SharedPtr& operator=(const SharedPtr<U>& o)
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

    template <typename U> SharedPtr& operator=(const TransferSharedPtr<U>& o)
    {
        T* ptr = m_ptr;
        m_ptr = o.releaseRef();
        if (ptr)
            ptr->deref();
        return *this;
    }

private:
    T* m_ptr;
};

template <typename T> inline bool operator==(const SharedPtr<T>& a, const SharedPtr<T>& b)
{
    return a.get() == b.get();
}

template <typename T> inline bool operator==(const SharedPtr<T>& a, T* b)
{
    return a.get() == b;
}

template <typename T> inline bool operator==(T* a, const SharedPtr<T>& b)
{
    return a == b.get();
}

template <typename T> inline bool operator!=(const SharedPtr<T>& a, const SharedPtr<T>& b)
{
    return a.get() != b.get();
}

template <typename T> inline bool operator!=(const SharedPtr<T>& a, T* b)
{
    return a.get() != b;
}

template <typename T> inline bool operator!=(T* a, const SharedPtr<T>& b)
{
    return a != b.get();
}

#endif // SharedPtr_h
