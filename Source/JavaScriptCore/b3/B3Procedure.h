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

#ifndef B3Procedure_h
#define B3Procedure_h

#if ENABLE(B3_JIT)

#include "B3OpaqueByproducts.h"
#include "B3Origin.h"
#include "B3PCToOriginMap.h"
#include "B3SparseCollection.h"
#include "B3Type.h"
#include "B3ValueKey.h"
#include "PureNaN.h"
#include "RegisterAtOffsetList.h"
#include <wtf/Bag.h>
#include <wtf/FastMalloc.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/PrintStream.h>
#include <wtf/SharedTask.h>
#include <wtf/TriState.h>
#include <wtf/Vector.h>

namespace JSC { namespace B3 {

class BasicBlock;
class BlockInsertionSet;
class CFG;
class Dominators;
class StackSlot;
class Value;
class Variable;

namespace Air { class Code; }

class Procedure {
    WTF_MAKE_NONCOPYABLE(Procedure);
    WTF_MAKE_FAST_ALLOCATED;
public:

    JS_EXPORT_PRIVATE Procedure();
    JS_EXPORT_PRIVATE ~Procedure();

    template<typename Callback>
    void setOriginPrinter(Callback&& callback)
    {
        m_originPrinter = createSharedTask<void(PrintStream&, Origin)>(
            std::forward<Callback>(callback));
    }

    // Usually you use this via OriginDump, though it's cool to use it directly.
    void printOrigin(PrintStream& out, Origin origin) const;

    // This is a debugging hack. Sometimes while debugging B3 you need to break the abstraction
    // and get at the DFG Graph, or whatever data structure the frontend used to describe the
    // program. The FTL passes the DFG Graph.
    void setFrontendData(const void* value) { m_frontendData = value; }
    const void* frontendData() const { return m_frontendData; }

    JS_EXPORT_PRIVATE BasicBlock* addBlock(double frequency = 1);

    // Changes the order of basic blocks to be as in the supplied vector. The vector does not
    // need to mention every block in the procedure. Blocks not mentioned will be placed after
    // these blocks in the same order as they were in originally.
    template<typename BlockIterable>
    void setBlockOrder(const BlockIterable& iterable)
    {
        Vector<BasicBlock*> blocks;
        for (BasicBlock* block : iterable)
            blocks.append(block);
        setBlockOrderImpl(blocks);
    }

    JS_EXPORT_PRIVATE StackSlot* addStackSlot(unsigned byteSize);
    JS_EXPORT_PRIVATE Variable* addVariable(Type);
    
    template<typename ValueType, typename... Arguments>
    ValueType* add(Arguments...);

    Value* clone(Value*);

    Value* addIntConstant(Origin, Type, int64_t value);
    Value* addIntConstant(Value*, int64_t value);

    Value* addBottom(Origin, Type);
    Value* addBottom(Value*);

    // Returns null for MixedTriState.
    Value* addBoolConstant(Origin, TriState);

    void resetValueOwners();
    void resetReachability();

    // This destroys CFG analyses. If we ask for them again, we will recompute them. Usually you
    // should call this anytime you call resetReachability().
    void invalidateCFG();

    JS_EXPORT_PRIVATE void dump(PrintStream&) const;

    unsigned size() const { return m_blocks.size(); }
    BasicBlock* at(unsigned index) const { return m_blocks[index].get(); }
    BasicBlock* operator[](unsigned index) const { return at(index); }

    class iterator {
    public:
        iterator()
            : m_procedure(nullptr)
            , m_index(0)
        {
        }

        iterator(const Procedure& procedure, unsigned index)
            : m_procedure(&procedure)
            , m_index(findNext(index))
        {
        }

        BasicBlock* operator*()
        {
            return m_procedure->at(m_index);
        }

        iterator& operator++()
        {
            m_index = findNext(m_index + 1);
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            ASSERT(m_procedure == other.m_procedure);
            return m_index == other.m_index;
        }
        
        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

    private:
        unsigned findNext(unsigned index)
        {
            while (index < m_procedure->size() && !m_procedure->at(index))
                index++;
            return index;
        }

