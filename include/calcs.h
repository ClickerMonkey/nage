#pragma once

#include <cmath>
#include <vector>
#include <limits>

#include "debug.h"
#include "types.h"

// The calculator namespace is used to define math operations on types. 
// Calculators can also be defined and retrieved given a types::Type.
// For most operations just specifying two speciations (Adds, Mul) is enough.
namespace calc {

    // How close to a number do we consider two floating precision numbers equal?
    float EPSILON = 0.00001f;

    // Returns v but no larger than the max or smaller than the min.
    template<typename T>
    T Clamp(T v, T min, T max) {
        return v < min ? min : v > max ? max : v;
    }

    // Computes the quadratic formula between a, b, and c. If it can't be computed
    // then none is returned.
    float QuadraticFormula(float a, float b, float c, float none) {
        auto t0 = std::numeric_limits<float>::min();
        auto t1 = std::numeric_limits<float>::min();

        if (abs(a) < EPSILON) {
            if (abs(b) < EPSILON) {
                if (abs(c) < EPSILON) {
                    t0 = 0.0f;
                    t1 = 0.0f;
                }
            } else {
                t0 = -c / b;
                t1 = -c / b;
            }
        } else {
            auto disc = b * b - 4.0f * a * c;

            if (disc >= 0) {
                disc = sqrtf(disc);
                a = 2.0f * a;
                t0 = (-b - disc) / a;
                t1 = (-b + disc) / a;
            }
        }

        if (t0 != std::numeric_limits<float>::min()) {
            auto t = t0 < t1 ? t0 : t1;

            if (t < 0) {
                t = t0 > t1 ? t0 : t1;
            }

            if (t > 0) {
                return t;
            }
        }

        return none;
    }
    // Calculates the height triangle given it's base length and two sides.
    inline float GetTriangleHeight(float base, float side1, float side2) {
        auto p = (base + side1 + side2) * 0.5f;
        auto area = sqrtf(p * (p - base) * (p - side1) * (p - side2));
        auto height = area * 2.0f / base;

        return height;
    }

    //============================================================================
    // Functions you can specialize per type.
    //============================================================================

    // Returns a + b * scale
    // A specialization needs to be specified if + and * operators are not implemented on the type.
    template<typename T>
    constexpr T Adds(const T& a, const T& b, float scale) {
        return a + b * scale;
    }
    // Returns a * b
    // A specialization needs to be specified if the * operator are not implemented on the type.
    template<typename T>
    constexpr T Mul(const T& a, const T& b) {
        return a * b;
    }
    // Returns a / b
    // A specialization should be specified if the / operator are not implemented on the type.
    template<typename T>
    constexpr T Div(const T& a, const T& b) {
        return b == 0 ? 0 : a / b;
    }
    // Returns a[n]*b[n] + ...
    template<typename T>
    constexpr float Dot(const T&a, const T& b) {
        return a * b;
    }
    // Returns a + b
    template<typename T>
    constexpr T Add(const T& a, const T& b) {
        return Adds<T>(a, b, 1.0f);
    }
    // Returns a - b
    template<typename T>
    constexpr T Sub(const T& a, const T& b) {
        return Adds<T>(a, b, -1.0f);
    }
    // Returns a * scale
    template<typename T>
    constexpr T Scale(const T& a, float scale) {
        return Adds<T>(a, a, scale - 1.0f);
    }
    // Returns a + (b - a) * d
    template<typename T>
    constexpr T Lerp(const T& a, const T& b, float d) {
        return Adds<T>(a, Adds<T>(b, a, -1.0f), d);
    }
    // Returns the squared length of a
    template<typename T>
    constexpr float LengthSq(const T& a) {
        return Dot<T>(a, a);
    }
    // Returns the length of a
    template<typename T>
    constexpr float Length(const T& a) {
        return sqrtf(Dot<T>(a, a));
    }
    // Returns the squared distance between a andb
    template<typename T>
    constexpr float DistanceSq(const T& a, const T& b) {
        auto diff = Sub<T>(a, b);
        return Dot<T>(diff, diff);
    }
    // Returns the length of a
    template<typename T>
    constexpr float Distance(const T& a, const T& b) {
        return sqrtf(DistanceSq<T>(a, b));
    }
    // Returns a < b
    template<typename T>
    constexpr bool IsLess(const T& a, const T& b) {
        return a < b;
    }
    // Returns the number of components for the given type.
    // A specialization needs to be specified if the type is a non-scalar type.
    template<typename T>
    constexpr int Components() {
        return 1;
    }
    // Returns the value for a component at a given component index.
    // A specialization needs to be specified if the type is a non-scalar type.
    template<typename T>
    constexpr float Get(const T& a, int index) {
        return static_cast<float>(a);
    }
    // Sets the value for a component at a given component index.
    // A specialization needs to be specified if the type is a non-scalar type.
    template<typename T>
    constexpr void Set(T& out, int index, float value) {
        out = static_cast<T>(value);
    }
    // Sets all components in out to the given value.
    template<typename T>
    constexpr void SetAll(T& out, float value) {
        for (int i = 0; i < Components<T>(); i++) {
            Set<T>(out, i, value);
        }
    }
    // Zeros out the value.
    template<typename T>
    constexpr void Zero(T& out) {
        SetAll<T>(out, 0.0f);
    }
    // Returns a zero value.
    template<typename T>
    constexpr T Create() {
        return T();
    }

