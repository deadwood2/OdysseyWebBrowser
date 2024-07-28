/*
 * Copyright (C) 2011-2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SpeculatedType_h
#define SpeculatedType_h

#include "JSCJSValue.h"
#include "TypedArrayType.h"
#include <wtf/PrintStream.h>

namespace JSC {

class Structure;

typedef uint32_t SpeculatedType;
static const SpeculatedType SpecNone               = 0; // We don't know anything yet.
static const SpeculatedType SpecFinalObject        = 1u << 0; // It's definitely a JSFinalObject.
static const SpeculatedType SpecArray              = 1u << 1; // It's definitely a JSArray.
static const SpeculatedType SpecFunction           = 1u << 2; // It's definitely a JSFunction.
static const SpeculatedType SpecInt8Array          = 1u << 3; // It's definitely an Int8Array or one of its subclasses.
static const SpeculatedType SpecInt16Array         = 1u << 4; // It's definitely an Int16Array or one of its subclasses.
static const SpeculatedType SpecInt32Array         = 1u << 5; // It's definitely an Int32Array or one of its subclasses.
static const SpeculatedType SpecUint8Array         = 1u << 6; // It's definitely an Uint8Array or one of its subclasses.
static const SpeculatedType SpecUint8ClampedArray  = 1u << 7; // It's definitely an Uint8ClampedArray or one of its subclasses.
static const SpeculatedType SpecUint16Array        = 1u << 8; // It's definitely an Uint16Array or one of its subclasses.
static const SpeculatedType SpecUint32Array        = 1u << 9; // It's definitely an Uint32Array or one of its subclasses.
static const SpeculatedType SpecFloat32Array       = 1u << 10; // It's definitely an Uint16Array or one of its subclasses.
static const SpeculatedType SpecFloat64Array       = 1u << 11; // It's definitely an Uint16Array or one of its subclasses.
static const SpeculatedType SpecTypedArrayView     = SpecInt8Array | SpecInt16Array | SpecInt32Array | SpecUint8Array | SpecUint8ClampedArray | SpecUint16Array | SpecUint32Array | SpecFloat32Array | SpecFloat64Array;
static const SpeculatedType SpecDirectArguments    = 1u << 12; // It's definitely a DirectArguments object.
static const SpeculatedType SpecScopedArguments    = 1u << 13; // It's definitely a ScopedArguments object.
static const SpeculatedType SpecStringObject       = 1u << 14; // It's definitely a StringObject.
static const SpeculatedType SpecRegExpObject       = 1u << 15; // It's definitely a RegExpObject (and not any subclass of RegExpObject).
static const SpeculatedType SpecObjectOther        = 1u << 16; // It's definitely an object but not JSFinalObject, JSArray, or JSFunction.
static const SpeculatedType SpecObject             = SpecFinalObject | SpecArray | SpecFunction | SpecTypedArrayView | SpecDirectArguments | SpecScopedArguments | SpecStringObject | SpecRegExpObject | SpecObjectOther; // Bitmask used for testing for any kind of object prediction.
static const SpeculatedType SpecStringIdent        = 1u << 17; // It's definitely a JSString, and it's an identifier.
static const SpeculatedType SpecStringVar          = 1u << 18; // It's definitely a JSString, and it's not an identifier.
static const SpeculatedType SpecString             = SpecStringIdent | SpecStringVar; // It's definitely a JSString.
static const SpeculatedType SpecSymbol             = 1u << 19; // It's definitely a Symbol.
static const SpeculatedType SpecCellOther          = 1u << 20; // It's definitely a JSCell but not a subclass of JSObject and definitely not a JSString or a Symbol. FIXME: This shouldn't be part of heap-top or bytecode-top. https://bugs.webkit.org/show_bug.cgi?id=133078
static const SpeculatedType SpecCell               = SpecObject | SpecString | SpecSymbol | SpecCellOther; // It's definitely a JSCell.
static const SpeculatedType SpecBoolInt32          = 1u << 21; // It's definitely an Int32 with value 0 or 1.
static const SpeculatedType SpecNonBoolInt32       = 1u << 22; // It's definitely an Int32 with value other than 0 or 1.
static const SpeculatedType SpecInt32              = SpecBoolInt32 | SpecNonBoolInt32; // It's definitely an Int32.
static const SpeculatedType SpecInt52              = 1u << 23; // It's definitely an Int52 and we intend it to unbox it.
static const SpeculatedType SpecMachineInt         = SpecInt32 | SpecInt52; // It's something that we can do machine int arithmetic on.
static const SpeculatedType SpecInt52AsDouble      = 1u << 24; // It's definitely an Int52 and it's inside a double.
static const SpeculatedType SpecInteger            = SpecMachineInt | SpecInt52AsDouble; // It's definitely some kind of integer.
static const SpeculatedType SpecNonIntAsDouble     = 1u << 25; // It's definitely not an Int52 but it's a real number and it's a double.
static const SpeculatedType SpecDoubleReal         = SpecNonIntAsDouble | SpecInt52AsDouble; // It's definitely a non-NaN double.
static const SpeculatedType SpecDoublePureNaN      = 1u << 26; // It's definitely a NaN that is sae to tag (i.e. pure).
static const SpeculatedType SpecDoubleImpureNaN    = 1u << 27; // It's definitely a NaN that is unsafe to tag (i.e. impure).
static const SpeculatedType SpecDoubleNaN          = SpecDoublePureNaN | SpecDoubleImpureNaN; // It's definitely some kind of NaN.
static const SpeculatedType SpecBytecodeDouble     = SpecDoubleReal | SpecDoublePureNaN; // It's either a non-NaN or a NaN double, but it's definitely not impure NaN.
static const SpeculatedType SpecFullDouble         = SpecDoubleReal | SpecDoubleNaN; // It's either a non-NaN or a NaN double.
static const SpeculatedType SpecBytecodeRealNumber = SpecInt32 | SpecDoubleReal; // It's either an Int32 or a DoubleReal.
static const SpeculatedType SpecFullRealNumber     = SpecMachineInt | SpecDoubleReal; // It's either an Int32 or a DoubleReal, or a Int52.
static const SpeculatedType SpecBytecodeNumber     = SpecInt32 | SpecBytecodeDouble; // It's either an Int32 or a Double, and the Double cannot be an impure NaN.
static const SpeculatedType SpecFullNumber         = SpecMachineInt | SpecFullDouble; // It's either an Int32, Int52, or a Double, and the Double can be impure NaN.
static const SpeculatedType SpecBoolean            = 1u << 28; // It's definitely a Boolean.
static const SpeculatedType SpecOther              = 1u << 29; // It's definitely either Null or Undefined.
static const SpeculatedType SpecMisc               = SpecBoolean | SpecOther; // It's definitely either a boolean, Null, or Undefined.
static const SpeculatedType SpecHeapTop            = SpecCell | SpecBytecodeNumber | SpecMisc; // It can be any of the above, except for SpecInt52 and SpecDoubleImpureNaN.
static const SpeculatedType SpecPrimitive          = SpecString | SpecSymbol | SpecBytecodeNumber | SpecMisc; // It's any non-Object JSValue.
static const SpeculatedType SpecEmpty              = 1u << 30; // It's definitely an empty value marker.
static const SpeculatedType SpecBytecodeTop        = SpecHeapTop | SpecEmpty; // It can be any of the above, except for SpecInt52 and SpecDoubleImpureNaN. Corresponds to what could be found in a bytecode local.
static const SpeculatedType SpecFullTop            = SpecBytecodeTop | SpecFullNumber; // It can be anything that bytecode could see plus exotic encodings of numbers.

typedef bool (*SpeculatedTypeChecker)(SpeculatedType);

// Dummy prediction checker, only useful if someone insists on requiring a prediction checker.
inline bool isAnySpeculation(SpeculatedType)
{
    return true;
}

inline bool isCellSpeculation(SpeculatedType value)
{
    return !!(value & SpecCell) && !(value & ~SpecCell);
}

inline bool isCellOrOtherSpeculation(SpeculatedType value)
{
    return !!value && !(value & ~(SpecCell | SpecOther));
}

inline bool isNotCellSpeculation(SpeculatedType value)
{
    return !(value & SpecCell) && value;
}

inline bool isObjectSpeculation(SpeculatedType value)
{
    return !!(value & SpecObject) && !(value & ~SpecObject);
}

inline bool isObjectOrOtherSpeculation(SpeculatedType value)
{
    return !!(value & (SpecObject | SpecOther)) && !(value & ~(SpecObject | SpecOther));
}

inline bool isFinalObjectSpeculation(SpeculatedType value)
{
    return value == SpecFinalObject;
}

inline bool isFinalObjectOrOtherSpeculation(SpeculatedType value)
{
    return !!(value & (SpecFinalObject | SpecOther)) && !(value & ~(SpecFinalObject | SpecOther));
}

inline bool isStringIdentSpeculation(SpeculatedType value)
{
    return value == SpecStringIdent;
}

inline bool isNotStringVarSpeculation(SpeculatedType value)
{
    return !(value & SpecStringVar);
}

inline bool isStringSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecString) == value;
}

inline bool isStringOrOtherSpeculation(SpeculatedType value)
{
    return !!value && (value & (SpecString | SpecOther)) == value;
}

inline bool isSymbolSpeculation(SpeculatedType value)
{
    return value == SpecSymbol;
}

inline bool isArraySpeculation(SpeculatedType value)
{
    return value == SpecArray;
}

inline bool isFunctionSpeculation(SpeculatedType value)
{
    return value == SpecFunction;
}

inline bool isInt8ArraySpeculation(SpeculatedType value)
{
    return value == SpecInt8Array;
}

inline bool isInt16ArraySpeculation(SpeculatedType value)
{
    return value == SpecInt16Array;
}

inline bool isInt32ArraySpeculation(SpeculatedType value)
{
    return value == SpecInt32Array;
}

inline bool isUint8ArraySpeculation(SpeculatedType value)
{
    return value == SpecUint8Array;
}

inline bool isUint8ClampedArraySpeculation(SpeculatedType value)
{
    return value == SpecUint8ClampedArray;
}

inline bool isUint16ArraySpeculation(SpeculatedType value)
{
    return value == SpecUint16Array;
}

inline bool isUint32ArraySpeculation(SpeculatedType value)
{
    return value == SpecUint32Array;
}

inline bool isFloat32ArraySpeculation(SpeculatedType value)
{
    return value == SpecFloat32Array;
}

inline bool isFloat64ArraySpeculation(SpeculatedType value)
{
    return value == SpecFloat64Array;
}

inline bool isDirectArgumentsSpeculation(SpeculatedType value)
{
    return value == SpecDirectArguments;
}

inline bool isScopedArgumentsSpeculation(SpeculatedType value)
{
    return value == SpecScopedArguments;
}

inline bool isActionableIntMutableArraySpeculation(SpeculatedType value)
{
    return isInt8ArraySpeculation(value)
        || isInt16ArraySpeculation(value)
        || isInt32ArraySpeculation(value)
        || isUint8ArraySpeculation(value)
        || isUint8ClampedArraySpeculation(value)
        || isUint16ArraySpeculation(value)
        || isUint32ArraySpeculation(value);
}

inline bool isActionableFloatMutableArraySpeculation(SpeculatedType value)
{
    return isFloat32ArraySpeculation(value)
        || isFloat64ArraySpeculation(value);
}

inline bool isActionableTypedMutableArraySpeculation(SpeculatedType value)
{
    return isActionableIntMutableArraySpeculation(value)
        || isActionableFloatMutableArraySpeculation(value);
}

inline bool isActionableMutableArraySpeculation(SpeculatedType value)
{
    return isArraySpeculation(value)
        || isActionableTypedMutableArraySpeculation(value);
}

inline bool isActionableArraySpeculation(SpeculatedType value)
{
    return isStringSpeculation(value)
        || isDirectArgumentsSpeculation(value)
        || isScopedArgumentsSpeculation(value)
        || isActionableMutableArraySpeculation(value);
}

inline bool isArrayOrOtherSpeculation(SpeculatedType value)
{
    return !!(value & (SpecArray | SpecOther)) && !(value & ~(SpecArray | SpecOther));
}

inline bool isStringObjectSpeculation(SpeculatedType value)
{
    return value == SpecStringObject;
}

inline bool isStringOrStringObjectSpeculation(SpeculatedType value)
{
    return !!value && !(value & ~(SpecString | SpecStringObject));
}

inline bool isRegExpObjectSpeculation(SpeculatedType value)
{
    return value == SpecRegExpObject;
}

inline bool isBoolInt32Speculation(SpeculatedType value)
{
    return value == SpecBoolInt32;
}

inline bool isInt32Speculation(SpeculatedType value)
{
    return value && !(value & ~SpecInt32);
}

inline bool isInt32OrBooleanSpeculation(SpeculatedType value)
{
    return value && !(value & ~(SpecBoolean | SpecInt32));
}

inline bool isInt32SpeculationForArithmetic(SpeculatedType value)
{
    return !(value & (SpecFullDouble | SpecInt52));
}

inline bool isInt32OrBooleanSpeculationForArithmetic(SpeculatedType value)
{
    return !(value & (SpecFullDouble | SpecInt52));
}

inline bool isInt32OrBooleanSpeculationExpectingDefined(SpeculatedType value)
{
    return isInt32OrBooleanSpeculation(value & ~SpecOther);
}

inline bool isInt52Speculation(SpeculatedType value)
{
    return value == SpecInt52;
}

inline bool isMachineIntSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecMachineInt) == value;
}

inline bool isInt52AsDoubleSpeculation(SpeculatedType value)
{
    return value == SpecInt52AsDouble;
}

inline bool isIntegerSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecInteger) == value;
}

inline bool isDoubleRealSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecDoubleReal) == value;
}

inline bool isDoubleSpeculation(SpeculatedType value)
{
    return !!value && (value & SpecFullDouble) == value;
}

inline bool isDoubleSpeculationForArithmetic(SpeculatedType value)
{
    return !!(value & SpecFullDouble);
}

inline bool isBytecodeRealNumberSpeculation(SpeculatedType value)
{
    return !!(value & SpecBytecodeRealNumber) && !(value & ~SpecBytecodeRealNumber);
}

inline bool isFullRealNumberSpeculation(SpeculatedType value)
{
    return !!(value & SpecFullRealNumber) && !(value & ~SpecFullRealNumber);
}

inline bool isBytecodeNumberSpeculation(SpeculatedType value)
{
    return !!(value & SpecBytecodeNumber) && !(value & ~SpecBytecodeNumber);
}

inline bool isFullNumberSpeculation(SpeculatedType value)
{
    return !!(value & SpecFullNumber) && !(value & ~SpecFullNumber);
}

inline bool isFullNumberOrBooleanSpeculation(SpeculatedType value)
{
    return value && !(value & ~(SpecFullNumber | SpecBoolean));
}

inline bool isFullNumberOrBooleanSpeculationExpectingDefined(SpeculatedType value)
{
    return isFullNumberOrBooleanSpeculation(value & ~SpecOther);
}

inline bool isBooleanSpeculation(SpeculatedType value)
{
    return value == SpecBoolean;
}

inline bool isOtherSpeculation(SpeculatedType value)
{
    return value == SpecOther;
}

inline bool isMiscSpeculation(SpeculatedType value)
{
    return !!value && !(value & ~SpecMisc);
}

inline bool isOtherOrEmptySpeculation(SpeculatedType value)
{
    return !value || value == SpecOther;
}

inline bool isEmptySpeculation(SpeculatedType value)
{
    return value == SpecEmpty;
}

inline bool isUntypedSpeculationForArithmetic(SpeculatedType value)
{
    return !!(value & ~(SpecFullNumber | SpecBoolean));
}

inline bool isUntypedSpeculationForBitOps(SpeculatedType value)
{
    return !!(value & ~(SpecFullNumber | SpecBoolean | SpecOther));
}

void dumpSpeculation(PrintStream&, SpeculatedType);
void dumpSpeculationAbbreviated(PrintStream&, SpeculatedType);

MAKE_PRINT_ADAPTOR(SpeculationDump, SpeculatedType, dumpSpeculation);
MAKE_PRINT_ADAPTOR(AbbreviatedSpeculationDump, SpeculatedType, dumpSpeculationAbbreviated);

// Merge two predictions. Note that currently this just does left | right. It may
// seem tempting to do so directly, but you would be doing so at your own peril,
// since the merging protocol SpeculatedType may change at any time (and has already
// changed several times in its history).
inline SpeculatedType mergeSpeculations(SpeculatedType left, SpeculatedType right)
{
    return left | right;
}

template<typename T>
inline bool mergeSpeculation(T& left, SpeculatedType right)
{
    SpeculatedType newSpeculation = static_cast<T>(mergeSpeculations(static_cast<SpeculatedType>(left), right));
    bool result = newSpeculation != static_cast<SpeculatedType>(left);
    left = newSpeculation;
    return result;
}

inline bool speculationChecked(SpeculatedType actual, SpeculatedType desired)
{
    return (actual | desired) == desired;
}

SpeculatedType speculationFromClassInfo(const ClassInfo*);
SpeculatedType speculationFromStructure(Structure*);
SpeculatedType speculationFromCell(JSCell*);
SpeculatedType speculationFromValue(JSValue);

SpeculatedType speculationFromTypedArrayType(TypedArrayType); // only valid for typed views.
TypedArrayType typedArrayTypeFromSpeculation(SpeculatedType);

SpeculatedType leastUpperBoundOfStrictlyEquivalentSpeculations(SpeculatedType);

bool valuesCouldBeEqual(SpeculatedType, SpeculatedType);

// Precise computation of the type of the result of a double computation after we
// already know that the inputs are doubles and that the result must be a double. Use
// the closest one of these that applies.
SpeculatedType typeOfDoubleSum(SpeculatedType, SpeculatedType);
SpeculatedType typeOfDoubleDifference(SpeculatedType, SpeculatedType);
SpeculatedType typeOfDoubleProduct(SpeculatedType, SpeculatedType);
SpeculatedType typeOfDoubleQuotient(SpeculatedType, SpeculatedType);
SpeculatedType typeOfDoubleMinMax(SpeculatedType, SpeculatedType);
SpeculatedType typeOfDoubleNegation(SpeculatedType);
SpeculatedType typeOfDoubleAbs(SpeculatedType);
SpeculatedType typeOfDoubleRounding(SpeculatedType);
SpeculatedType typeOfDoublePow(SpeculatedType, SpeculatedType);

// This conservatively models the behavior of arbitrary double operations.
SpeculatedType typeOfDoubleBinaryOp(SpeculatedType, SpeculatedType);
SpeculatedType typeOfDoubleUnaryOp(SpeculatedType);

} // namespace JSC

#endif // SpeculatedType_h