        const Procedure* m_procedure;
        unsigned m_index;
    };

    iterator begin() const { return iterator(*this, 0); }
    iterator end() const { return iterator(*this, size()); }

    Vector<BasicBlock*> blocksInPreOrder();
    Vector<BasicBlock*> blocksInPostOrder();

    SparseCollection<StackSlot>& stackSlots() { return m_stackSlots; }
    const SparseCollection<StackSlot>& stackSlots() const { return m_stackSlots; }

    // Short for stackSlots().remove(). It's better to call this method since it's out of line.
    void deleteStackSlot(StackSlot*);

    SparseCollection<Variable>& variables() { return m_variables; }
    const SparseCollection<Variable>& variables() const { return m_variables; }

    // Short for variables().remove(). It's better to call this method since it's out of line.
    void deleteVariable(Variable*);

    SparseCollection<Value>& values() { return m_values; }
    const SparseCollection<Value>& values() const { return m_values; }

    // Short for values().remove(). It's better to call this method since it's out of line.
    void deleteValue(Value*);

    // A valid procedure cannot contain any orphan values. An orphan is a value that is not in
    // any basic block. It is possible to create an orphan value during code generation or during
    // transformation. If you know that you may have created some, you can call this method to
    // delete them, making the procedure valid again.
    void deleteOrphans();

    CFG& cfg() const { return *m_cfg; }

    Dominators& dominators();

    void addFastConstant(const ValueKey&);
    bool isFastConstant(const ValueKey&);

    // The name has to be a string literal, since we don't do any memory management for the string.
    void setLastPhaseName(const char* name)
    {
        m_lastPhaseName = name;
    }

    const char* lastPhaseName() const { return m_lastPhaseName; }

    void* addDataSection(size_t size);

    OpaqueByproducts& byproducts() { return *m_byproducts; }

    // Below are methods that make sense to call after you have generated code for the procedure.

    // You have to call this method after calling generate(). The code generated by B3::generate()
    // will require you to keep this object alive for as long as that code is runnable. Usually, this
    // just keeps alive things like the double constant pool and switch lookup tables. If this sounds
    // confusing, you should probably be using the B3::Compilation API to compile code. If you use
    // that API, then you don't have to worry about this.
    std::unique_ptr<OpaqueByproducts> releaseByproducts() { return WTFMove(m_byproducts); }

    // This gives you direct access to Code. However, the idea is that clients of B3 shouldn't have to
    // call this. So, Procedure has some methods (below) that expose some Air::Code functionality.
    const Air::Code& code() const { return *m_code; }
    Air::Code& code() { return *m_code; }

    unsigned callArgAreaSize() const;
    void requestCallArgAreaSize(unsigned size);

    JS_EXPORT_PRIVATE unsigned frameSize() const;
    const RegisterAtOffsetList& calleeSaveRegisters() const;

    PCToOriginMap& pcToOriginMap() { return m_pcToOriginMap; }
    PCToOriginMap releasePCToOriginMap() { return WTFMove(m_pcToOriginMap); }

private:
    friend class BlockInsertionSet;

    JS_EXPORT_PRIVATE Value* addValueImpl(Value*);
    void setBlockOrderImpl(Vector<BasicBlock*>&);

    SparseCollection<StackSlot> m_stackSlots;
    SparseCollection<Variable> m_variables;
    Vector<std::unique_ptr<BasicBlock>> m_blocks;
    SparseCollection<Value> m_values;
    std::unique_ptr<CFG> m_cfg;
    std::unique_ptr<Dominators> m_dominators;
    HashSet<ValueKey> m_fastConstants;
    const char* m_lastPhaseName;
    std::unique_ptr<OpaqueByproducts> m_byproducts;
    std::unique_ptr<Air::Code> m_code;
    RefPtr<SharedTask<void(PrintStream&, Origin)>> m_originPrinter;
    const void* m_frontendData;
    PCToOriginMap m_pcToOriginMap;
};

} } // namespace JSC::B3

#endif // ENABLE(B3_JIT)

#endif // B3Procedure_h

