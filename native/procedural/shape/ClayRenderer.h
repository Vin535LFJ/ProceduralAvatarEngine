#pragma once

// G2 (SPEC 8.3 Layer A item 5 / 8.4 / 13 #1): grey-clay fixed-view export.
//
// This is a SOFTWARE renderer used only to produce owner-facing evidence. It
// deliberately reuses viewFrame() from the numeric camera protocol, so the
// recorded images and the silhouette numbers come from the exact same camera.
// It does NOT replace the Android GLES renderer and is NOT a production entry.

#include "procedural/MeshData.h"
#include "procedural/shape/CameraProtocol.h"
#include "procedural/shape/ImageWriter.h"

#include <vector>

namespace pae::shape {

struct ClayFrame {
    int width = 480;
    int height = 480;
    float uMin = -0.4f;
    float uMax = 0.4f;
    float vMin = -0.4f;
    float vMax = 0.4f;
};

// One orthographic frame PER VIEW, computed from ALL routes and ALL parameter
// groups together. Every route rendered in a given view gets exactly the same
// frame, so no route can receive a tighter crop than another.
std::vector<ClayFrame> computeSharedClayFrames(const std::vector<const MeshData*>& meshes,
                                               const std::vector<View>& views,
                                               float yMin, float yMax,
                                               int width, int height);

// Lambert grey clay with a fixed light in the view frame.
// Light components are (right, up, towards camera).
const Vec3& clayLightInViewFrame();

RgbImage renderClayView(const MeshData& mesh, const View& view,
                        const ClayFrame& frame, float yMin, float yMax);

} // namespace pae::shape
