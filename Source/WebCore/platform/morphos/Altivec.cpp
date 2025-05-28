/*
 * Copyright (C) 2020-2022 Jacek Piszczek
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
 
 #include "Altivec.h"
#include <altivec.h>
#define vector __vector

extern "C" { void dprintf(const char *, ...); }

namespace WebCore {

void Altivec::muxFloatAudioChannelsToInterleavedInt16(int16_t *out, const float *inA, const float *inB, size_t samples)
{
	size_t i;
	for (i = 0; i + 4 <= samples; i += 4, inA += 4, inB += 4, out += 8)
	{
		vector float input0 = vec_ld( 0, inA);
		vector float input1 = vec_ld( 0, inB);
		vector float merge0 = vec_mergeh(input0, input1);
		vector float merge1 = vec_mergel(input0, input1);
		vector signed int result0 = vec_cts(merge0, 15);
		vector signed int result1 = vec_cts(merge1, 15);
		vec_st(vec_packs(result0, result1), 0, out);
	}
}

#define TYPE float
typedef vector unsigned char vUInt8;

static inline vector TYPE vec_loadAndSplatScalar( TYPE *scalarPtr )
{
	vUInt8 splatMap = vec_lvsl( 0, scalarPtr );
	vector TYPE result = vec_lde( 0, scalarPtr );
	splatMap = (vUInt8) vec_splat( (vector TYPE) splatMap, 0 );

	return vec_perm( result, result, splatMap );
}

void Altivec::clamp(float* outputVector, float minimum, float maximum, const float* inputVector, size_t numberOfElementsToProcess)
{
	vector float vmin = vec_loadAndSplatScalar(&minimum);
	vector float vmax = vec_loadAndSplatScalar(&maximum);
	size_t i;

	for (i = 0; i + 8 <= numberOfElementsToProcess; i += 8, inputVector += 8, outputVector += 8)
	{
		vector float input0 = vec_ld( 0, inputVector);
		vector float input1 = vec_ld(16, inputVector);
		input0 = vec_min(input0, vmax);
		input1 = vec_min(input1, vmax);
		input0 = vec_max(input0, vmin);
		input1 = vec_max(input1, vmin);
		vec_st(input0, 0, outputVector);
		vec_st(input1,16, outputVector);
	}
}

void Altivec::multiplyByScalarThenAddToOutput(const float* inputVector, float scalar, float* outputVector, size_t numberOfElementsToProcess)
{
	vector float vscalar = vec_loadAndSplatScalar(&scalar);
	size_t i;

	for (i = 0; i + 8 <= numberOfElementsToProcess; i += 8, inputVector += 8, outputVector += 8)
	{
		vector float input0 = vec_ld( 0, inputVector);
		vector float input1 = vec_ld(16, inputVector);
		vector float output0 = vec_ld(0, outputVector);
		vector float output1 = vec_ld(16, outputVector);
		output0 = vec_madd(input0, vscalar, output0);
		output1 = vec_madd(input1, vscalar, output1);
		vec_st(output0, 0, outputVector);
		vec_st(output1,16, outputVector);
	}
}

void Altivec::multiplyByScalar(const float* inputVector, float scalar, float* outputVector, size_t numberOfElementsToProcess)
{
	vector float vscalar = vec_loadAndSplatScalar(&scalar);
	float zero = 0.f;
	vector float vzero = vec_loadAndSplatScalar(&zero);
	size_t i;

	for (i = 0; i + 4 <= numberOfElementsToProcess; i += 4, inputVector += 4, outputVector += 4)
	{
		vector float input = vec_ld( 0, inputVector);
		input = vec_madd(input, vscalar, vzero);
		vec_st(input, 0, outputVector);
	}
}

void Altivec::addScalar(const float* inputVector, float scalar, float* outputVector, size_t numberOfElementsToProcess)
{
	vector float vscalar = vec_loadAndSplatScalar(&scalar);
	size_t i;

	for (i = 0; i + 4 <= numberOfElementsToProcess; i += 4, inputVector += 4, outputVector += 4)
	{
		vector float input = vec_ld( 0, inputVector);
		input = vec_add(input, vscalar);
		vec_st(input, 0, outputVector);
	}
}

void Altivec::add(const float* inputVector1, const float* inputVector2, float* outputVector, size_t numberOfElementsToProcess)
{
	size_t i;

	for (i = 0; i + 4 <= numberOfElementsToProcess; i += 4, inputVector1 += 4, inputVector2 += 4, outputVector += 4)
	{
		vector float input1 = vec_ld( 0, inputVector1);
		vector float input2 = vec_ld( 0, inputVector2);
		vec_st(vec_add(input1, input2), 0, outputVector);
	}
}

void Altivec::multiply(const float* inputVector1, const float* inputVector2, float* outputVector, size_t numberOfElementsToProcess)
{
	float zero = 0.f;
	vector float vzero = vec_loadAndSplatScalar(&zero);
	size_t i;

	for (i = 0; i + 4 <= numberOfElementsToProcess; i += 4, inputVector1 += 4, inputVector2 += 4, outputVector += 4)
	{
		vector float input1 = vec_ld( 0, inputVector1);
		vector float input2 = vec_ld( 0, inputVector2);
		vec_st(vec_madd(input1, input2, vzero), 0, outputVector);
	}
}

}
