#include "procedural/shape/v2/SharedSkinEnvelope.h"

#include <algorithm>
#include <cmath>

namespace pae {
namespace shape {
namespace v2 {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Interpolating natural cubic spline — the ACTUAL height interpolant.
//
// This is C2 everywhere AND passes exactly through its input knots. The three
// REGION splines are built from anatomical anchors, so those measured values
// are not approximated at that stage. The final width/depth profile is then
// resampled onto 26 uniform control points and refit with the same interpolant
// in buildProfileData: the delivered profile is a C2 reconstruction of the
// combined region splines, not a literal interpolation of the anchor list.
// anchorHalfWidths/Depths are back-computed from the delivered profile for
// reporting, so they describe the mesh rather than the input knots.
//
// An earlier design described this code as a "clamped uniform cubic B-spline".
// That description is STALE and was removed: a cubic B-spline passes only
// through its end control points, so interior anchors drifted by millimetres
// and the continuity test caught the resulting slope break. The implementation
// below solves for second derivatives with a natural boundary condition
// (second derivative = 0 at both ends). It needs two or more points and works
// with unevenly spaced heights; `valid()` reports otherwise.
// ---------------------------------------------------------------------------
class CubicInterpolant {
public:
    CubicInterpolant() = default;

    CubicInterpolant(const std::vector<float>& xs, const std::vector<float>& ys) {
        build(xs, ys);
    }

    bool valid() const { return xs_.size() >= 2 && xs_.size() == ys_.size(); }

    void build(const std::vector<float>& xs, const std::vector<float>& ys) {
        const size_t n = xs.size();
        if (n < 2 || n != ys.size()) return;
        for (size_t i = 0; i + 1 < n; ++i) {
            if (!(xs[i + 1] > xs[i])) return;   // must be strictly increasing
        }
        xs_ = xs;
        ys_ = ys;
        const size_t m = n - 1;   // intervals

        // Second derivatives at the knots (natural: c[0] = c[n-1] = 0).
        std::vector<double> c(n, 0.0);
        if (n > 2) {
            const size_t k = n - 2;   // interior knots
            std::vector<double> lower(k, 0.0), diag(k, 0.0), upper(k, 0.0), rhs(k, 0.0);
            for (size_t i = 1; i <= k; ++i) {
                const double hPrev = xs[i] - xs[i - 1];
                const double hNext = xs[i + 1] - xs[i];
                diag[i - 1] = 2.0 * (hPrev + hNext);
                rhs[i - 1] = 6.0 * ((ys[i + 1] - ys[i]) / hNext - (ys[i] - ys[i - 1]) / hPrev);
                if (i > 1) lower[i - 1] = hPrev;
                if (i < k) upper[i - 1] = hNext;
            }
            // Thomas algorithm.
            for (size_t i = 1; i < k; ++i) {
                const double w = lower[i] / diag[i - 1];
                diag[i] -= w * upper[i - 1];
                rhs[i] -= w * rhs[i - 1];
            }
            c[k - 1] = rhs[k - 1] / diag[k - 1];
            for (int i = static_cast<int>(k) - 2; i >= 0; --i) {
                c[i] = (rhs[i] - upper[i] * c[i + 1]) / diag[i];
            }
            // `c[j]` now holds the second derivative at interior knot j+1;
            // scatter it into the full knot array (natural ends stay 0).
            std::vector<double> full(n, 0.0);
            for (size_t j = 0; j < k; ++j) full[j + 1] = c[j];
            c = full;
        }

        c_ = c;
    }

    float at(float x) const {
        if (!valid()) return 0.0f;
        const size_t n = xs_.size();
        if (x <= xs_.front()) return ys_.front();
        if (x >= xs_.back()) return ys_.back();
        size_t i = 0;
        while (i + 2 < n && x > xs_[i + 1]) ++i;
        const double h = xs_[i + 1] - xs_[i];
        const double a = (xs_[i + 1] - x) / h;
        const double b = (x - xs_[i]) / h;
        // Standard natural-cubic-spline evaluation form.
        const double v = a * ys_[i] + b * ys_[i + 1] +
                         ((a * a * a - a) * c_[i] + (b * b * b - b) * c_[i + 1]) * h * h / 6.0;
        return static_cast<float>(v);
    }

private:
    std::vector<float> xs_;
    std::vector<float> ys_;
    std::vector<double> c_;
};

// A region contribution: anchors at anatomical heights, interpolated exactly.
class HeightSpline {
public:
    HeightSpline() = default;

