/*
 * Copyright (C) 2006-2018 Apple Inc. All rights reserved.
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

#pragma once

#include <algorithm>
#include <cmath>
#include <float.h>
#include <limits>
#include <stdint.h>
#include <stdlib.h>
#include <wtf/StdLibExtras.h>

#if OS(OPENBSD)
#include <sys/types.h>
#include <machine/ieee.h>
#endif

#ifndef M_PI
const double piDouble = 3.14159265358979323846;
const float piFloat = 3.14159265358979323846f;
#else
const double piDouble = M_PI;
const float piFloat = static_cast<float>(M_PI);
#endif

#ifndef M_PI_2
const double piOverTwoDouble = 1.57079632679489661923;
const float piOverTwoFloat = 1.57079632679489661923f;
#else
const double piOverTwoDouble = M_PI_2;
const float piOverTwoFloat = static_cast<float>(M_PI_2);
#endif

#ifndef M_PI_4
const double piOverFourDouble = 0.785398163397448309616;
const float piOverFourFloat = 0.785398163397448309616f;
#else
const double piOverFourDouble = M_PI_4;
const float piOverFourFloat = static_cast<float>(M_PI_4);
#endif

#ifndef M_SQRT2
const double sqrtOfTwoDouble = 1.41421356237309504880;
const float sqrtOfTwoFloat = 1.41421356237309504880f;
#else
const double sqrtOfTwoDouble = M_SQRT2;
const float sqrtOfTwoFloat = static_cast<float>(M_SQRT2);
#endif

#if OS(AROS) // temp for 32-bit SDK
#ifndef UINT16_MAX
#define UINT16_MAX (65535)
#endif
#endif

#if COMPILER(MSVC)

// Work around a bug in Win, where atan2(+-infinity, +-infinity) yields NaN instead of specific values.
extern "C" inline double wtf_atan2(double x, double y)
{
    double posInf = std::numeric_limits<double>::infinity();
    double negInf = -std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();

    double result = nan;

    if (x == posInf && y == posInf)
        result = piOverFourDouble;
    else if (x == posInf && y == negInf)
        result = 3 * piOverFourDouble;
    else if (x == negInf && y == posInf)
        result = -piOverFourDouble;
    else if (x == negInf && y == negInf)
        result = -3 * piOverFourDouble;
    else
        result = ::atan2(x, y);

    return result;
}

#define atan2(x, y) wtf_atan2(x, y)

#endif // COMPILER(MSVC)

inline double deg2rad(double d)  { return d * piDouble / 180.0; }
inline double rad2deg(double r)  { return r * 180.0 / piDouble; }
inline double deg2grad(double d) { return d * 400.0 / 360.0; }
inline double grad2deg(double g) { return g * 360.0 / 400.0; }
inline double turn2deg(double t) { return t * 360.0; }
inline double deg2turn(double d) { return d / 360.0; }
inline double rad2grad(double r) { return r * 200.0 / piDouble; }
inline double grad2rad(double g) { return g * piDouble / 200.0; }

inline float deg2rad(float d)  { return d * piFloat / 180.0f; }
inline float rad2deg(float r)  { return r * 180.0f / piFloat; }
inline float deg2grad(float d) { return d * 400.0f / 360.0f; }
inline float grad2deg(float g) { return g * 360.0f / 400.0f; }
inline float turn2deg(float t) { return t * 360.0f; }
inline float deg2turn(float d) { return d / 360.0f; }
inline float rad2grad(float r) { return r * 200.0f / piFloat; }
inline float grad2rad(float g) { return g * piFloat / 200.0f; }

// std::numeric_limits<T>::min() returns the smallest positive value for floating point types
template<typename T> constexpr T defaultMinimumForClamp() { return std::numeric_limits<T>::min(); }
template<> constexpr float defaultMinimumForClamp() { return -std::numeric_limits<float>::max(); }
template<> constexpr double defaultMinimumForClamp() { return -std::numeric_limits<double>::max(); }
template<typename T> constexpr T defaultMaximumForClamp() { return std::numeric_limits<T>::max(); }

// Same type in and out.
template<typename TargetType, typename SourceType>
typename std::enable_if<std::is_same<TargetType, SourceType>::value, TargetType>::type
clampTo(SourceType value, TargetType min = defaultMinimumForClamp<TargetType>(), TargetType max = defaultMaximumForClamp<TargetType>())
{
    if (value >= max)
        return max;
    if (value <= min)
        return min;
    return value;
}

// Floating point source.
template<typename TargetType, typename SourceType>
typename std::enable_if<!std::is_same<TargetType, SourceType>::value
    && std::is_floating_point<SourceType>::value
    && !(std::is_floating_point<TargetType>::value && sizeof(TargetType) > sizeof(SourceType)), TargetType>::type
clampTo(SourceType value, TargetType min = defaultMinimumForClamp<TargetType>(), TargetType max = defaultMaximumForClamp<TargetType>())
{
    if (value >= static_cast<SourceType>(max))
        return max;
    if (value <= static_cast<SourceType>(min))
        return min;
    return static_cast<TargetType>(value);
}

template<typename TargetType, typename SourceType>
typename std::enable_if<!std::is_same<TargetType, SourceType>::value
    && std::is_floating_point<SourceType>::value
    && std::is_floating_point<TargetType>::value
    && (sizeof(TargetType) > sizeof(SourceType)), TargetType>::type
clampTo(SourceType value, TargetType min = defaultMinimumForClamp<TargetType>(), TargetType max = defaultMaximumForClamp<TargetType>())
{
    TargetType convertedValue = static_cast<TargetType>(value);
    if (convertedValue >= max)
        return max;
    if (convertedValue <= min)
        return min;
    return convertedValue;
}

// Source and Target have the same sign and Source is larger or equal to Target
template<typename TargetType, typename SourceType>
typename std::enable_if<!std::is_same<TargetType, SourceType>::value
    && std::numeric_limits<SourceType>::is_integer
    && std::numeric_limits<TargetType>::is_integer
    && std::numeric_limits<TargetType>::is_signed == std::numeric_limits<SourceType>::is_signed
    && sizeof(SourceType) >= sizeof(TargetType), TargetType>::type
clampTo(SourceType value, TargetType min = defaultMinimumForClamp<TargetType>(), TargetType max = defaultMaximumForClamp<TargetType>())
{
    if (value >= static_cast<SourceType>(max))
        return max;
    if (value <= static_cast<SourceType>(min))
        return min;
    return static_cast<TargetType>(value);
}

// Clamping a unsigned integer to the max signed value.
template<typename TargetType, typename SourceType>
typename std::enable_if<!std::is_same<TargetType, SourceType>::value
    && std::numeric_limits<SourceType>::is_integer
    && std::numeric_limits<TargetType>::is_integer
    && std::numeric_limits<TargetType>::is_signed
    && !std::numeric_limits<SourceType>::is_signed
    && sizeof(SourceType) >= sizeof(TargetType), TargetType>::type
clampTo(SourceType value)
{
    TargetType max = std::numeric_limits<TargetType>::max();
    if (value >= static_cast<SourceType>(max))
        return max;
    return static_cast<TargetType>(value);
}

// Clamping a signed integer into a valid unsigned integer.
template<typename TargetType, typename SourceType>
typename std::enable_if<!std::is_same<TargetType, SourceType>::value
    && std::numeric_limits<SourceType>::is_integer
    && std::numeric_limits<TargetType>::is_integer
    && !std::numeric_limits<TargetType>::is_signed
    && std::numeric_limits<SourceType>::is_signed
    && sizeof(SourceType) == sizeof(TargetType), TargetType>::type
clampTo(SourceType value)
{
    if (value < 0)
        return 0;
    return static_cast<TargetType>(value);
}

template<typename TargetType, typename SourceType>
typename std::enable_if<!std::is_same<TargetType, SourceType>::value
    && std::numeric_limits<SourceType>::is_integer
    && std::numeric_limits<TargetType>::is_integer
    && !std::numeric_limits<TargetType>::is_signed
    && std::numeric_limits<SourceType>::is_signed
    && (sizeof(SourceType) > sizeof(TargetType)), TargetType>::type
clampTo(SourceType value)
{
    if (value < 0)
        return 0;
    TargetType max = std::numeric_limits<TargetType>::max();
    if (value >= static_cast<SourceType>(max))
        return max;
    return static_cast<TargetType>(value);
}

inline int clampToInteger(double value)
{
    return clampTo<int>(value);
}

inline unsigned clampToUnsigned(double value)
{
    return clampTo<unsigned>(value);
}

inline float clampToFloat(double value)
{
    return clampTo<float>(value);
}

inline int clampToPositiveInteger(double value)
{
    return clampTo<int>(value, 0);
}

inline int clampToInteger(float value)
{
    return clampTo<int>(value);
}

template<typename T>
inline int clampToInteger(T x)
{
    static_assert(std::numeric_limits<T>::is_integer, "T must be an integer.");

    const T intMax = static_cast<unsigned>(std::numeric_limits<int>::max());

    if (x >= intMax)
        return std::numeric_limits<int>::max();
    return static_cast<int>(x);
}

// Explicitly accept 64bit result when clamping double value.
// Keep in mind that double can only represent 53bit integer precisely.
template<typename T> constexpr T clampToAccepting64(double value, T min = defaultMinimumForClamp<T>(), T max = defaultMaximumForClamp<T>())
{
    return (value >= static_cast<double>(max)) ? max : ((value <= static_cast<double>(min)) ? min : static_cast<T>(value));
}

inline bool isWithinIntRange(float x)
{
    return x > static_cast<float>(std::numeric_limits<int>::min()) && x < static_cast<float>(std::numeric_limits<int>::max());
}

inline float normalizedFloat(float value)
{
    if (value > 0 && value < std::numeric_limits<float>::min())
        return std::numeric_limits<float>::min();
    if (value < 0 && value > -std::numeric_limits<float>::min())
        return -std::numeric_limits<float>::min();
    return value;
}

template<typename T> constexpr bool hasOneBitSet(T value)
{
    return !((value - 1) & value) && value;
}

template<typename T> constexpr bool hasZeroOrOneBitsSet(T value)
{
    return !((value - 1) & value);
}

template<typename T> constexpr bool hasTwoOrMoreBitsSet(T value)
{
    return !hasZeroOrOneBitsSet(value);
}

// FIXME: Some Darwin projects shamelessly include WTF headers and don't build with C++14... See: rdar://problem/45395767
// Since C++11 and before don't support constexpr statements we can't mark this function constexpr.
#if !defined(WTF_CPP_STD_VER) || WTF_CPP_STD_VER >= 14
template <typename T> constexpr unsigned getLSBSet(T value)
{
    typedef typename std::make_unsigned<T>::type UnsignedT;
    unsigned result = 0;

    UnsignedT unsignedValue = static_cast<UnsignedT>(value);
    while (unsignedValue >>= 1)
        ++result;

    return result;
}
#endif

template<typename T> inline T divideRoundedUp(T a, T b)
{
    return (a + b - 1) / b;
}

template<typename T> inline T timesThreePlusOneDividedByTwo(T value)
{
    // Mathematically equivalent to:
    //   (value * 3 + 1) / 2;
    // or:
    //   (unsigned)ceil(value * 1.5));
    // This form is not prone to internal overflow.
    return value + (value >> 1) + (value & 1);
}

template<typename T> inline bool isNotZeroAndOrdered(T value)
{
    return value > 0.0 || value < 0.0;
}

template<typename T> inline bool isZeroOrUnordered(T value)
{
    return !isNotZeroAndOrdered(value);
}

template<typename T> inline bool isGreaterThanNonZeroPowerOfTwo(T value, unsigned power)
{
    // The crazy way of testing of index >= 2 ** power
    // (where I use ** to denote pow()).
    return !!((value >> 1) >> (power - 1));
}

template<typename T> constexpr bool isLessThan(const T& a, const T& b) { return a < b; }
template<typename T> constexpr bool isLessThanEqual(const T& a, const T& b) { return a <= b; }
template<typename T> constexpr bool isGreaterThan(const T& a, const T& b) { return a > b; }
template<typename T> constexpr bool isGreaterThanEqual(const T& a, const T& b) { return a >= b; }

#ifndef UINT64_C
#if COMPILER(MSVC)
#define UINT64_C(c) c ## ui64
#else
#define UINT64_C(c) c ## ull
#endif
#endif

#if COMPILER(MINGW64) && (!defined(__MINGW64_VERSION_RC) || __MINGW64_VERSION_RC < 1)
inline double wtf_pow(double x, double y)
{
    // MinGW-w64 has a custom implementation for pow.
    // This handles certain special cases that are different.
    if ((x == 0.0 || std::isinf(x)) && std::isfinite(y)) {
        double f;
        if (modf(y, &f) != 0.0)
            return ((x == 0.0) ^ (y > 0.0)) ? std::numeric_limits<double>::infinity() : 0.0;
    }

    if (x == 2.0) {
        int yInt = static_cast<int>(y);
        if (y == yInt)
            return ldexp(1.0, yInt);
    }

    return pow(x, y);
}
#define pow(x, y) wtf_pow(x, y)
#endif // COMPILER(MINGW64) && (!defined(__MINGW64_VERSION_RC) || __MINGW64_VERSION_RC < 1)


// decompose 'number' to its sign, exponent, and mantissa components.
// The result is interpreted as:
//     (sign ? -1 : 1) * pow(2, exponent) * (mantissa / (1 << 52))
inline void decomposeDouble(double number, bool& sign, int32_t& exponent, uint64_t& mantissa)
{
    ASSERT(std::isfinite(number));

    sign = std::signbit(number);

    uint64_t bits = WTF::bitwise_cast<uint64_t>(number);
    exponent = (static_cast<int32_t>(bits >> 52) & 0x7ff) - 0x3ff;
    mantissa = bits & 0xFFFFFFFFFFFFFull;

    // Check for zero/denormal values; if so, adjust the exponent,
    // if not insert the implicit, omitted leading 1 bit.
    if (exponent == -0x3ff)
        exponent = mantissa ? -0x3fe : 0;
    else
        mantissa |= 0x10000000000000ull;
}

// Calculate d % 2^{64}.
inline void doubleToInteger(double d, unsigned long long& value)
{
    if (std::isnan(d) || std::isinf(d))
        value = 0;
    else {
        // -2^{64} < fmodValue < 2^{64}.
        double fmodValue = fmod(trunc(d), std::numeric_limits<unsigned long long>::max() + 1.0);
        if (fmodValue >= 0) {
            // 0 <= fmodValue < 2^{64}.
            // 0 <= value < 2^{64}. This cast causes no loss.
            value = static_cast<unsigned long long>(fmodValue);
        } else {
            // -2^{64} < fmodValue < 0.
            // 0 < fmodValueInUnsignedLongLong < 2^{64}. This cast causes no loss.
            unsigned long long fmodValueInUnsignedLongLong = static_cast<unsigned long long>(-fmodValue);
            // -1 < (std::numeric_limits<unsigned long long>::max() - fmodValueInUnsignedLongLong) < 2^{64} - 1.
            // 0 < value < 2^{64}.
            value = std::numeric_limits<unsigned long long>::max() - fmodValueInUnsignedLongLong + 1;
        }
    }
}

namespace WTF {

// From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
constexpr uint32_t roundUpToPowerOfTwo(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

constexpr unsigned maskForSize(unsigned size)
{
    if (!size)
        return 0;
    return roundUpToPowerOfTwo(size) - 1;
}

inline unsigned fastLog2(unsigned i)
{
    unsigned log2 = 0;
    if (i & (i - 1))
        log2 += 1;
    if (i >> 16) {
        log2 += 16;
        i >>= 16;
    }
    if (i >> 8) {
        log2 += 8;
        i >>= 8;
    }
    if (i >> 4) {
        log2 += 4;
        i >>= 4;
    }
    if (i >> 2) {
        log2 += 2;
        i >>= 2;
    }
    if (i >> 1)
        log2 += 1;
    return log2;
}

inline unsigned fastLog2(uint64_t value)
{
    unsigned high = static_cast<unsigned>(value >> 32);
    if (high)
        return fastLog2(high) + 32;
    return fastLog2(static_cast<unsigned>(value));
}

template <typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type safeFPDivision(T u, T v)
{
    // Protect against overflow / underflow.
    if (v < 1 && u > v * std::numeric_limits<T>::max())
        return std::numeric_limits<T>::max();
    if (v > 1 && u < v * std::numeric_limits<T>::min())
        return 0;
    return u / v;
}

// Floating point numbers comparison:
// u is "essentially equal" [1][2] to v if: | u - v | / |u| <= e and | u - v | / |v| <= e
//
// [1] Knuth, D. E. "Accuracy of Floating Point Arithmetic." The Art of Computer Programming. 3rd ed. Vol. 2.
//     Boston: Addison-Wesley, 1998. 229-45.
// [2] http://www.boost.org/doc/libs/1_34_0/libs/test/doc/components/test_tools/floating_point_comparison.html
template <typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type areEssentiallyEqual(T u, T v, T epsilon = std::numeric_limits<T>::epsilon())
{
    if (u == v)
        return true;

    const T delta = std::abs(u - v);
    return safeFPDivision(delta, std::abs(u)) <= epsilon && safeFPDivision(delta, std::abs(v)) <= epsilon;
}

// Match behavior of Math.min, where NaN is returned if either argument is NaN.
template <typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type nanPropagatingMin(T a, T b)
{
    return std::isnan(a) || std::isnan(b) ? std::numeric_limits<T>::quiet_NaN() : std::min(a, b);
}

// Match behavior of Math.max, where NaN is returned if either argument is NaN.
template <typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type nanPropagatingMax(T a, T b)
{
    return std::isnan(a) || std::isnan(b) ? std::numeric_limits<T>::quiet_NaN() : std::max(a, b);
}

inline bool isIntegral(float value)
{
    return static_cast<int>(value) == value;
}

template<typename T>
inline void incrementWithSaturation(T& value)
{
    if (value != std::numeric_limits<T>::max())
        value++;
}

template<typename T>
inline T leftShiftWithSaturation(T value, unsigned shiftAmount, T max = std::numeric_limits<T>::max())
{
    T result = value << shiftAmount;
    // We will have saturated if shifting right doesn't recover the original value.
    if (result >> shiftAmount != value)
        return max;
    if (result > max)
        return max;
    return result;
}

// Check if two ranges overlap assuming that neither range is empty.
template<typename T>
inline bool nonEmptyRangesOverlap(T leftMin, T leftMax, T rightMin, T rightMax)
{
    ASSERT(leftMin < leftMax);
    ASSERT(rightMin < rightMax);

    return leftMax > rightMin && rightMax > leftMin;
}

// Pass ranges with the min being inclusive and the max being exclusive. For example, this should
// return false:
//
//     rangesOverlap(0, 8, 8, 16)
template<typename T>
inline bool rangesOverlap(T leftMin, T leftMax, T rightMin, T rightMax)
{
    ASSERT(leftMin <= leftMax);
    ASSERT(rightMin <= rightMax);
    
    // Empty ranges interfere with nothing.
    if (leftMin == leftMax)
        return false;
    if (rightMin == rightMax)
        return false;

    return nonEmptyRangesOverlap(leftMin, leftMax, rightMin, rightMax);
}

// This mask is not necessarily the minimal mask, specifically if size is
// a power of 2. It has the advantage that it's fast to compute, however.
inline uint32_t computeIndexingMask(uint32_t size)
{
    return static_cast<uint64_t>(static_cast<uint32_t>(-1)) >> std::clz(size);
}

constexpr unsigned preciseIndexMaskShiftForSize(unsigned size)
{
    return size * 8 - 1;
}

template<typename T>
constexpr unsigned preciseIndexMaskShift()
{
    return preciseIndexMaskShiftForSize(sizeof(T));
}

template<typename T>
T opaque(T pointer)
{
#if !OS(WINDOWS)
    asm("" : "+r"(pointer));
#endif
    return pointer;
}

// This masks the given pointer with 0xffffffffffffffff (ptrwidth) if `index <
// length`. Otherwise, it masks the pointer with 0. Similar to Linux kernel's array_ptr.
template<typename T>
inline T* preciseIndexMaskPtr(uintptr_t index, uintptr_t length, T* value)
{
    uintptr_t result = bitwise_cast<uintptr_t>(value) & static_cast<uintptr_t>(
        static_cast<intptr_t>(index - opaque(length)) >>
        static_cast<intptr_t>(preciseIndexMaskShift<T*>()));
    return bitwise_cast<T*>(result);
}

template<typename VectorType, typename RandomFunc>
void shuffleVector(VectorType& vector, size_t size, const RandomFunc& randomFunc)
{
    for (size_t i = 0; i + 1 < size; ++i)
        std::swap(vector[i], vector[i + randomFunc(size - i)]);
}

template<typename VectorType, typename RandomFunc>
void shuffleVector(VectorType& vector, const RandomFunc& randomFunc)
{
    shuffleVector(vector, vector.size(), randomFunc);
}

inline unsigned clz32(uint32_t number)
{
#if COMPILER(GCC_COMPATIBLE)
    if (number)
        return __builtin_clz(number);
    return 32;
#elif COMPILER(MSVC)
    // Visual Studio 2008 or upper have __lzcnt, but we can't detect Intel AVX at compile time.
    // So we use bit-scan-reverse operation to calculate clz.
    unsigned long ret = 0;
    if (_BitScanReverse(&ret, number))
        return 31 - ret;
    return 32;
#else
    unsigned zeroCount = 0;
    for (int i = 31; i >= 0; i--) {
        if (!(number >> i))
            zeroCount++;
        else
            break;
    }
    return zeroCount;
#endif
}

inline unsigned clz64(uint64_t number)
{
#if COMPILER(GCC_COMPATIBLE)
    if (number)
        return __builtin_clzll(number);
    return 64;
#elif COMPILER(MSVC) && !CPU(X86)
    // Visual Studio 2008 or upper have __lzcnt, but we can't detect Intel AVX at compile time.
    // So we use bit-scan-reverse operation to calculate clz.
    // _BitScanReverse64 is defined in X86_64 and ARM in MSVC supported environments.
    unsigned long ret = 0;
    if (_BitScanReverse64(&ret, number))
        return 63 - ret;
    return 64;
#else
    unsigned zeroCount = 0;
    for (int i = 63; i >= 0; i--) {
        if (!(number >> i))
            zeroCount++;
        else
            break;
    }
    return zeroCount;
#endif
}

inline unsigned ctz32(uint32_t number)
{
#if COMPILER(GCC_COMPATIBLE)
    if (number)
        return __builtin_ctz(number);
    return 32;
#elif COMPILER(MSVC) && !CPU(X86)
    unsigned long ret = 0;
    if (_BitScanForward(&ret, number))
        return ret;
    return 32;
#else
    unsigned zeroCount = 0;
    for (unsigned i = 0; i < 32; i++) {
        if (number & 1)
            break;

        zeroCount++;
        number >>= 1;
    }
    return zeroCount;
#endif
}

} // namespace WTF

using WTF::opaque;
using WTF::preciseIndexMaskPtr;
using WTF::preciseIndexMaskShift;
using WTF::preciseIndexMaskShiftForSize;
using WTF::shuffleVector;
using WTF::clz32;
using WTF::clz64;
using WTF::ctz32;