    //============================================================================
    // Utility functions. No need to specialize, but the ability is there.
    //============================================================================

    // Sets the length of the value
    template<typename T>
    constexpr T Lengthen(const T& v, float length) {
        auto lenSq = LengthSq<T>(v);
        if (lenSq != 0 && lenSq != length * length) {
            return Scale<T>(v, length / sqrtf(lenSq));
        } else {
            return v;
        }
    }
    // CLamps the length of the value
    template<typename T>
    constexpr T ClampLength(const T& v, float min, float max) {
        auto lenSq = LengthSq<T>(v);
        if (lenSq != 0) {
            if (lenSq < min * min) {
                return Scale<T>(v, min / sqrtf(lenSq));
            } else if (lenSq > max * max) {
                return Scale<T>(v, max / sqrtf(lenSq));
            }
        }
        return v;
    }
    // Performs a slerp between the start and end value given the curves "angle" between the two values and a t value.
    // When t is 0 the returned value will equal start and when 1 will match end. Angle is in radians.
    template<typename T>
    T SlerpAngle(const T& start, const T& end, float angle, float t) {
        if (angle == 0) {
            return start;
        }
        auto denom = Div<float>(1, sinf(angle));
        auto d0 = sinf((1.0f - t) * angle) * denom;
        auto d1 = sinf(t * angle) * denom;

        return Adds<T>(Scale<T>(end, d1), start, d0);
    }
    // Performs a slerp between two values. Imagine the two values define a curve that lies
    // on an ellipse where the zero value is the center. The returned value will exist on that
    // curve where 0 is at start and 1 is at end and 0.5 is halfway between.
    template<typename T>
    T Slerp(const T& start, const T& end, float t) {
        auto slength = Length<T>(start);
        auto elength = Length<T>(end);
        auto lengthSq = slength * elength;
        if (lengthSq == 0) {
            return start;
        }
        auto angle = acosf(Dot<T>(start, end) / lengthSq);

        return SlerpAngle<T>(start, end, angle, t);
    }
    // Performs a slerp between two normals given a t. The returned value will exist on the shortest
    // curve from the end of start and end and t between those two values (t is 0 to 1).
    template<typename T>
    inline T SlerpNormal(const T& start, const T& end, float t) {
        auto angle = acosf(Dot<T>(start, end));

        return SlerpAngle<T>(start, end, angle, t);
    }
    // A line is defined by start and end and there is a point on it that's closest to the given point.
    // The delta value defines how close that casted point is between start and end where 0 is on start,
    // 1 is on end, 0.5 is halfway between, and -1 is before start on the line the same distance
    // between start and end.
    template<typename T>
    float Delta(const T& start, const T& end, const T& point) {
        auto p0 = Sub<T>(end, start);
        auto p1 = Sub<T>(point, start);

        return Div<float>(Dot<T>(p0, p1), LengthSq<T>(p0));
    }
    // Returns the closest value to the given point on the line or segment defined by start and end.
    template<typename T>
    T Closest(const T& start, const T& end, const T& point, bool line) {
        auto delta = Delta<T>(start, end, point);
        if (!line) {
            delta = Clamp<float>(delta, 0, 1);
        }
        return Lerp<T>(start, end, delta);
    }
    // Sets normal to the given value but with a length of 1 or 0.
    template<typename T>
    inline float Normalize(T& value) {
        auto d = LengthSq<T>(value);
        if (d != 0 && d != 1) {
            d = sqrtf(d);
            value = Scale<T>(value, 1 / d);
        }
        return d;
    }
    // Returns if the given value is normalized (has a length of 1).
    template<typename T>
    inline bool IsNormal(const T& value) {
        return fabs(LengthSq<T>(value)-1) < EPSILON;
    }
    // Returns the shortest distance from the point to the line or segment defined by start and end.
    template<typename T>
    float DistanceFrom(const T& start, const T& end, const T& point, bool line) {
        auto closest = Closest<T>(start, end, point, line);
        return Distance<T>(point, closest);
    }
    // Returns true if the point is in the defined conical view.
    template<typename T>
    inline bool IsPointInView(const T& viewOrigin, const T& viewDirection, float fovCos, const T& point) {
        return Dot<T>(Sub<T>(point, viewOrigin), viewDirection) > fovCos;
    }
    // Returns true if the circle is fully or partially in the defined conical view.
    template<typename T>
    bool IsCircleInView(const T& viewOrigin, const T& viewDirection, float fovTan, float fovCos, const T& circle, float circleRadius, bool entirely) {
        // http://www.cbloom.com/3d/techdocs/culling.txt
        auto circleToOrigin = Sub<T>(circle, viewOrigin);
        auto distanceAlongDirection = Dot<T>(circleToOrigin, viewDirection);
        auto coneRadius = distanceAlongDirection * fovTan;
        auto distanceFromAxis = sqrtf(LengthSq<T>(circleToOrigin) - distanceAlongDirection * distanceAlongDirection);
        auto distanceFromCenterToCone = distanceFromAxis - coneRadius;
        auto shortestDistance = distanceFromCenterToCone * fovCos;

        if (entirely) {
            shortestDistance += circleRadius;
        } else {
            shortestDistance -= circleRadius;
        }

        return shortestDistance <= 0;
    }
    // Field of view controls for IsCircleInViewType
    enum class FieldOfView { ignore, half, full };
    // Returns true if the circle is in the defined view based on FOV option.
    template<typename T>
    bool IsCircleInViewType(const T& viewOrigin, const T& viewDirection, float fovTan, float fovCos, const T& circle, float circleRadius, FieldOfView fovType) {
        if (fovType == FieldOfView::ignore) {
            return true;
        }

        if (fovType == FieldOfView::half) {
            circleRadius = 0;
        }

        return IsCircleInView<T>(viewOrigin, viewDirection, fovTan, fovCos, circle, circleRadius, fovType == FieldOfView::full);
    }
    // Calculates the value on the cubic curve given a delta between 0 and 1, the 4 control points, the matrix weights, and if its an inverse.
    template<typename T>
    T CubicCurve(float delta, const T& p0, const T& p1, const T& p2, const T& p3, float matrix[4][4], bool inverse) {
        auto d0 = 1.0f;
        auto d1 = delta;
        auto d2 = d1 * d1;
        auto d3 = d2 * d1;
        float ts[4] = {d0, d1, d2, d3};
        if (inverse) {
            ts[0] = d3;
            ts[1] = d2;
            ts[2] = d1;
            ts[3] = d0;
        }
        auto out = Create<T>();

        for (int i = 0; i < 4; i++) {
            auto temp = Scale<T>(p0, matrix[i][0]);
            temp = Adds<T>(temp, p1, matrix[i][1]);
            temp = Adds<T>(temp, p2, matrix[i][2]);
            temp = Adds<T>(temp, p3, matrix[i][3]);
            out = Adds<T>(out, temp, ts[i]);
        }
        return out;
    }
    // Calculates a parametric cubic curve given a delta between 0 and 1, the points of the curve, the weights, if its inversed, and if the curve loops.
    template<typename T>
    T ParametricCubicCurve(float delta, const std::vector<T>& points, float matrix[4][4], float weight, bool inverse, bool loop) {
        auto n = points.size() - 1;
        auto a = delta * (float)n;
        auto i = Clamp<float>(floorf(a), 0, (float)(n-1));
        auto d = a - i;
        auto index = (int)i;

        auto p0 = i == 0
            ? loop ? points[n] : points[0]
            : points[index - 1];
        auto p1 = points[index];
        auto p2 = points[index+1];
        auto p3 = index == n - 1
            ? !loop ? points[n] : points[0]
            : points[index + 2];

        auto out = CubicCurve<T>(d, p0, p1, p2, p3, matrix, inverse);
        auto scaled = Scale<T>(out, weight);
        return scaled;
    }
    // Calculates the time an interceptor could intercept a target given the interceptors
    // position and possible speed and the targets current position and velocity. If no
    // intercept exists based on the parameters then -1 is returned. Otherwise a value is
    // returned that can be used to calculate the interception point (targetPosition+(targetVelocity*time)).
    template<typename T>
    float InterceptTime(const T& interceptor, float interceptorSpeed, const T& targetPosition, const T& targetVelocity) {
        auto tvec = Sub<T>(targetPosition, interceptor);
        
        auto a = LengthSq<T>(targetVelocity) - (interceptorSpeed * interceptorSpeed);
        auto b = 2 * Dot<T>(targetVelocity, tvec);
        auto c = LengthSq<T>(tvec);

        return QuadraticFormula(a, b, c, -1);
    }
    // Reflects the direction across a given normal. Imagine the normal is on a plane
    // pointing away from it and a reflection is a ball with the given direction bouncing off of it.
    template<typename T>
    inline T Reflect(const T& dir, const T& normal) {
        auto scale = 2.0f * Dot<T>(dir, normal);
        return Adds<T>(dir, normal, -scale);
    }
    // Refracts the direction across the given normal. Like reflect except through the
    // plane defined by the normal.
    template<typename T>
    inline T Refract(const T& dir, const T& normal) {
        auto scale = 2.0f * Dot<T>(dir, normal);
        return Sub<T>(Scale<T>(normal, scale), dir);
    }

