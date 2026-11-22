// M12-A proportion probe: objective torso cross-sections + joint dump.
// Not a pass/fail test — a diagnostic that prints the numbers a user sees.
#include "procedural/humanoid/Humanoid.h"
#include "procedural/humanoid/anatomy/AnatomyField.h"

#include <cmath>
#include <cstdio>

using namespace pae;
using namespace pae::anatomy;

int main() {
    BodyParameters bp{};
    const BodyJoints j = deriveJoints(bp, 1.0f);
    std::printf("== joints (m) ==\n");
    std::printf("hips   (%.3f, %.3f, %.3f)\n", j.hips.x, j.hips.y, j.hips.z);
    std::printf("spine  (%.3f, %.3f, %.3f)\n", j.spine.x, j.spine.y, j.spine.z);
    std::printf("chest  (%.3f, %.3f, %.3f)\n", j.chest.x, j.chest.y, j.chest.z);
    std::printf("neck   (%.3f, %.3f, %.3f)\n", j.neck.x, j.neck.y, j.neck.z);
    std::printf("head   (%.3f, %.3f, %.3f)\n", j.head.x, j.head.y, j.head.z);
    std::printf("Lshldr (%.3f, %.3f, %.3f)\n", j.leftShoulder.x, j.leftShoulder.y, j.leftShoulder.z);
    std::printf("Lelbow (%.3f, %.3f, %.3f)\n", j.leftElbow.x, j.leftElbow.y, j.leftElbow.z);
    std::printf("Lwrist (%.3f, %.3f, %.3f)\n", j.leftWrist.x, j.leftWrist.y, j.leftWrist.z);
    const float armLen = (j.leftElbow - j.leftShoulder).length()
                       + (j.leftWrist - j.leftElbow).length();
    std::printf("arm length (shoulder->wrist) = %.3f m\n", armLen);
    std::printf("shoulder joint X = %.3f (biacromial width = %.3f m)\n",
                j.leftShoulder.x, 2.0f * j.leftShoulder.x);
    std::printf("head height param = %.3f, head top ~ %.3f, total height ~ %.3f\n",
                bp.headHeight, j.head.y + bp.headHeight * 0.5f, j.head.y + bp.headHeight * 0.5f);

    HumanAnatomyParameters ap{};
    auto halfWidthAt = [&](float y, float z) {
        float prev = upperBodySdf(ap, j, Vec3{0.0f, y, z});
        for (float x = 0.005f; x < 0.7f; x += 0.0025f) {
            float cur = upperBodySdf(ap, j, Vec3{x, y, z});
            if (prev < 0.0f && cur >= 0.0f) return x;
            prev = cur;
        }
        return -1.0f;
    };
    auto halfDepthAt = [&](float y, float x) {
        float prev = upperBodySdf(ap, j, Vec3{x, y, 0.0f});
        for (float z = 0.005f; z < 0.5f; z += 0.0025f) {
            float cur = upperBodySdf(ap, j, Vec3{x, y, z});
            if (prev < 0.0f && cur >= 0.0f) return z;
            prev = cur;
        }
        return -1.0f;
    };
    std::printf("\n== torso cross-section half-widths at z=0 (m), arm-immune ==\n");
    const float ys[] = {1.68f, 1.60f, 1.52f, 1.45f, 1.38f, 1.31f, 1.24f,
                        1.17f, 1.10f, 1.03f, 0.96f, 0.90f};
    const char* names[] = {"head top", "mid head", "neck base", "clavicle",
                           "upper chest", "chest", "mid torso", "lower chest",
                           "spine", "abdomen", "waist", "hips"};
    for (int i = 0; i < 12; ++i) {
        float hw = halfWidthAt(ys[i], 0.0f);
        std::printf("  y=%.2f %-12s halfW=%s fullW=%.3f m\n", ys[i], names[i],
                    hw < 0 ? "  --  " : "", hw < 0 ? -1.0f : 2.0f * hw);
    }
    std::printf("\n== torso depth (front z+) at x=0 ==\n");
    for (int i = 0; i < 12; ++i) {
        float hd = halfDepthAt(ys[i], 0.0f);
        std::printf("  y=%.2f depth(z+)=%.3f m\n", ys[i], hd < 0 ? -1.0f : hd);
    }

    // Silhouette extremes (what dominates the rendered outline).
    std::printf("\n== silhouette max |x| per height band (includes arms) ==\n");
    for (int i = 0; i < 12; ++i) {
        float hw = halfWidthAt(ys[i], 0.0f); // torso only at z=0
        // full silhouette: scan at the arm plane too by taking max over z
        float best = hw;
        for (float z = -0.12f; z <= 0.14f; z += 0.02f) {
            float h = halfWidthAt(ys[i], z);
            if (h > best) best = h;
        }
        std::printf("  y=%.2f %-12s torsoHW=%.3f  silhouetteHW=%.3f (full %.3f)\n",
                    ys[i], names[i], hw < 0 ? -1.0f : hw, best, 2.0f * best);
    }
    return 0;
}