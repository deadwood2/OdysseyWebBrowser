//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Vector:
//   Vector class for linear math.
//

#ifndef UTIL_VECTOR_H
#define UTIL_VECTOR_H

#include <ostream>

#include <export.h>

struct ANGLE_EXPORT Vector2
{
    Vector2();
    Vector2(float x, float y);

    bool operator==(const Vector2 &vec) const;
    bool operator!=(const Vector2 &vec) const;

    static float length(const Vector2 &vec);
    static float lengthSquared(const Vector2 &vec);

    static Vector2 normalize(const Vector2 &vec);

    float *data() { return &x; }
    const float *data() const { return &x; }

    float x, y;
};

ANGLE_EXPORT std::ostream &operator<<(std::ostream &stream, const Vector2 &vec);

struct ANGLE_EXPORT Vector3
{
    Vector3();
    Vector3(float x, float y, float z);

    static float length(const Vector3 &vec);
    static float lengthSquared(const Vector3 &vec);

    static Vector3 normalize(const Vector3 &vec);

    static float dot(const Vector3 &a, const Vector3 &b);
    static Vector3 cross(const Vector3 &a, const Vector3 &b);

    float *data() { return &x; }
    const float *data() const { return &x; }

    float x, y, z;
};

ANGLE_EXPORT Vector3 operator*(const Vector3 &a, const Vector3 &b);
ANGLE_EXPORT Vector3 operator*(const Vector3 &a, const float &b);
ANGLE_EXPORT Vector3 operator/(const Vector3 &a, const Vector3 &b);
ANGLE_EXPORT Vector3 operator/(const Vector3 &a, const float &b);
ANGLE_EXPORT Vector3 operator+(const Vector3 &a, const Vector3 &b);
ANGLE_EXPORT Vector3 operator-(const Vector3 &a, const Vector3 &b);
ANGLE_EXPORT bool operator==(const Vector3 &a, const Vector3 &b);
ANGLE_EXPORT bool operator!=(const Vector3 &a, const Vector3 &b);

struct ANGLE_EXPORT Vector4
{
    Vector4();
    Vector4(float x, float y, float z, float w);

    static float length(const Vector4 &vec);
    static float lengthSquared(const Vector4 &vec);

    static Vector4 normalize(const Vector4 &vec);

    static float dot(const Vector4 &a, const Vector4 &b);

    float *data() { return &x; }
    const float *data() const { return &x; }

    float x, y, z, w;
};

#endif  // UTIL_VECTOR_H
