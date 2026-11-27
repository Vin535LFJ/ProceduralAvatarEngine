#include "wp4_poses.h"

namespace pae::wp4 {

static LocalRotation R(const std::string& bone, int axis, double deg) { return {bone, axis, deg}; }

// Local axes: 0=X, 1=Y, 2=Z. Signs verified by direction assertions (WP4 run):
//  shoulder abduction: L +Z, R -Z ; forward flexion: -X both
//  elbow flexion: -X (hand toward shoulder, anterior) ; hip flexion: +X ;
//  knee flexion: +X (ankle posterior) ; finger curl: +X toward palm.
std::vector<Pose> poseSuite() {
    std::vector<Pose> s;
    s.push_back({"bind", {}});
    s.push_back({"neutral_standing", {}});

    // Shoulder.
    s.push_back({"shoulder_forward_flexion", {R("upper_arm.L", 0, -60), R("upper_arm.R", 0, -60)}});
    s.push_back({"shoulder_abduction_30", {R("upper_arm.L", 2, 30), R("upper_arm.R", 2, -30)}});
    s.push_back({"shoulder_abduction_60", {R("upper_arm.L", 2, 60), R("upper_arm.R", 2, -60)}});
    s.push_back({"shoulder_abduction_90", {R("upper_arm.L", 2, 90), R("upper_arm.R", 2, -90)}});

    // Elbow.
    s.push_back({"elbow_0", {}});
    s.push_back({"elbow_45", {R("forearm.L", 0, -45), R("forearm.R", 0, -45)}});
    s.push_back({"elbow_90", {R("forearm.L", 0, -90), R("forearm.R", 0, -90)}});
    s.push_back({"elbow_120", {R("forearm.L", 0, -120), R("forearm.R", 0, -120)}});

    // Hip / knee.
    s.push_back({"hip_flexion", {R("thigh.L", 0, 50), R("thigh.R", 0, 50)}});
    s.push_back({"hip_abduction", {R("thigh.L", 2, 25), R("thigh.R", 2, -25)}});
    s.push_back({"knee_45", {R("shin.L", 0, 45), R("shin.R", 0, 45)}});
    s.push_back({"knee_90", {R("shin.L", 0, 90), R("shin.R", 0, 90)}});

    // Wrist / hand.
    s.push_back({"wrist_flexion", {R("hand.L", 0, 40), R("hand.R", 0, 40)}});
    s.push_back({"wrist_extension", {R("hand.L", 0, -40), R("hand.R", 0, -40)}});
    {
        Pose curl{"finger_curl", {}};
        Pose fist{"fist", {}};
        for (const char* side : {"L", "R"}) {
            for (int f = 1; f <= 5; ++f) {
                for (int p = 1; p <= 3; ++p) {
                    const std::string bone = "finger.0" + std::to_string(f) + ".0" + std::to_string(p) + "." + side;
                    curl.rotations.push_back(R(bone, 0, 30));
                    fist.rotations.push_back(R(bone, 0, f == 1 ? 45 : 70));
                }
            }
        }
        s.push_back(curl);
        s.push_back(fist);
    }

    // Whole body.
    s.push_back({"arms_raised", {R("upper_arm.L", 2, 150), R("upper_arm.R", 2, -150)}});
    s.push_back({"one_arm_raised", {R("upper_arm.L", 2, 150)}});
    s.push_back({"basic_squat", {R("thigh.L", 0, 70), R("thigh.R", 0, 70),
                                 R("shin.L", 0, 90), R("shin.R", 0, 90),
                                 R("foot.L", 0, 20), R("foot.R", 0, 20)}});
    s.push_back({"asymmetric", {R("upper_arm.L", 2, 90), R("thigh.R", 0, 40), R("shin.R", 0, 50)}});
    return s;
}

std::vector<Pose> syncSubset() {
    std::vector<Pose> s;
    s.push_back({"bind", {}});
    s.push_back({"shoulder_abduction_60", {R("upper_arm.L", 2, 60), R("upper_arm.R", 2, -60)}});
    s.push_back({"elbow_90", {R("forearm.L", 0, -90), R("forearm.R", 0, -90)}});
    s.push_back({"knee_90", {R("shin.L", 0, 90), R("shin.R", 0, 90)}});
    return s;
}

std::vector<Pose> comparisonPoses() {
    std::vector<Pose> s;
    s.push_back({"neutral_standing", {}});
    s.push_back({"shoulder_abduction_90", {R("upper_arm.L", 2, 90), R("upper_arm.R", 2, -90)}});
    s.push_back({"elbow_90", {R("forearm.L", 0, -90), R("forearm.R", 0, -90)}});
    s.push_back({"hip_flexion", {R("thigh.L", 0, 50), R("thigh.R", 0, 50)}});
    s.push_back({"knee_90", {R("shin.L", 0, 90), R("shin.R", 0, 90)}});
    {
        Pose fist{"fist", {}};
        for (const char* side : {"L", "R"})
            for (int f = 1; f <= 5; ++f)
                for (int p = 1; p <= 3; ++p)
                    fist.rotations.push_back(R("finger.0" + std::to_string(f) + ".0" + std::to_string(p) + "." + side,
                                               0, f == 1 ? 45 : 70));
        s.push_back(fist);
    }
    return s;
}

const Pose* findPose(const std::vector<Pose>& suite, const std::string& name) {
    for (const Pose& p : suite) if (p.name == name) return &p;
    return nullptr;
}

} // namespace pae::wp4