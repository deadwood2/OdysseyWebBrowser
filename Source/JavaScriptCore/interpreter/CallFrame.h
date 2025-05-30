/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003-2018 Apple Inc. All rights reserved.
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

#pragma once

#include "AbstractPC.h"
#include "CalleeBits.h"
#include "MacroAssemblerCodeRef.h"
#include "Register.h"
#include "StackVisitor.h"
#include "VM.h"
#include "VMEntryRecord.h"

namespace JSC  {

    class Arguments;
    class ExecState;
    class Interpreter;
    class JSCallee;
    class JSScope;
    class SourceOrigin;

    struct Instruction;

    typedef ExecState CallFrame;

    class CallSiteIndex {
    public:
        CallSiteIndex() = default;
        
        explicit CallSiteIndex(uint32_t bits)
            : m_bits(bits)
        { }
#if USE(JSVALUE32_64)
        explicit CallSiteIndex(const Instruction* instruction)
            : m_bits(bitwise_cast<uint32_t>(instruction))
        { }
#endif

        explicit operator bool() const { return m_bits != UINT_MAX; }
        bool operator==(const CallSiteIndex& other) const { return m_bits == other.m_bits; }
        
        inline uint32_t bits() const { return m_bits; }

    private:
        uint32_t m_bits { UINT_MAX };
    };

    class DisposableCallSiteIndex : public CallSiteIndex {
    public:
        DisposableCallSiteIndex() = default;

        explicit DisposableCallSiteIndex(uint32_t bits)
            : CallSiteIndex(bits)
        {
        }

        static DisposableCallSiteIndex fromCallSiteIndex(CallSiteIndex callSiteIndex)
        {
            return DisposableCallSiteIndex(callSiteIndex.bits());
        }
    };

    // arm64_32 expects caller frame and return pc to use 8 bytes 
    struct CallerFrameAndPC {
        alignas(CPURegister) CallFrame* callerFrame;
        alignas(CPURegister) const Instruction* returnPC;
        static const int sizeInRegisters = 2 * sizeof(CPURegister) / sizeof(Register);
    };
    static_assert(CallerFrameAndPC::sizeInRegisters == sizeof(CallerFrameAndPC) / sizeof(Register), "CallerFrameAndPC::sizeInRegisters is incorrect.");

    struct CallFrameSlot {
        static const int codeBlock = CallerFrameAndPC::sizeInRegisters;
        static const int callee = codeBlock + 1;
        static const int argumentCount = callee + 1;
        static const int thisArgument = argumentCount + 1;
        static const int firstArgument = thisArgument + 1;
    };

    // Represents the current state of script execution.
    // Passed as the first argument to most functions.
    class ExecState : private Register {
    public:
        static const int headerSizeInRegisters = CallFrameSlot::argumentCount + 1;

        // This function should only be called in very specific circumstances
        // when you've guaranteed the callee can't be a Wasm callee, and can
        // be an arbitrary JSValue. This function should basically never be used.
        // Its only use right now is when we are making a call, and we're not
        // yet sure if the callee is a cell. In general, a JS callee is guaranteed
        // to be a cell, however, there is a brief window where we need to check
        // to see if it's a cell, and if it's not, we throw an exception.
        JSValue guaranteedJSValueCallee() const
        {
            ASSERT(!callee().isWasm());
            return this[CallFrameSlot::callee].jsValue();
        }
        JSObject* jsCallee() const
        {
            ASSERT(!callee().isWasm());
            return this[CallFrameSlot::callee].object();
        }
        CalleeBits callee() const { return CalleeBits(this[CallFrameSlot::callee].pointer()); }
        SUPPRESS_ASAN CalleeBits unsafeCallee() const { return CalleeBits(this[CallFrameSlot::callee].asanUnsafePointer()); }
        CodeBlock* codeBlock() const { return this[CallFrameSlot::codeBlock].Register::codeBlock(); }
        CodeBlock** addressOfCodeBlock() const { return bitwise_cast<CodeBlock**>(this + CallFrameSlot::codeBlock); }
        SUPPRESS_ASAN CodeBlock* unsafeCodeBlock() const { return this[CallFrameSlot::codeBlock].Register::asanUnsafeCodeBlock(); }
        JSScope* scope(int scopeRegisterOffset) const
        {
            ASSERT(this[scopeRegisterOffset].Register::scope());
            return this[scopeRegisterOffset].Register::scope();
        }

        JSGlobalObject* wasmAwareLexicalGlobalObject(VM&);

        bool isAnyWasmCallee();

