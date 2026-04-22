//----------------------------------------------------------------------------------------
/**
 * \file       spline.h
 * \author     bielevan
 * \date       2025
 * \brief      Functions and data resposible for curve animation 
*/
//----------------------------------------------------------------------------------------
#ifndef __SPLINE_H
#define __SPLINE_H

#include "pgr.h" // glm support

extern glm::vec3 flyingCurve[];
extern size_t curveSize;

// Checks whether vector is zero-length or not.
bool isVectorNull(const glm::vec3 &vect);



// Computes the transformation matrix that aligns an object with a given direction.
glm::mat4 alignObject(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up);

// Template, wraps a value cyclically within a [minBound, maxBound) range.
// good for looping animations.
template <typename T>
T cyclic_clamp(const T value, const T minBound, const T maxBound) {
    T amp = maxBound - minBound;
    T val = fmod(value - minBound, amp);
    if (val < T(0))
        val += amp;
    return val + minBound;
}

// Evaluates a point on a spline segment using Catmull-Rom interpolation.
glm::vec3 evaluateCurveSegment(
    const glm::vec3& P0,
    const glm::vec3& P1,
    const glm::vec3& P2,
    const glm::vec3& P3,
    const float t
);

// Computes the first derivative (tangent vector) of a spline segment
glm::vec3 evaluateCurveSegment_1stDerivative(
    const glm::vec3& P0,
    const glm::vec3& P1,
    const glm::vec3& P2,
    const glm::vec3& P3,
    const float t
);

// Evaluates a closed Catmull-Rom spline (looped) at a given t in [0, count)
glm::vec3 evaluateClosedCurve(
    const glm::vec3 points[],
    const size_t    count,
    const float     t
);

// Computes the first derivative of a closed curve at parameter t
glm::vec3 evaluateClosedCurve_1stDerivative(
    const glm::vec3 points[],
    const size_t    count,
    const float     t
);

#endif // __SPLINE_H