    // A calculator for types::Value but calls the functions and specializations above.
    class ValueCalculator {
    public:
        virtual types::Value Add(const types::Value& a, const types::Value& b) = 0;
        virtual types::Value Sub(const types::Value& a, const types::Value& b) = 0;
        virtual types::Value Adds(const types::Value& a, const types::Value& b, float scale) = 0;
        virtual types::Value Mul(const types::Value& a, const types::Value& b) = 0;
        virtual types::Value Div(const types::Value& a, const types::Value& b) = 0;
        virtual types::Value Scale(const types::Value& a, float scale) = 0;
        virtual types::Value Lerp(const types::Value& a, const types::Value& b, float d) = 0;
        virtual float Dot(const types::Value& a, const types::Value& b) = 0;
        virtual float LengthSq(const types::Value& a) = 0;
        virtual float Length(const types::Value& a) = 0;
        virtual float DistanceSq(const types::Value& a, const types::Value& b) = 0;
        virtual float Distance(const types::Value& a, const types::Value& b) = 0;
        virtual bool IsLess(const types::Value& a, const types::Value& b) = 0;
        virtual int Components() = 0;
        virtual float Get(const types::Value& a, int index) = 0;
        virtual void Set(types::Value& out, int index, float value) = 0;
        virtual void SetAll(types::Value& out, float value) = 0;
        virtual void Zero(types::Value& out) = 0;
        virtual types::Value Create() = 0;
        virtual types::Value Lengthen(const types::Value& a, float length) = 0;
        virtual types::Value ClampLength(const types::Value& a, float min, float max) = 0;
        virtual types::Value SlerpAngle(const types::Value& start, const types::Value& end, float angle, float t) = 0;
        virtual types::Value Slerp(const types::Value& start, const types::Value& end, float t) = 0;
        virtual types::Value SlerpNormal(const types::Value& start, const types::Value& end, float t) = 0;
        virtual types::Value Reflect(const types::Value& dir, const types::Value& normal) = 0;
        virtual types::Value Refract(const types::Value& dir, const types::Value& normal) = 0;
        virtual float Delta(const types::Value& start, const types::Value& end, const types::Value& point) = 0;
        virtual types::Value Closest(const types::Value& start, const types::Value& end, const types::Value& point, bool line) = 0;
        virtual float Normalize(types::Value& value) = 0;
        virtual bool IsNormal(const types::Value& a) = 0;
        virtual float DistanceFrom(const types::Value& start, const types::Value& end, const types::Value& point, bool line) = 0;
        virtual bool IsPointInView(const types::Value& viewOrigin, const types::Value& viewDirection, float fovCos, const types::Value& point) = 0;
        virtual bool IsCircleInView(const types::Value& viewOrigin, const types::Value& viewDirection, float fovTan, float fovCos, const types::Value& circle, float circleRadius, bool entirely) = 0;
        virtual bool IsCircleInViewType(const types::Value& viewOrigin, const types::Value& viewDirection, float fovTan, float fovCos, const types::Value& circle, float circleRadius, FieldOfView fovType) = 0;
        virtual float InterceptTime(const types::Value& interceptor, float interceptorSpeed, const types::Value& targetPosition, const types::Value& targetVelocity) = 0;
        virtual types::Value CubicCurve(float delta, const types::Value& p0, const types::Value& p1, const types::Value& p2, const types::Value& p3, float matrix[4][4], bool inverse) = 0;
        virtual types::Value ParametricCubicCurve(float delta, const std::vector<types::Value>& points, float matrix[4][4], float weight, bool inverse, bool loop) = 0;
    };

