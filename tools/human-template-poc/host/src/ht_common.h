#pragma once

// P1-VR-ARCH-1 host-only prototype: shared helpers (double precision math,
// file IO, string parsing, SHA-256). Written for this prototype; contains no
// code from MakeHuman, MPFB2 or Blender.

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace pae::ht {

struct D3 {
    double x = 0.0, y = 0.0, z = 0.0;
    D3() = default;
    D3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}
    D3 operator+(const D3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    D3 operator-(const D3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    D3 operator*(double s) const { return {x * s, y * s, z * s}; }
    D3& operator+=(const D3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    D3 operator-() const { return {-x, -y, -z}; }
    double dot(const D3& o) const { return x * o.x + y * o.y + z * o.z; }
    D3 cross(const D3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    double lengthSq() const { return dot(*this); }
    double length() const { return std::sqrt(lengthSq()); }
    D3 normalized() const {
        const double l = length();
        return l > 1e-15 ? *this * (1.0 / l) : D3{};
    }
    bool operator==(const D3& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct D2 {
    double u = 0.0, v = 0.0;
};

// Rigid affine transform (rotation + translation), column-vector convention.
struct Affine {
    double m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    D3 t;

    static Affine identity() { return {}; }
    static Affine translation(const D3& p) {
        Affine a;
        a.t = p;
        return a;
    }
    static Affine rotation(const D3& axis, double angleRad) {
        const D3 n = axis.normalized();
        const double c = std::cos(angleRad), s = std::sin(angleRad), k = 1.0 - c;
        Affine a;
        a.m[0][0] = c + n.x * n.x * k;   a.m[0][1] = n.x * n.y * k - n.z * s; a.m[0][2] = n.x * n.z * k + n.y * s;
        a.m[1][0] = n.y * n.x * k + n.z * s; a.m[1][1] = c + n.y * n.y * k;   a.m[1][2] = n.y * n.z * k - n.x * s;
        a.m[2][0] = n.z * n.x * k - n.y * s; a.m[2][1] = n.z * n.y * k + n.x * s; a.m[2][2] = c + n.z * n.z * k;
        return a;
    }
    Affine operator*(const Affine& o) const {
        Affine r;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                double sum = 0.0;
                for (int k = 0; k < 3; ++k) sum += m[i][k] * o.m[k][j];
                r.m[i][j] = sum;
            }
        }
        r.t = apply(o.t);
        return r;
    }
    Affine inverse() const {
        // Rotation matrices are orthonormal: inverse = transpose.
        Affine r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) r.m[i][j] = m[j][i];
        r.t = -(r.apply(t));
        return r;
    }
    D3 apply(const D3& p) const {
        return D3{m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z,
                  m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z,
                  m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z} + t;
    }
    D3 rotateOnly(const D3& p) const {
        return D3{m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z,
                  m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z,
                  m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z};
    }
};

double distanceToSegment(const D3& p, const D3& a, const D3& b);

bool readFile(const std::string& path, std::string& out);
bool readFileBytes(const std::string& path, std::vector<uint8_t>& out);
bool writeFile(const std::string& path, const std::string& bytes);
bool fileExists(const std::string& path);
bool makeDirs(const std::string& path);  // creates all missing directories

std::vector<std::string> tokenize(const std::string& line);
bool parseLong(const std::string& s, long& out);
bool parseDouble(const std::string& s, double& out);

std::string sha256Hex(const std::vector<uint8_t>& bytes);
std::string sha256HexFile(const std::string& path);
std::string toLower(std::string s);
std::string fileNameOf(const std::string& path);
std::string fmt(const char* format, ...);

// Wall-clock helper for host measurements (milliseconds).
double nowMs();
std::string utcNow();  // "YYYY-MM-DDTHH:MM:SSZ"

} // namespace pae::ht