        // Global object in which the currently executing code was defined.
        // Differs from VM::vmEntryGlobalObject() during function calls across web browser frames.
        JSGlobalObject* lexicalGlobalObject() const;

        // Differs from lexicalGlobalObject because this will have DOM window shell rather than
        // the actual DOM window, which can't be "this" for security reasons.
        JSObject* globalThisValue() const;

        VM& vm() const;

        static CallFrame* create(Register* callFrameBase) { return static_cast<CallFrame*>(callFrameBase); }
        Register* registers() { return this; }
        const Register* registers() const { return this; }

        CallFrame& operator=(const Register& r) { *static_cast<Register*>(this) = r; return *this; }

        CallFrame* callerFrame() const { return static_cast<CallFrame*>(callerFrameOrEntryFrame()); }
        void* callerFrameOrEntryFrame() const { return callerFrameAndPC().callerFrame; }
        SUPPRESS_ASAN void* unsafeCallerFrameOrEntryFrame() const { return unsafeCallerFrameAndPC().callerFrame; }

        CallFrame* unsafeCallerFrame(EntryFrame*&) const;
        JS_EXPORT_PRIVATE CallFrame* callerFrame(EntryFrame*&) const;

        JS_EXPORT_PRIVATE SourceOrigin callerSourceOrigin();

        static ptrdiff_t callerFrameOffset() { return OBJECT_OFFSETOF(CallerFrameAndPC, callerFrame); }

        ReturnAddressPtr returnPC() const { return ReturnAddressPtr(callerFrameAndPC().returnPC); }
        bool hasReturnPC() const { return !!callerFrameAndPC().returnPC; }
        void clearReturnPC() { callerFrameAndPC().returnPC = 0; }
        static ptrdiff_t returnPCOffset() { return OBJECT_OFFSETOF(CallerFrameAndPC, returnPC); }
        AbstractPC abstractReturnPC(VM& vm) { return AbstractPC(vm, this); }

        bool callSiteBitsAreBytecodeOffset() const;
        bool callSiteBitsAreCodeOriginIndex() const;

        unsigned callSiteAsRawBits() const;
        unsigned unsafeCallSiteAsRawBits() const;
        CallSiteIndex callSiteIndex() const;
        CallSiteIndex unsafeCallSiteIndex() const;
    private:
        unsigned callSiteBitsAsBytecodeOffset() const;
    public:

        // This will try to get you the bytecode offset, but you should be aware that
        // this bytecode offset may be bogus in the presence of inlining. This will
        // also return 0 if the call frame has no notion of bytecode offsets (for
        // example if it's native code).
        // https://bugs.webkit.org/show_bug.cgi?id=121754
        unsigned bytecodeOffset();
        
        // This will get you a CodeOrigin. It will always succeed. May return
        // CodeOrigin(0) if we're in native code.
        JS_EXPORT_PRIVATE CodeOrigin codeOrigin();

        Register* topOfFrame()
        {
            if (!codeBlock())
                return registers();
            return topOfFrameInternal();
        }
    
        const Instruction* currentVPC() const; // This only makes sense in the LLInt and baseline.
        void setCurrentVPC(const Instruction*);

        void setCallerFrame(CallFrame* frame) { callerFrameAndPC().callerFrame = frame; }
        void setScope(int scopeRegisterOffset, JSScope* scope) { static_cast<Register*>(this)[scopeRegisterOffset] = scope; }

        static void initGlobalExec(ExecState* globalExec, JSCallee* globalCallee);

        // Read a register from the codeframe (or constant from the CodeBlock).
        Register& r(int);
        Register& r(VirtualRegister);
        // Read a register for a non-constant
        Register& uncheckedR(int);
        Register& uncheckedR(VirtualRegister);

        // Access to arguments as passed. (After capture, arguments may move to a different location.)
        size_t argumentCount() const { return argumentCountIncludingThis() - 1; }
        size_t argumentCountIncludingThis() const { return this[CallFrameSlot::argumentCount].payload(); }
        static int argumentOffset(int argument) { return (CallFrameSlot::firstArgument + argument); }
        static int argumentOffsetIncludingThis(int argument) { return (CallFrameSlot::thisArgument + argument); }

        // In the following (argument() and setArgument()), the 'argument'
        // parameter is the index of the arguments of the target function of
        // this frame. The index starts at 0 for the first arg, 1 for the
        // second, etc.
        //
        // The arguments (in this case) do not include the 'this' value.
        // arguments(0) will not fetch the 'this' value. To get/set 'this',
        // use thisValue() and setThisValue() below.