    // A ValueCalculator for the given type.
    template<typename T>
    class TypedCalculator : public ValueCalculator {
    public:
        TypedCalculator(types::Type* type): m_type(type) {}

        types::Value Add(const types::Value& a, const types::Value& b) {
            return types::ValueOf(calc::Add<T>(a.Get<T>(), b.Get<T>()), m_type);
        }
        types::Value Sub(const types::Value& a, const types::Value& b) {
            return types::ValueOf(calc::Sub<T>(a.Get<T>(), b.Get<T>()), m_type);
        }
        types::Value Adds(const types::Value& a, const types::Value& b, float scale) {
            return types::ValueOf(calc::Adds<T>(a.Get<T>(), b.Get<T>(), scale), m_type);
        }
        types::Value Mul(const types::Value& a, const types::Value& b) {
            return types::ValueOf(calc::Mul<T>(a.Get<T>(), b.Get<T>()), m_type);
        }
        types::Value Div(const types::Value& a, const types::Value& b) {
            return types::ValueOf(calc::Div<T>(a.Get<T>(), b.Get<T>()), m_type);
        }
        types::Value Scale(const types::Value& a, float scale) {
            return types::ValueOf(calc::Scale<T>(a.Get<T>(), scale), m_type);
        }
        types::Value Lerp(const types::Value& a, const types::Value& b, float d) {
            return types::ValueOf(calc::Lerp<T>(a.Get<T>(), b.Get<T>(), d), m_type);
        }
        float Dot(const types::Value& a, const types::Value& b) {
            return calc::Dot<T>(a.Get<T>(), b.Get<T>());
        }
        float LengthSq(const types::Value& a) {
            return calc::LengthSq<T>(a.Get<T>());
        }
        float Length(const types::Value& a) {
            return calc::Length<T>(a.Get<T>());
        }
        float DistanceSq(const types::Value& a, const types::Value& b) {
            return calc::DistanceSq(a.Get<T>(), b.Get<T>());
        }
        float Distance(const types::Value& a, const types::Value& b) {
            return calc::Distance(a.Get<T>(), b.Get<T>());
        }
        bool IsLess(const types::Value& a, const types::Value& b) {
            return calc::IsLess(a.Get<T>(), b.Get<T>());
        }
        int Components() {
            return calc::Components<T>();
        }
        float Get(const types::Value& a, int index) {
            return calc::Get<T>(a.Get<T>(), index);
        }
        void Set(types::Value& out, int index, float value) {
            auto p = out.Ptr<T>();
            if (p != nullptr) {
                calc::Set<T>(*p, index, value);
            }
        }
        void SetAll(types::Value& out, float value) {
            auto p = out.Ptr<T>();
            if (p != nullptr) {
                calc::SetAll<T>(*p, value);
            }
        }
        void Zero(types::Value& out) {
            auto p = out.Ptr<T>();
            if (p != nullptr) {
                calc::Zero<T>(*p);
            }
        }
        types::Value Create() {
            return types::ValueOf(calc::Create<T>(), m_type);
        }
        types::Value Lengthen(const types::Value& a, float length) {
            return types::ValueOf(calc::Lengthen<T>(a.Get<T>(), length), m_type);
        }
        types::Value ClampLength(const types::Value& a, float min, float max) {
            return types::ValueOf(calc::ClampLength<T>(a.Get<T>(), min, max), m_type);
        }
        types::Value SlerpAngle(const types::Value& start, const types::Value& end, float angle, float t) {
            return types::ValueOf(calc::SlerpAngle<T>(start.Get<T>(), end.Get<T>(), angle, t), m_type);
        }
        types::Value Slerp(const types::Value& start, const types::Value& end, float t) {
            return types::ValueOf(calc::Slerp<T>(start.Get<T>(), end.Get<T>(), t), m_type);
        }
        types::Value SlerpNormal(const types::Value& start, const types::Value& end, float t) {
            return types::ValueOf(calc::SlerpNormal<T>(start.Get<T>(), end.Get<T>(), t), m_type);
        }
        types::Value Reflect(const types::Value& dir, const types::Value& normal) {
            return types::ValueOf(calc::Reflect<T>(dir.Get<T>(), normal.Get<T>()), m_type);
        }
        types::Value Refract(const types::Value& dir, const types::Value& normal) {
            return types::ValueOf(calc::Refract<T>(dir.Get<T>(), normal.Get<T>()), m_type);
        }
        float Delta(const types::Value& start, const types::Value& end, const types::Value& point) {
            return calc::Delta(start.Get<T>(), end.Get<T>(), point.Get<T>());
        }
        types::Value Closest(const types::Value& start, const types::Value& end, const types::Value& point, bool line) {
            return types::ValueOf(calc::Closest<T>(start.Get<T>(), end.Get<T>(), point.Get<T>(), line), m_type);
        }
        float Normalize(types::Value& value) {
            auto p = value.Ptr<T>();
            if (p != nullptr) {
                return calc::Normalize<T>(*p);
            }
            return 0;
        }
        bool IsNormal(const types::Value& a) {
            return calc::IsNormal(a.Get<T>());
        }
        float DistanceFrom(const types::Value& start, const types::Value& end, const types::Value& point, bool line) {
            return calc::DistanceFrom(start.Get<T>(), end.Get<T>(), point.Get<T>(), line);
        }
        bool IsPointInView(const types::Value& viewOrigin, const types::Value& viewDirection, float fovCos, const types::Value& point) {
            return calc::IsPointInView(viewOrigin.Get<T>(), viewDirection.Get<T>(), fovCos, point.Get<T>());
        }
        bool IsCircleInView(const types::Value& viewOrigin, const types::Value& viewDirection, float fovTan, float fovCos, const types::Value& circle, float circleRadius, bool entirely) {
            return calc::IsCircleInView(viewOrigin.Get<T>(), viewDirection.Get<T>(), fovTan, fovCos, circle.Get<T>(), circleRadius, entirely);
        }
        bool IsCircleInViewType(const types::Value& viewOrigin, const types::Value& viewDirection, float fovTan, float fovCos, const types::Value& circle, float circleRadius, FieldOfView fovType) {
            return calc::IsCircleInViewType(viewOrigin.Get<T>(), viewDirection.Get<T>(), fovTan, fovCos, circle.Get<T>(), circleRadius, fovType);
        }
        float InterceptTime(const types::Value& interceptor, float interceptorSpeed, const types::Value& targetPosition, const types::Value& targetVelocity) {
            return calc::InterceptTime(interceptor.Get<T>(), interceptorSpeed, targetPosition.Get<T>(), targetVelocity.Get<T>());
        }
        types::Value CubicCurve(float delta, const types::Value& p0, const types::Value& p1, const types::Value& p2, const types::Value& p3, float matrix[4][4], bool inverse) {
            return types::ValueOf(calc::CubicCurve(delta, p0.Get<T>(), p1.Get<T>(), p2.Get<T>(), p3.Get<T>(), matrix, inverse), m_type);
        }
        types::Value ParametricCubicCurve(float delta, const std::vector<types::Value>& points, float matrix[4][4], float weight, bool inverse, bool loop) {
            auto converted = std::vector<T>();
            for (auto& pt : points) {
                converted.push_back(std::move(pt.Get<T>()));
            }
            return types::ValueOf(calc::ParametricCubicCurve(delta, converted, matrix, weight, inverse, loop), m_type);
        }
    private:
        types::Type* m_type;
    };

    // All the created calculators.
    auto Calculators = types::TypedMap<std::unique_ptr<ValueCalculator>>();

    // Registers the type and creates the calculator for it. 
    // The type T should match what was used when the type was created.
    template<typename T>
    void Register(types::Type* type) {
        Calculators.Set(type, std::make_unique<TypedCalculator<T>>(type));
    }

    // Sets a custom calculator implementation for a type.
    void SetFor(const types::Type* type, std::unique_ptr<ValueCalculator> calc) {
        Calculators.Set(type, std::move(calc));
    }

    // Returns the calculator for the given type, or returns null if none has been defined.
    ValueCalculator* For(const types::Type* type) noexcept {
        auto up = Calculators.Get(type);
        return up ? up->get() : nullptr;
    }

    // Returns if a calculator has been specified for the given type.
    bool Supported(const types::Type* type) {
        return For(type) != nullptr;
    }

}