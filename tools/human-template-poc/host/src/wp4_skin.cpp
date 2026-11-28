#include "wp4_skin.h"

#include <cmath>

namespace pae::wp4 {
using ht::D3;
using ht::Affine;

std::vector<Affine> poseGlobals(const RigV2& rig, const Pose& pose) {
    const size_t n = rig.bones.size();
    std::vector<Affine> local(n);
    for (size_t b = 0; b < n; ++b) {
        Affine rl = Affine::identity();
        for (const LocalRotation& r : pose.rotations) {
            if (r.bone != rig.bones[b].name || r.angleDeg == 0.0) continue;
            // M2-POSE-FRAME-AUDIT-01 fix: rl is RIGHT-multiplied onto restLocal,
            // so it acts in the bone's LOCAL frame. Its rotation axis must be a
            // LOCAL unit axis (1,0,0)/(0,1,0)/(0,0,1), NOT rig.ax/ay/az (which are
            // the REST WORLD axes). Using world axes here made the skin rotate
            // about R*axis instead of axis (wrong for any non-axis-aligned bone).
            // With local unit axes, S = T(head)*R*Rot(e_k)*R^-1*T(-head)
            //   = T(head)*Rot(R*e_k)*T(-head) = rotation about the rest world axis.
            const D3 axis = r.axis == 0 ? D3(1, 0, 0) : (r.axis == 1 ? D3(0, 1, 0) : D3(0, 0, 1));
            rl = rl * Affine::rotation(axis, r.angleDeg * 3.14159265358979323846 / 180.0);
        }
        local[b] = rig.restLocal[b] * rl;
    }
    std::vector<Affine> globals(n);
    std::vector<int> state(n, 0);
    for (size_t b = 0; b < n; ++b) {
        std::vector<int> chain;
        int cur = static_cast<int>(b);
        while (cur >= 0 && state[static_cast<size_t>(cur)] != 2) {
            if (state[static_cast<size_t>(cur)] == 1) break;
            state[static_cast<size_t>(cur)] = 1;
            chain.push_back(cur);
            cur = rig.bones[static_cast<size_t>(cur)].parent;
        }
        Affine g = cur >= 0 ? globals[static_cast<size_t>(cur)] : Affine::identity();
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            g = g * local[static_cast<size_t>(*it)];
            globals[static_cast<size_t>(*it)] = g;
            state[static_cast<size_t>(*it)] = 2;
        }
    }
    return globals;
}

std::vector<Affine> skinMatrices(const RigV2& rig, const std::vector<Affine>& globals) {
    std::vector<Affine> skin(rig.bones.size());
    for (size_t b = 0; b < rig.bones.size(); ++b)
        skin[b] = globals[b] * rig.restGlobal[b].inverse();
    return skin;
}

std::vector<D3> skinPositions(const std::vector<D3>& rest, const ht::SkinWeights& sw,
                              const std::vector<Affine>& skin) {
    return ht::lbsSkin(rest, sw, skin);
}

std::vector<D3> skinNormals(const std::vector<D3>& restNormals, const ht::SkinWeights& sw,
                            const std::vector<Affine>& skin) {
    std::vector<D3> out(restNormals.size());
    for (size_t v = 0; v < restNormals.size() && v < static_cast<size_t>(sw.vertexCount); ++v) {
        D3 sum;
        const uint8_t n = sw.count[v];
        if (n == 0) { out[v] = restNormals[v]; continue; }
        for (int k = 0; k < n; ++k) {
            const double w = sw.w[v][static_cast<size_t>(k)];
            if (w == 0.0) continue;
            sum += skin[sw.bone[v][static_cast<size_t>(k)]].rotateOnly(restNormals[v]) * w;
        }
        out[v] = sum.normalized();
    }
    return out;
}

double bindRecoveryError(const std::vector<D3>& rest, const ht::SkinWeights& sw, const RigV2& rig) {
    const Pose bind{"bind", {}};
    const auto skin = skinMatrices(rig, poseGlobals(rig, bind));
    const auto posed = skinPositions(rest, sw, skin);
    double maxErr = 0.0;
    for (size_t v = 0; v < rest.size() && v < posed.size(); ++v)
        maxErr = std::max(maxErr, (posed[v] - rest[v]).length());
    return maxErr;
}

} // namespace pae::wp4