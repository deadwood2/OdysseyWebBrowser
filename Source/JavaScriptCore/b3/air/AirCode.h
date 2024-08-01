/*
 * Copyright (C) 2015-2016 Apple Inc. All rights reserved.
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

#ifndef AirCode_h
#define AirCode_h

#if ENABLE(B3_JIT)

#include "AirArg.h"
#include "AirBasicBlock.h"
#include "AirSpecial.h"
#include "AirStackSlot.h"
#include "AirTmp.h"
#include "B3SparseCollection.h"
#include "RegisterAtOffsetList.h"
#include "StackAlignment.h"

namespace JSC { namespace B3 {

class Procedure;

#if COMPILER(GCC) && ASSERT_DISABLED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif // COMPILER(GCC) && ASSERT_DISABLED

namespace Air {

class BlockInsertionSet;
class CCallSpecial;

// This is an IR that is very close to the bare metal. It requires about 40x more bytes than the
// generated machine code - for example if you're generating 1MB of machine code, you need about
// 40MB of Air.

class Code {
    WTF_MAKE_NONCOPYABLE(Code);
    WTF_MAKE_FAST_ALLOCATED;
public:
    ~Code();

    Procedure& proc() { return m_proc; }

    JS_EXPORT_PRIVATE BasicBlock* addBlock(double frequency = 1);

    // Note that you can rely on stack slots always getting indices that are larger than the index
    // of any prior stack slot. In fact, all stack slots you create in the future will have an index
    // that is >= stackSlots().size().
    JS_EXPORT_PRIVATE StackSlot* addStackSlot(
        unsigned byteSize, StackSlotKind, B3::StackSlot* = nullptr);
    StackSlot* addStackSlot(B3::StackSlot*);

    Special* addSpecial(std::unique_ptr<Special>);

    // This is the special you need to make a C call!
    CCallSpecial* cCallSpecial();

    Tmp newTmp(Arg::Type type)
    {
        switch (type) {
        case Arg::GP:
            return Tmp::gpTmpForIndex(m_numGPTmps++);
        case Arg::FP:
            return Tmp::fpTmpForIndex(m_numFPTmps++);
        }
        ASSERT_NOT_REACHED();
    }

    unsigned numTmps(Arg::Type type)
    {
        switch (type) {
        case Arg::GP:
            return m_numGPTmps;
        case Arg::FP:
            return m_numFPTmps;
        }
        ASSERT_NOT_REACHED();
    }

    unsigned callArgAreaSize() const { return m_callArgAreaSize; }

    // You can call this before code generation to force a minimum call arg area size.
    void requestCallArgAreaSize(unsigned size)
    {
        m_callArgAreaSize = std::max(
            m_callArgAreaSize,
            static_cast<unsigned>(WTF::roundUpToMultipleOf(stackAlignmentBytes(), size)));
    }

    unsigned frameSize() const { return m_frameSize; }

    // Only phases that do stack allocation are allowed to set this. Currently, only
    // Air::allocateStack() does this.
    void setFrameSize(unsigned frameSize)
    {
        m_frameSize = frameSize;
    }

    const RegisterAtOffsetList& calleeSaveRegisters() const { return m_calleeSaveRegisters; }
    RegisterAtOffsetList& calleeSaveRegisters() { return m_calleeSaveRegisters; }

    // Recomputes predecessors and deletes unreachable blocks.
    void resetReachability();

    void dump(PrintStream&) const;

    unsigned size() const { return m_blocks.size(); }
    BasicBlock* at(unsigned index) const { return m_blocks[index].get(); }
    BasicBlock* operator[](unsigned index) const { return at(index); }

    // This is used by phases that optimize the block list. You shouldn't use this unless you really know
    // what you're doing.
    Vector<std::unique_ptr<BasicBlock>>& blockList() { return m_blocks; }

    // Finds the smallest index' such that at(index') != null and index' >= index.
    unsigned findFirstBlockIndex(unsigned index) const;

    // Finds the smallest index' such that at(index') != null and index' > index.
    unsigned findNextBlockIndex(unsigned index) const;

    BasicBlock* findNextBlock(BasicBlock*) const;

    class iterator {
    public:
        iterator()
            : m_code(nullptr)
            , m_index(0)
        {
        }

        iterator(const Code& code, unsigned index)
            : m_code(&code)
            , m_index(m_code->findFirstBlockIndex(index))
        {
        }

        BasicBlock* operator*()
        {
            return m_code->at(m_index);
        }

        iterator& operator++()
        {
            m_index = m_code->findFirstBlockIndex(m_index + 1);
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            return m_index == other.m_index;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

    private:
        const Code* m_code;
        unsigned m_index;
    };

    iterator begin() const { return iterator(*this, 0); }
    iterator end() const { return iterator(*this, size()); }

    const SparseCollection<StackSlot>& stackSlots() const { return m_stackSlots; }
    SparseCollection<StackSlot>& stackSlots() { return m_stackSlots; }

    const SparseCollection<Special>& specials() const { return m_specials; }
    SparseCollection<Special>& specials() { return m_specials; }

    template<typename Callback>
    void forAllTmps(const Callback& callback) const
    {
        for (unsigned i = m_numGPTmps; i--;)
            callback(Tmp::gpTmpForIndex(i));
        for (unsigned i = m_numFPTmps; i--;)
            callback(Tmp::fpTmpForIndex(i));
    }

    void addFastTmp(Tmp);
    bool isFastTmp(Tmp tmp) const { return m_fastTmps.contains(tmp); }
    
    // The name has to be a string literal, since we don't do any memory management for the string.
    void setLastPhaseName(const char* name)
    {
        m_lastPhaseName = name;
    }

    const char* lastPhaseName() const { return m_lastPhaseName; }

private:
    friend class ::JSC::B3::Procedure;
    friend class BlockInsertionSet;
    
    Code(Procedure&);

    Procedure& m_proc; // Some meta-data, like byproducts, is stored in the Procedure.
    SparseCollection<StackSlot> m_stackSlots;
    Vector<std::unique_ptr<BasicBlock>> m_blocks;
    SparseCollection<Special> m_specials;
    HashSet<Tmp> m_fastTmps;
    CCallSpecial* m_cCallSpecial { nullptr };
    unsigned m_numGPTmps { 0 };
    unsigned m_numFPTmps { 0 };
    unsigned m_frameSize { 0 };
    unsigned m_callArgAreaSize { 0 };
    RegisterAtOffsetList m_calleeSaveRegisters;
    const char* m_lastPhaseName;
};

} } } // namespace JSC::B3::Air

#if COMPILER(GCC) && ASSERT_DISABLED
#pragma GCC diagnostic pop
#endif // COMPILER(GCC) && ASSERT_DISABLED

#endif // ENABLE(B3_JIT)

#endif // AirCode_h

