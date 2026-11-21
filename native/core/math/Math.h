#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace pae {

struct Mat3;
struct Mat4;

struct Vec2 {
    float x = 0.0f, y = 0.0f;
    constexpr Vec2() = default;
    constexpr Vec2(float xx, float yy) : x(xx), y(yy) {}
};

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    constexpr Vec3() = default;
    constexpr Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    constexpr float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    constexpr Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(dot(*this)); }
    float lengthSq() const { return dot(*this); }
    Vec3 normalized(float fallback = 0.0f) const {
        const float len = length();
        if (!(len > 1e-8f) || !std::isfinite(len)) return {fallback, fallback, fallback};
        return *this * (1.0f / len);
    }
    Vec3 normalized(const Vec3& fallback) const {
        const float len = length();
        if (!(len > 1e-8f) || !std::isfinite(len)) return fallback;
        return *this * (1.0f / len);
    }
    bool finite() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

struct Vec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    constexpr Vec4() = default;
    constexpr Vec4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}
    constexpr Vec4(const Vec3& v, float ww) : x(v.x), y(v.y), z(v.z), w(ww) {}
};

struct U16Vec4 {
    uint16_t x = 0, y = 0, z = 0, w = 0;
    constexpr U16Vec4() = default;
    constexpr U16Vec4(uint16_t xx, uint16_t yy, uint16_t zz, uint16_t ww)
        : x(xx), y(yy), z(zz), w(ww) {}
};

struct Quat {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    constexpr Quat() = default;
    constexpr Quat(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}

    float lengthSq() const { return x * x + y * y + z * z + w * w; }
    Quat normalized() const {
        const float l2 = lengthSq();
        if (!(l2 > 1e-12f) || !std::isfinite(l2)) return {};
        const float inv = 1.0f / std::sqrt(l2);
        return {x * inv, y * inv, z * inv, w * inv};
    }
    Quat conjugate() const { return {-x, -y, -z, w}; }
    Quat inverse() const { return conjugate().normalized(); }

    Quat operator*(const Quat& o) const {
        return {
            w * o.x + x * o.w + y * o.z - z * o.y,
            w * o.y - x * o.z + y * o.w + z * o.x,
            w * o.z + x * o.y - y * o.x + z * o.w,
            w * o.w - x * o.x - y * o.y - z * o.z
        };
    }
    Vec3 rotate(const Vec3& v) const;

    static Quat fromToRotation(const Vec3& from, const Vec3& to) {
        const Vec3 a = from.normalized();
        const Vec3 b = to.normalized();
        const float d = a.dot(b);
        if (d < -0.999999f) {
            Vec3 axis = a.cross({1.0f, 0.0f, 0.0f});
            if (axis.lengthSq() < 1e-8f) axis = a.cross({0.0f, 1.0f, 0.0f});
            return Quat{axis.x, axis.y, axis.z, 0.0f}.normalized();
        }
        const Vec3 c = a.cross(b);
        const float s = std::sqrt((1.0f + d) * 2.0f);
        if (!(s > 1e-8f)) return {};
        const float inv = 1.0f / s;
        return Quat{c.x * inv, c.y * inv, c.z * inv, 0.5f * s}.normalized();
    }

    static Quat slerp(const Quat& aIn, const Quat& bIn, float t) {
        Quat a = aIn.normalized(), b = bIn.normalized();
        float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        if (d < 0.0f) { b = {-b.x, -b.y, -b.z, -b.w}; d = -d; }
        if (d > 0.9995f) {
            return Quat{
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            }.normalized();
        }
        const float theta = std::acos(d);
        const float sinTheta = std::sin(theta);
        if (!(sinTheta > 1e-8f)) return a;
        const float wa = std::sin((1.0f - t) * theta) / sinTheta;
        const float wb = std::sin(t * theta) / sinTheta;
        return {
            a.x * wa + b.x * wb,
            a.y * wa + b.y * wb,
            a.z * wa + b.z * wb,
            a.w * wa + b.w * wb
        };
    }

    Mat4 toMat4() const;
};

struct Mat4 {
    std::array<float, 16> m{};