        JSValue* addressOfArgumentsStart() const { return bitwise_cast<JSValue*>(this + argumentOffset(0)); }
        JSValue argument(size_t argument)
        {
            if (argument >= argumentCount())
                 return jsUndefined();
            return getArgumentUnsafe(argument);
        }
        JSValue uncheckedArgument(size_t argument)
        {
            ASSERT(argument < argumentCount());
            return getArgumentUnsafe(argument);
        }
        void setArgument(size_t argument, JSValue value)
        {
            this[argumentOffset(argument)] = value;
        }

        JSValue getArgumentUnsafe(size_t argIndex)
        {
            // User beware! This method does not verify that there is a valid
            // argument at the specified argIndex. This is used for debugging
            // and verification code only. The caller is expected to know what
            // he/she is doing when calling this method.
            return this[argumentOffset(argIndex)].jsValue();
        }

        static int thisArgumentOffset() { return argumentOffsetIncludingThis(0); }
        JSValue thisValue() { return this[thisArgumentOffset()].jsValue(); }
        void setThisValue(JSValue value) { this[thisArgumentOffset()] = value; }

        // Under the constructor implemented in C++, thisValue holds the newTarget instead of the automatically constructed value.
        // The result of this function is only effective under the "construct" context.
        JSValue newTarget() { return thisValue(); }

        JSValue argumentAfterCapture(size_t argument);

        static int offsetFor(size_t argumentCountIncludingThis) { return argumentCountIncludingThis + CallFrameSlot::thisArgument - 1; }

        static CallFrame* noCaller() { return nullptr; }
        bool isGlobalExec() const
        {
            return callerFrameAndPC().callerFrame == noCaller() && callerFrameAndPC().returnPC == nullptr;
        }

        void convertToStackOverflowFrame(VM&, CodeBlock* codeBlockToKeepAliveUntilFrameIsUnwound);
        inline bool isStackOverflowFrame() const;
        inline bool isWasmFrame() const;

        void setArgumentCountIncludingThis(int count) { static_cast<Register*>(this)[CallFrameSlot::argumentCount].payload() = count; }
        void setCallee(JSObject* callee) { static_cast<Register*>(this)[CallFrameSlot::callee] = callee; }
        void setCodeBlock(CodeBlock* codeBlock) { static_cast<Register*>(this)[CallFrameSlot::codeBlock] = codeBlock; }
        void setReturnPC(void* value) { callerFrameAndPC().returnPC = reinterpret_cast<const Instruction*>(value); }

        String friendlyFunctionName();

        // CallFrame::iterate() expects a Functor that implements the following method:
        //     StackVisitor::Status operator()(StackVisitor&) const;
        // FIXME: This method is improper. We rely on the fact that we can call it with a null
        // receiver. We should always be using StackVisitor directly.
        // It's only valid to call this from a non-wasm top frame.
        template <StackVisitor::EmptyEntryFrameAction action = StackVisitor::ContinueIfTopEntryFrameIsEmpty, typename Functor> void iterate(const Functor& functor)
        {
            VM* vm;
            void* rawThis = this;
            if (!!rawThis) {
                RELEASE_ASSERT(callee().isCell());
                vm = &this->vm();
            } else
                vm = nullptr;
            StackVisitor::visit<action, Functor>(this, vm, functor);
        }

        void dump(PrintStream&);
        JS_EXPORT_PRIVATE const char* describeFrame();

    private:

        ExecState();
        ~ExecState();

        Register* topOfFrameInternal();

        // The following are for internal use in debugging and verification
        // code only and not meant as an API for general usage:

        size_t argIndexForRegister(Register* reg)
        {
            // The register at 'offset' number of slots from the frame pointer
            // i.e.
            //       reg = frame[offset];
            //   ==> reg = frame + offset;
            //   ==> offset = reg - frame;
            int offset = reg - this->registers();

            // The offset is defined (based on argumentOffset()) to be:
            //       offset = CallFrameSlot::firstArgument - argIndex;
            // Hence:
            //       argIndex = CallFrameSlot::firstArgument - offset;
            size_t argIndex = offset - CallFrameSlot::firstArgument;
            return argIndex;
        }

        CallerFrameAndPC& callerFrameAndPC() { return *reinterpret_cast<CallerFrameAndPC*>(this); }
        const CallerFrameAndPC& callerFrameAndPC() const { return *reinterpret_cast<const CallerFrameAndPC*>(this); }
        SUPPRESS_ASAN const CallerFrameAndPC& unsafeCallerFrameAndPC() const { return *reinterpret_cast<const CallerFrameAndPC*>(this); }
    };

} // namespace JSC
