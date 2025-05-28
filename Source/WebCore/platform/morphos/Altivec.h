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
 
#pragma once
#include <cstddef>
#include <sys/types.h>

namespace WebCore {

class Altivec
{
public:
	static void muxFloatAudioChannelsToInterleavedInt16(int16_t *out, const float *channelA, const float *channelB, size_t samples);

	// in 8-floats
	static void clamp(float* outputVector, float minimum, float maximum, const float* inputVector, size_t numberOfElementsToProcess);
	static void multiplyByScalarThenAddToOutput(const float* inputVector, float scalar, float* outputVector, size_t numberOfElementsToProcess);

	// in 4-floats
	static void multiplyByScalar(const float* inputVector, float scalar, float* outputVector, size_t numberOfElementsToProcess);
	static void addScalar(const float* inputVector, float scalar, float* outputVector, size_t numberOfElementsToProcess);
	static void add(const float* inputVector1, const float* inputVector2, float* outputVector, size_t numberOfElementsToProcess);
	static void multiply(const float* inputVector1, const float* inputVector2, float* outputVector, size_t numberOfElementsToProcess);
};

}