    constexpr Mat4() {
        m.fill(0.0f);
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    static Mat4 fromColumns(const std::array<float, 16>& values) {
        Mat4 r; r.m = values; return r;
    }
    float at(int col, int row) const { return m[static_cast<size_t>(col) * 4 + row]; }
    void set(int col, int row, float v) { m[static_cast<size_t>(col) * 4 + row] = v; }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) sum += at(k, row) * o.at(c, k);
                r.set(c, row, sum);
            }
        }
        return r;
    }

    Vec4 transform(const Vec4& v) const {
        return {
            at(0, 0) * v.x + at(1, 0) * v.y + at(2, 0) * v.z + at(3, 0) * v.w,
            at(0, 1) * v.x + at(1, 1) * v.y + at(2, 1) * v.z + at(3, 1) * v.w,
            at(0, 2) * v.x + at(1, 2) * v.y + at(2, 2) * v.z + at(3, 2) * v.w,
            at(0, 3) * v.x + at(1, 3) * v.y + at(2, 3) * v.z + at(3, 3) * v.w
        };
    }
    Vec3 rotateVector(const Vec3& v) const {
        const Vec4 r = transform(Vec4{v, 0.0f});
        return {r.x, r.y, r.z};
    }

    Mat3 upperLeft3x3() const;

    Mat4 transpose() const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row) r.set(row, c, at(c, row));
        return r;
    }

    bool finite() const {
        for (float v : m) if (!std::isfinite(v)) return false;
        return true;
    }

    static Mat4 translation(const Vec3& t) {
        Mat4 r; r.set(3, 0, t.x); r.set(3, 1, t.y); r.set(3, 2, t.z); return r;
    }
    static Mat4 scaling(const Vec3& s) {
        Mat4 r; r.set(0, 0, s.x); r.set(1, 1, s.y); r.set(2, 2, s.z); return r;
    }
    static Mat4 trs(const Vec3& t, const Quat& q, const Vec3& s) {
        Mat4 r = q.toMat4();
        r.set(0, 0, r.at(0, 0) * s.x); r.set(0, 1, r.at(0, 1) * s.x); r.set(0, 2, r.at(0, 2) * s.x);
        r.set(1, 0, r.at(1, 0) * s.y); r.set(1, 1, r.at(1, 1) * s.y); r.set(1, 2, r.at(1, 2) * s.y);
        r.set(2, 0, r.at(2, 0) * s.z); r.set(2, 1, r.at(2, 1) * s.z); r.set(2, 2, r.at(2, 2) * s.z);
        r.set(3, 0, t.x); r.set(3, 1, t.y); r.set(3, 2, t.z); r.set(3, 3, 1.0f);
        return r;
    }
    static Mat4 perspective(float fovRadians, float aspect, float near, float far) {
        const float f = 1.0f / std::tan(fovRadians * 0.5f);
        Mat4 r;
        r.set(0, 0, f / aspect);
        r.set(1, 1, f);
        r.set(2, 2, (far + near) / (near - far));
        r.set(2, 3, -1.0f);
        r.set(3, 2, (2.0f * far * near) / (near - far));
        r.set(3, 3, 0.0f);
        return r;
    }
    static Mat4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        Mat4 r;
        r.set(0, 0, 2.0f / (right - left));
        r.set(1, 1, 2.0f / (top - bottom));
        r.set(2, 2, -2.0f / (far - near));
        r.set(3, 0, -(right + left) / (right - left));
        r.set(3, 1, -(top + bottom) / (top - bottom));
        r.set(3, 2, -(far + near) / (far - near));
        return r;
    }
    // Right-handed look-at; camera looks down -Z.
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        const Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        if (s.lengthSq() < 1e-8f) s = {1.0f, 0.0f, 0.0f};
        const Vec3 u = s.cross(f);
        Mat4 r;
        r.set(0, 0, s.x); r.set(1, 0, s.y); r.set(2, 0, s.z);
        r.set(0, 1, u.x); r.set(1, 1, u.y); r.set(2, 1, u.z);
        r.set(0, 2, -f.x); r.set(1, 2, -f.y); r.set(2, 2, -f.z);
        r.set(3, 0, -s.dot(eye)); r.set(3, 1, -u.dot(eye)); r.set(3, 2, f.dot(eye));
        return r;
    }

    // General inverse via cofactors; returns false for singular matrices.
    bool inverse(Mat4& out) const {
        const float a00 = at(0, 0), a01 = at(0, 1), a02 = at(0, 2), a03 = at(0, 3);
        const float a10 = at(1, 0), a11 = at(1, 1), a12 = at(1, 2), a13 = at(1, 3);
        const float a20 = at(2, 0), a21 = at(2, 1), a22 = at(2, 2), a23 = at(2, 3);
        const float a30 = at(3, 0), a31 = at(3, 1), a32 = at(3, 2), a33 = at(3, 3);

        const float b00 = a00 * a11 - a01 * a10, b01 = a00 * a12 - a02 * a10;
        const float b02 = a00 * a13 - a03 * a10, b03 = a01 * a12 - a02 * a11;
        const float b04 = a01 * a13 - a03 * a11, b05 = a02 * a13 - a03 * a12;
        const float b06 = a20 * a31 - a21 * a30, b07 = a20 * a32 - a22 * a30;
        const float b08 = a20 * a33 - a23 * a30, b09 = a21 * a32 - a22 * a31;
        const float b10 = a21 * a33 - a23 * a31, b11 = a22 * a33 - a23 * a32;

        float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
        if (!std::isfinite(det) || std::fabs(det) < 1e-10f) return false;
        det = 1.0f / det;

        Mat4 r;
        r.set(0, 0, (a11 * b11 - a12 * b10 + a13 * b09) * det);
        r.set(0, 1, (a02 * b10 - a01 * b11 - a03 * b09) * det);
        r.set(0, 2, (a31 * b05 - a32 * b04 + a33 * b03) * det);
        r.set(0, 3, (a22 * b04 - a21 * b05 - a23 * b03) * det);
        r.set(1, 0, (a12 * b08 - a10 * b11 - a13 * b07) * det);
        r.set(1, 1, (a00 * b11 - a02 * b08 + a03 * b07) * det);
        r.set(1, 2, (a32 * b02 - a30 * b05 - a33 * b01) * det);
        r.set(1, 3, (a20 * b05 - a22 * b02 + a23 * b01) * det);
        r.set(2, 0, (a10 * b10 - a11 * b08 + a13 * b06) * det);
        r.set(2, 1, (a01 * b08 - a00 * b10 - a03 * b06) * det);
        r.set(2, 2, (a30 * b04 - a31 * b02 + a33 * b00) * det);
        r.set(2, 3, (a21 * b02 - a20 * b04 - a23 * b00) * det);
        r.set(3, 0, (a11 * b07 - a10 * b09 - a12 * b06) * det);
        r.set(3, 1, (a00 * b09 - a01 * b07 + a02 * b06) * det);
        r.set(3, 2, (a31 * b01 - a30 * b03 - a32 * b00) * det);
        r.set(3, 3, (a20 * b03 - a21 * b01 + a22 * b00) * det);
        out = r;
        return true;
    }
};