    HeightSpline(const std::vector<float>& heights, const std::vector<float>& values) {
        spline_ = CubicInterpolant(heights, values);
    }

    bool valid() const { return spline_.valid(); }

    float at(float y) const { return spline_.at(y); }

private:
    CubicInterpolant spline_;
};

// C2 smoothstep (quintic). Used for every region weight so the weighted sum
// of C2 splines stays C2.
float smoothWeight(float x) {
    const float t = std::clamp(x, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// A region weight that ramps up over [a, b] and back down over [c, d].
// Outside [a, d] it is 0; between [b, c] it is 1.
float bandWeight(float y, float a, float b, float c, float d) {
    if (y <= a || y >= d) return 0.0f;
    if (y < b) return smoothWeight((y - a) / std::max(1e-6f, b - a));
    if (y <= c) return 1.0f;
    return smoothWeight(1.0f - (y - c) / std::max(1e-6f, d - c));
}


// ---------------------------------------------------------------------------
// The profile data shared by the mesh builder and the profile query.
//
// There is exactly ONE builder, so the mesh and any query about the mesh
// always agree. An earlier revision kept two copies of this logic and they
// drifted apart; that is what the "profile_matches_the_measured_anchors"
// test now guards against.
// ---------------------------------------------------------------------------
struct ProfileData {
    HeightSpline width;
    HeightSpline depth;

    float yBottom = 0.0f;
    float yTop = 0.0f;        // the shoulder-root crop height
    float yGirdleTop = 0.0f;  // the top of the shoulder girdle
    float girdleHeight = 0.0f;

    float exponent = 2.0f;         // superellipse exponent
    float axillaDepth = 0.0f;      // metres of inward carve
    float axillaCenterY = 0.0f;
    float axillaHalfSpanY = 0.0f;
    float axillaHalfSpanU = 0.10f;

    // The anchors the profile was built from, for the report.
    std::vector<float> anchorHeights;
    std::vector<float> anchorHalfWidths;
    std::vector<float> anchorHalfDepths;
};

ProfileData buildProfileData(const CharacterDefinition& definition,
                              const BodyJoints& joints,
                              const SharedSkinEnvelopeSpec& spec) {
    ProfileData data;

    const float ribCageVolume = definition.parameters.value("thorax-ribcage-volume", 1.0f);
    const float thoraxDepthRatio = definition.parameters.value("thorax-depth-ratio", 1.0f);
    const float abdomenVolume = definition.parameters.value("abdomen-volume", 1.0f);
    const float waistPinch = definition.parameters.value("waist-pinch", 0.5f);
    const float pelvisVolume = definition.parameters.value("pelvis-volume", 1.0f);
    const float girdleMass = definition.parameters.value("shoulder-girdle-mass", 1.0f);
    const float axillaDepth = definition.parameters.value("axilla-depth", 0.028f);
    const float stylization = definition.parameters.value("stylization", 0.25f);

    data.girdleHeight = definition.parameters.value("shoulder-girdle-height", 0.075f);
    data.yBottom = joints.hips.y + spec.yBottomOffset;
    data.yTop = joints.chest.y + spec.yTopOffset;
    data.yGirdleTop = data.yTop + data.girdleHeight;

    // ---- Anatomical heights ----------------------------------------------
    const float yIliac = joints.hips.y + 0.04f;     // iliac crest
    const float yWaist = joints.hips.y + 0.12f;    // waist
    const float yLowerRib = joints.spine.y;         // lower rib / costal margin
    const float ySternum = joints.chest.y - 0.08f; // sternal angle
    const float yChest = joints.chest.y;
    // The thorax -> girdle hand-over finishes this far above the sternum.
    const float girdleRampEnd = ySternum + 0.05f;

    // ---- Measured / derived section half axes -----------------------------
    const ProportionProfile& p = definition.proportions;
    float waistHalfWidth = 0.0f;
    float waistHalfDepth = 0.0f;
    p.waistHalfAxes(&waistHalfWidth, &waistHalfDepth);

    const float hipHalfWidth = 0.5f * p.hipBreadthM();
    const float chestHalfWidth = p.chestHalfWidthM();
    const float chestHalfDepth = p.chestHalfDepthM();

    // ---- Region contributions ---------------------------------------------
    //
    // Each region is a C2 spline through anatomically anchored control
    // points. They are combined LATER, in control-point space.

    // Thorax guide: the rib cage from the costal margin up to the chest.
    const HeightSpline thorax(
        {yLowerRib, ySternum, yChest, yChest + 0.50f * data.girdleHeight},
        {chestHalfWidth * 0.92f * ribCageVolume,
         chestHalfWidth * ribCageVolume,
         chestHalfWidth * ribCageVolume,
         chestHalfWidth * 1.02f * ribCageVolume});

    const HeightSpline thoraxDepth(
        {yLowerRib, ySternum, yChest, yChest + 0.50f * data.girdleHeight},
        {chestHalfDepth * 0.92f * ribCageVolume * thoraxDepthRatio,
         chestHalfDepth * ribCageVolume * thoraxDepthRatio,
         chestHalfDepth * 0.97f * ribCageVolume * thoraxDepthRatio,
         chestHalfDepth * 0.99f * ribCageVolume * thoraxDepthRatio});

    // Abdomen / pelvis: from the hip breadth up through the waist to the
    // costal margin. The waist pinch pulls the section in.
    //
    // The LAST anchor equals the thorax's FIRST anchor, because the two
    // splines hand over at the costal margin: if they disagreed there, the
    // blend would carry a step instead of a slope.
    const float waistWidth = waistHalfWidth * (1.0f - 0.10f * waistPinch) * abdomenVolume;
    const float waistDepthValue = waistHalfDepth * (1.0f - 0.06f * waistPinch) * abdomenVolume;

    const HeightSpline abdomen(
        {data.yBottom, yIliac, yWaist, yLowerRib},
        {hipHalfWidth * 0.97f * pelvisVolume,
         hipHalfWidth * pelvisVolume,
         waistWidth,
         chestHalfWidth * 0.92f * abdomenVolume});

    const HeightSpline abdomenDepth(
        {data.yBottom, yIliac, yWaist, yLowerRib},
        {hipHalfWidth * 0.62f * pelvisVolume,
         hipHalfWidth * 0.64f * pelvisVolume,
         waistDepthValue,
         chestHalfDepth * 0.92f * abdomenVolume * thoraxDepthRatio});

    // Shoulder girdle: rises from the STERNUM to the crop ring, widening for
    // the deltoid / clavicle mass.
    //
    // The FIRST anchor equals the thorax's value at the sternum, for the
    // same reason: the hand-over must agree in value, and the weights then
    // provide the smooth transition. The measured biacromial breadth is
    // reached at the crop ring, but the approach starts at the sternum so it
    // does not read as a trumpet.
    const HeightSpline girdle(
        {ySternum, yChest, yChest + 0.45f * data.girdleHeight, data.yGirdleTop},
        {chestHalfWidth * ribCageVolume,
         chestHalfWidth * 1.08f * girdleMass,
         chestHalfWidth * 1.26f * girdleMass,
         chestHalfWidth * 1.42f * girdleMass});

    const HeightSpline girdleDepth(
        {ySternum, yChest, yChest + 0.45f * data.girdleHeight, data.yGirdleTop},
        {chestHalfDepth * ribCageVolume * thoraxDepthRatio,
         chestHalfDepth * 0.99f * girdleMass,
         chestHalfDepth * 1.01f * girdleMass,
         chestHalfDepth * 1.04f * girdleMass});

    // ---- Combine in control-point space -----------------------------------
    //
    // The global control grid is UNIFORM in height, so the combined profile
    // is a single uniform C2 spline. Each global control point takes the C2
    // weighted sum of the three regional splines; the weights sum to 1, so
    // the combined profile is itself C2.
    const int controlCount = 26;
    std::vector<float> controlHeights;
    std::vector<float> controlWidths;
    std::vector<float> controlDepths;
    controlHeights.reserve(static_cast<size_t>(controlCount));
    controlWidths.reserve(static_cast<size_t>(controlCount));
    controlDepths.reserve(static_cast<size_t>(controlCount));

    for (int i = 0; i < controlCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(controlCount - 1);
        const float y = data.yBottom + t * (data.yGirdleTop - data.yBottom);

        // The three region weights are an EXACT partition of unity, built
        // from two C2 smoothsteps:
        //
        //   S1 over [costal margin, sternum]      moves abdomen -> thorax
        //   S2 over [sternum, sternum + 5 cm]     moves thorax -> girdle
        //
        //   wAbdomen = 1 - S1
        //   wThorax  = S1 * (1 - S2)
        //   wGirdle  = S1 * S2
        //
        // The sum is identically 1 and every weight is C2, so the weighted
        // blend of three C2 splines is itself C2 with no kink at the
        // hand-over heights. An earlier revision used three independent
        // bandWeight ramps that did not complement; the continuity test
        // caught the kink that produced.
        const float s1 = smoothWeight((y - yLowerRib) /
                                      std::max(1e-6f, ySternum - yLowerRib));
        const float s2 = smoothWeight((y - ySternum) /
                                      std::max(1e-6f, girdleRampEnd - ySternum));
        const float wAbdomen = 1.0f - s1;
        const float wThorax = s1 * (1.0f - s2);
        const float wGirdle = s1 * s2;

        const float halfWidth = (wAbdomen * abdomen.at(y) +
                                 wThorax * thorax.at(y) +
                                 wGirdle * girdle.at(y));
        const float halfDepth = (wAbdomen * abdomenDepth.at(y) +
                                 wThorax * thoraxDepth.at(y) +
                                 wGirdle * girdleDepth.at(y));

        controlHeights.push_back(y);
        controlWidths.push_back(std::max(1e-4f, halfWidth));
        controlDepths.push_back(std::max(1e-4f, halfDepth));
    }

    data.width = HeightSpline(controlHeights, controlWidths);
    data.depth = HeightSpline(controlHeights, controlDepths);

    data.exponent = 2.0f + 2.0f * stylization;
    data.axillaDepth = axillaDepth;
    data.axillaCenterY = data.yTop - 0.35f * data.girdleHeight;
    data.axillaHalfSpanY = std::max(0.012f, 0.55f * data.girdleHeight);
    data.axillaHalfSpanU = 0.13f;

    data.anchorHeights = {data.yBottom, yIliac, yWaist, yLowerRib, ySternum, yChest,
                          data.yGirdleTop};
    data.anchorHalfWidths.clear();
    data.anchorHalfDepths.clear();
    for (float y : data.anchorHeights) {
        data.anchorHalfWidths.push_back(data.width.at(y));
        data.anchorHalfDepths.push_back(data.depth.at(y));
    }
    return data;
}

// The axilla concave constraint at a height and an azimuth.
//
// Both falloffs are QUARTIC, not quadratic: a quadratic bump is only C1 and
// reads as a sharp notch in the clay render; the quartic is C3 at the edge
// and the notch reads as a smooth hollow. This was not a guess - the first
// render showed the difference and the falloff was changed because of it.
float axillaConcavityAt(const ProfileData& data, float y, float u) {
    const float dy = std::fabs(y - data.axillaCenterY) / data.axillaHalfSpanY;
    if (dy >= 1.0f) return 0.0f;
    const float h = 1.0f - dy * dy;
    const float h4 = h * h * h * h;
    // The armpit is on both sides of the body: u = 0 (+X) and u = 0.5 (-X).
    const float du = std::min(std::fabs(u - 0.0f), std::fabs(u - 0.5f));
    if (du >= data.axillaHalfSpanU) return 0.0f;
    const float q = 1.0f - du / data.axillaHalfSpanU;
    const float q4 = q * q * q * q;
    return data.axillaDepth * h4 * q4;
}

// Area-weighted vertex normals from the mesh's own triangles.
//
// The v2 module deliberately does NOT call shape::computeVertexNormals: the
// G2 shape module is frozen evidence (AUTH-08 FB-11) and the v2 slice is a
// separate construction, so it carries its own normal pass rather than
// linking against the legacy implementation.
void computeEnvelopeVertexNormals(MeshData& mesh) {
    for (Vertex& v : mesh.vertices) v.normal = Vec3{0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vec3& a = mesh.vertices[mesh.indices[i]].position;
        const Vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const Vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
        const Vec3 n = (b - a).cross(c - a);
        for (int k = 0; k < 3; ++k) {
            Vertex& v = mesh.vertices[mesh.indices[i + static_cast<size_t>(k)]];
            v.normal = v.normal + n;
        }
    }
    for (Vertex& v : mesh.vertices) {
        const float len = v.normal.length();
        v.normal = len > 1e-8f ? v.normal * (1.0f / len) : Vec3{0.0f, 1.0f, 0.0f};
    }
}

} // namespace

// ---------------------------------------------------------------------------
// The envelope.
// ---------------------------------------------------------------------------

SharedSkinEnvelopeResult buildSharedSkinEnvelope(const CharacterDefinition& definition,
                                                 const SharedSkinEnvelopeSpec& spec,
                                                 const BodyJoints& joints) {
    SharedSkinEnvelopeResult out;

    const int rings = std::max(4, spec.rings);
    const int segments = std::max(6, spec.segments);

    const ProfileData data = buildProfileData(definition, joints, spec);

    if (!(data.yGirdleTop > data.yBottom)) {
        out.error = "degenerate height range";
        return out;
    }
    if (!data.width.valid() || !data.depth.valid()) {
        out.error = "the section profile spline is not valid";
        return out;
    }

    // ---- Build the mesh ----------------------------------------------------
    out.mesh.vertices.reserve(static_cast<size_t>(rings) * static_cast<size_t>(segments) + 1);

    for (int v = 0; v < rings; ++v) {
        const float tv = static_cast<float>(v) / static_cast<float>(rings - 1);
        const float y = data.yBottom + tv * (data.yGirdleTop - data.yBottom);

        const float halfWidth = data.width.at(y);
        const float halfDepth = data.depth.at(y);

        for (int s = 0; s < segments; ++s) {
            const float u = static_cast<float>(s) / static_cast<float>(segments);

            // Superellipse radius at azimuth u.
            const float theta = u * 2.0f * kPi;
            const float ct = std::cos(theta);
            const float st = std::sin(theta);
            const double n = static_cast<double>(data.exponent);
            const double denom =
                std::pow(std::pow(std::fabs(static_cast<double>(ct) /
                                             static_cast<double>(halfWidth)), n) +
                             std::pow(std::fabs(static_cast<double>(st) /
                                               static_cast<double>(halfDepth)), n),
                         1.0 / n);
            float radius = denom > 1e-9 ? static_cast<float>(1.0 / denom) : halfWidth;

            // Axilla concave constraint.
            const float concavity = axillaConcavityAt(data, y, u);
            // Never invert the section.
            radius = std::max(radius * 0.25f, radius - concavity);

            Vertex vert;
            vert.position = Vec3{radius * ct, y, radius * st};
            vert.uv = Vec2{u, tv};
            vert.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
            vert.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
            out.mesh.vertices.push_back(vert);
        }
    }

    // Quads: quad(v, s) = { v*S+s, v*S+sn, (v+1)*S+sn, (v+1)*S+s }
    //
    // The TRIANGLE order is (a, c, b) and (a, d, c), matching the template
    // convention in shape/QuadTemplate.cpp. This matters: the reverse order
    // (a, b, c) produces INWARD normals, which would invert every face
    // normal and silently corrupt the seam measurement.
    out.quads.reserve(static_cast<size_t>(rings - 1) * static_cast<size_t>(segments));
    for (int v = 0; v + 1 < rings; ++v) {
        for (int s = 0; s < segments; ++s) {
            const uint32_t sn = static_cast<uint32_t>((s + 1) % segments);
            const uint32_t a = static_cast<uint32_t>(v * segments + s);
            const uint32_t b = static_cast<uint32_t>(v * segments + static_cast<int>(sn));
            const uint32_t c = static_cast<uint32_t>((v + 1) * segments + static_cast<int>(sn));
            const uint32_t d = static_cast<uint32_t>((v + 1) * segments + s);
            out.quads.push_back({a, b, c, d});
            out.mesh.indices.push_back(a);
            out.mesh.indices.push_back(c);
            out.mesh.indices.push_back(b);
            out.mesh.indices.push_back(a);
            out.mesh.indices.push_back(d);
            out.mesh.indices.push_back(c);
        }
    }

    // Bottom cap: a pelvis pole so the torso stays a closed surface and the
    // volume measurement remains meaningful.
    //
    // The triangle order is (center, v0, v1) so the cap normal points DOWN
    // (-Y), which is the outward direction of a bottom cap. The G2 template
    // used (center, v1, v0), whose normal points INTO the body - that is the
    // flip recorded as OQ-16. The v2 code gets it right rather than copying
    // the defect, and the seam tool measures the result either way.
    if (spec.capBottom) {
        const uint32_t bottomCenter = static_cast<uint32_t>(out.mesh.vertices.size());
        Vertex v;
        v.position = Vec3{0.0f, data.yBottom, 0.0f};
        v.uv = Vec2{0.5f, 0.0f};
        v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
        v.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
        out.mesh.vertices.push_back(v);
        for (int j = 0; j < segments; ++j) {
            const uint32_t v0 = static_cast<uint32_t>(j);
            const uint32_t v1 = static_cast<uint32_t>((j + 1) % segments);
            out.mesh.indices.push_back(bottomCenter);
            out.mesh.indices.push_back(v0);
            out.mesh.indices.push_back(v1);
        }
    }

    // Top: the shoulder-root crop boundary, left OPEN on purpose.
    if (spec.capTop) {
        const uint32_t topCenter = static_cast<uint32_t>(out.mesh.vertices.size());
        Vertex v;
        v.position = Vec3{0.0f, data.yGirdleTop, 0.0f};
        v.uv = Vec2{0.5f, 1.0f};
        v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
        v.color = Vec4{0.78f, 0.78f, 0.80f, 1.0f};
        out.mesh.vertices.push_back(v);
        const uint32_t ringOffset = static_cast<uint32_t>((rings - 1) * segments);
        for (int j = 0; j < segments; ++j) {
            const uint32_t v0 = ringOffset + static_cast<uint32_t>(j);
            const uint32_t v1 = ringOffset + static_cast<uint32_t>((j + 1) % segments);
            out.mesh.indices.push_back(topCenter);
            out.mesh.indices.push_back(v1);
            out.mesh.indices.push_back(v0);
        }
    }

    // Register the crop boundary on the TOP ring, matching the QuadTemplate
    // convention so the topology audit can tell it from a crack.
    {
        CropBoundary crop;
        crop.label = spec.cropLabel;
        crop.ringIndex = rings - 1;
        crop.vertexCount = segments;
        crop.edgeCount = segments;
        crop.loopCount = 1;
        crop.closed = true;
        float sumY = 0.0f;
        const uint32_t ringOffset = static_cast<uint32_t>((rings - 1) * segments);
        for (int j = 0; j < segments; ++j) {
            sumY += out.mesh.vertices[ringOffset + static_cast<uint32_t>(j)].position.y;
        }
        crop.y = sumY / static_cast<float>(segments);
        out.cropBoundaries.push_back(crop);
    }

    // Vertex normals from the ACTUAL geometry (the seam tool measures the
    // geometry, so the normals must come from it too).
    computeEnvelopeVertexNormals(out.mesh);

    for (const Vertex& v : out.mesh.vertices) out.mesh.bounds.expand(v.position);

    out.anchorHeights = data.anchorHeights;
    out.anchorHalfWidths = data.anchorHalfWidths;
    out.anchorHalfDepths = data.anchorHalfDepths;

    out.valid = true;
    return out;
}

EnvelopeProfile evaluateProfile(const CharacterDefinition& definition,
                                 const BodyJoints& joints,
                                 const SharedSkinEnvelopeSpec& spec,
                                 float y, float u) {
    // The SAME builder the mesh uses, so a query can never disagree with
    // the geometry it describes.
    const ProfileData data = buildProfileData(definition, joints, spec);

    EnvelopeProfile profile;
    profile.yBottom = data.yBottom;
    profile.yTop = data.yTop;
    profile.centerZ = 0.0f;
    profile.halfWidth = data.width.at(y);
    profile.halfDepth = data.depth.at(y);
    profile.superEllipseExponent = data.exponent;
    profile.axillaConcavity = axillaConcavityAt(data, y, u);
    return profile;
}

} // namespace v2
} // namespace shape
} // namespace pae
