// PoC-SM round 2: regenerate the gray-clay evidence set for this round.
//
// The first seven views use the UNCHANGED CameraProtocol::fixedViews() and
// the same vertical window as the previous slice, so the images are directly
// comparable with evidence/poc-sm/shared-envelope-*.png. Two additional
// close-ups use a narrower window to expose the shoulder/chest transition and
// the axilla concavity. No image is reused from an earlier round.

#include "procedural/shape/CameraProtocol.h"
#include "procedural/shape/ClayRenderer.h"
#include "procedural/shape/ImageWriter.h"
#include "procedural/shape/v2/SharedSkinEnvelope.h"
#include "skeleton/Skeleton.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace pae;
using namespace pae::shape;
using namespace pae::shape::v2;

namespace {

std::string outDir = "evidence/poc-sm/round-2/views";

bool ensureDir(const std::string& path) {
    const std::string cmd = "mkdir -p \"" + path + "\"";
    return std::system(cmd.c_str()) == 0;
}

struct ViewSpec {
    View view;
    float yMin = 0.0f;
    float yMax = 0.0f;
    std::string prefix;
};

} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) outDir = argv[++i];
    }
    if (!ensureDir(outDir)) return 2;

    const BodyJoints joints{};
    SharedSkinEnvelopeSpec spec;
    spec.rings = 26;
    spec.segments = 32;
    const SharedSkinEnvelopeResult envelope =
        buildSharedSkinEnvelope(defaultCharacterDefinition(), spec, joints);
    if (!envelope.valid) {
        std::fprintf(stderr, "envelope build failed: %s\n", envelope.error.c_str());
        return 2;
    }

    const float yMinFull = joints.hips.y - 0.02f;
    const float yMaxFull = joints.chest.y + 0.075f;

    std::vector<ViewSpec> specs;
    for (const View& v : fixedViews()) {
        specs.push_back({v, yMinFull, yMaxFull, "fixed"});
    }
    // Close-ups: same orthographic renderer and light, narrower clip window.
    specs.push_back({View{"shoulder-chest-closeup", 45.0f, 10.0f},
                     joints.chest.y - 0.06f, joints.chest.y + 0.10f, "closeup"});
    specs.push_back({View{"axilla-closeup", 90.0f, 15.0f},
                     joints.chest.y - 0.04f, joints.chest.y + 0.09f, "closeup"});

    std::FILE* manifest = std::fopen((outDir + "/camera-manifest.csv").c_str(), "wb");
    if (manifest == nullptr) return 2;
    std::fprintf(manifest, "kind,view,azimuth_deg,elevation_deg,y_min_m,y_max_m,width,height,file\n");

    int written = 0;
    for (const ViewSpec& s : specs) {
        const std::vector<View> oneView = {s.view};
        const std::vector<const MeshData*> meshes = {&envelope.mesh};
        const std::vector<ClayFrame> frames =
            computeSharedClayFrames(meshes, oneView, s.yMin, s.yMax, 640, 640);
        if (frames.empty()) continue;
        const RgbImage image = renderClayView(envelope.mesh, s.view, frames[0], s.yMin, s.yMax);
        const std::string name = s.prefix + "-" + s.view.name + ".png";
        const std::string path = outDir + "/" + name;
        if (writePng(path, image)) {
            std::fprintf(manifest, "%s,%s,%.1f,%.1f,%.4f,%.4f,640,640,%s\n",
                         s.prefix.c_str(), s.view.name.c_str(), s.view.azimuthDeg,
                         s.view.elevationDeg, s.yMin, s.yMax, name.c_str());
            ++written;
        }
    }
    std::fclose(manifest);
    std::printf("gray-clay images written: %d\n", written);
    std::printf("manifest: %s/camera-manifest.csv\n", outDir.c_str());
    return written == static_cast<int>(specs.size()) ? 0 : 2;
}