struct Mat3 {
    std::array<float, 9> m{};
    constexpr Mat3() {
        m.fill(0.0f);
        m[0] = m[4] = m[8] = 1.0f;
    }
    constexpr Mat3(const std::array<float, 9>& values) : m(values) {}
    float at(int col, int row) const { return m[static_cast<size_t>(col) * 3 + row]; }
    Mat3 transpose() const {
        Mat3 r;
        for (int c = 0; c < 3; ++c)
            for (int row = 0; row < 3; ++row) r.m[static_cast<size_t>(row) * 3 + c] = at(c, row);
        return r;
    }
};

inline Mat3 Mat4::upperLeft3x3() const {
    return Mat3{{
        at(0, 0), at(1, 0), at(2, 0),
        at(0, 1), at(1, 1), at(2, 1),
        at(0, 2), at(1, 2), at(2, 2)
    }};
}

inline Vec3 Quat::rotate(const Vec3& v) const { return toMat4().rotateVector(v); }

inline Mat4 Quat::toMat4() const {
    const Quat q = normalized();
    Mat4 r;
    const float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    const float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
    const float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
    const float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    r.set(0, 0, 1.0f - (yy + zz)); r.set(0, 1, xy + wz);       r.set(0, 2, xz - wy);
    r.set(1, 0, xy - wz);       r.set(1, 1, 1.0f - (xx + zz)); r.set(1, 2, yz + wx);
    r.set(2, 0, xz + wy);       r.set(2, 1, yz - wx);           r.set(2, 2, 1.0f - (xx + yy));
    r.set(3, 3, 1.0f);
    return r;
}

struct Aabb {
    Vec3 min{0.0f, 0.0f, 0.0f};
    Vec3 max{0.0f, 0.0f, 0.0f};
    bool valid = false;
    void expand(const Vec3& p) {
        if (!valid) { min = max = p; valid = true; return; }
        min.x = std::fmin(min.x, p.x); min.y = std::fmin(min.y, p.y); min.z = std::fmin(min.z, p.z);
        max.x = std::fmax(max.x, p.x); max.y = std::fmax(max.y, p.y); max.z = std::fmax(max.z, p.z);
    }
    Vec3 size() const { return valid ? max - min : Vec3{}; }
};

inline float clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace pae
