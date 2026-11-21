#include "mini_test.hpp"

#include "config/EngineConfig.h"
#include "math/Math.h"
#include "procedural/humanoid/Humanoid.h"

#include <cmath>
#include <string>

using namespace pae;

PAE_TEST_CASE(vec3_basic_operations) {
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, -1.0f, 2.0f};
    EXPECT_NEAR((a + b).x, 5.0f, 1e-6);
    EXPECT_NEAR(a.dot(b), 1.0f * 4.0f + 2.0f * -1.0f + 3.0f * 2.0f, 1e-6);
    EXPECT_NEAR(a.cross(b).x, 2.0f * 2.0f - 3.0f * -1.0f, 1e-6);
    EXPECT_TRUE(a.finite());
    const Vec3 n = a.normalized();
    EXPECT_NEAR(n.length(), 1.0f, 1e-5);
}

PAE_TEST_CASE(quat_rotation_matches_expectation) {
    const Quat q = Quat::fromToRotation({1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    const Vec3 rotated = q.rotate({1.0f, 0.0f, 0.0f});
    EXPECT_NEAR(rotated.x, 0.0f, 1e-5);
    EXPECT_NEAR(rotated.y, 1.0f, 1e-5);
    EXPECT_NEAR(rotated.z, 0.0f, 1e-5);
    const Quat slerped = Quat::slerp(Quat{}, q, 0.5f);
    EXPECT_NEAR(slerped.lengthSq(), 1.0f, 1e-5);
}

PAE_TEST_CASE(mat4_inverse_roundtrip) {
    const Mat4 m = Mat4::trs({1.0f, -2.0f, 3.0f}, Quat{0.0f, 0.70710678f, 0.0f, 0.70710678f},
                             {1.5f, 2.0f, 0.5f});
    Mat4 inv;
    EXPECT_TRUE(m.inverse(inv));
    const Mat4 product = m * inv;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            EXPECT_NEAR(product.at(c, r), c == r ? 1.0f : 0.0f, 1e-4);
        }
    }
}

PAE_TEST_CASE(engine_config_validation) {
    EngineConfig good;
    std::string reason;
    EXPECT_TRUE(validateEngineConfig(good, &reason));
    EXPECT_TRUE(reason.empty());

    EngineConfig bad = good;
    bad.maxQueuedCommands = 0;
    EXPECT_FALSE(validateEngineConfig(bad, &reason));
    EXPECT_TRUE(!reason.empty());

    bad = good;
    bad.targetFps = std::nanf("");
    EXPECT_FALSE(validateEngineConfig(bad, nullptr));
}

PAE_TEST_CASE(quality_levels_map_to_geometry_detail) {
    EXPECT_TRUE(detailLevelForQuality(QualityLevel::Low) == 0u);
    EXPECT_TRUE(detailLevelForQuality(QualityLevel::Medium) == 1u);
    EXPECT_TRUE(detailLevelForQuality(QualityLevel::High) == 2u);

    // The detail level must produce measurably different geometry tiers.
    BodyParameters low{};
    low.detailLevel = detailLevelForQuality(QualityLevel::Low);
    BodyParameters high{};
    high.detailLevel = detailLevelForQuality(QualityLevel::High);
    const HumanoidResult lowResult = generateHumanoid(low, false);
    const HumanoidResult highResult = generateHumanoid(high, false);
    EXPECT_TRUE(lowResult.valid && highResult.valid);
    EXPECT_TRUE(highResult.mesh.indices.size() > lowResult.mesh.indices.size());
}

PAE_TEST_CASE(aabb_expands_and_reports_size) {
    Aabb box;
    EXPECT_FALSE(box.valid);
    box.expand({-1.0f, 0.0f, 2.0f});
    box.expand({3.0f, 4.0f, -2.0f});
    EXPECT_TRUE(box.valid);
    EXPECT_NEAR(box.size().x, 4.0f, 1e-6);
    EXPECT_NEAR(box.size().y, 4.0f, 1e-6);
    EXPECT_NEAR(box.size().z, 4.0f, 1e-6);
}

int main() {
    return test::runAllTests();
}
