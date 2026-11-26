#pragma once

// Software z-buffer renderer for the host-only evidence images. The camera is
// PAE's own CameraProtocol::fixedViews()/viewFrame(), the shading formula and
// light are the ones of PAE's ClayRenderer, and PNG output goes through PAE's
// ImageWriter. This is NOT the Android GLES renderer and proves nothing about
// GLES3 compatibility.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_rig.h"

#include <string>
#include <vector>

namespace pae::ht {

enum class ShadeMode : uint8_t { Clay, Wireframe, ViewNormal, WeightHeat, RegionColor };

struct Camera {
    float right[3], up[3], dir[3];  // orthonormal view frame (PAE ViewFrame)
    std::string name;
    float uMin = -0.5f, uMax = 0.5f, vMin = 0.0f, vMax = 1.0f;  // metres
    int width = 320, height = 640;
};

struct RenderSurface {
    const PolyMesh* mesh = nullptr;        // topology + UVs (quad faces)
    const std::vector<D3>* positions = nullptr;  // current positions (metres)
    const std::vector<Region>* regions = nullptr;
    const SkinWeights* weights = nullptr;
    const Rig* rig = nullptr;
};

// The seven fixed views of PAE's camera protocol plus the character's right
// side (the brief asks for left AND right; the protocol only has side-left).
std::vector<Camera> protocolCameras(int width, int height);

// Free camera for anatomy close-ups (azimuth 0 = in front of the body,
// 90 = the character's left; negative elevation = looking from below).
Camera makeCamera(const std::string& name, double azimuthDeg, double elevationDeg, int width, int height);

// Fits a camera to one axis-aligned box (metres) instead of the whole body.
void fitCameraToBox(Camera& camera, const D3& lo, const D3& hi);

// Fits one shared orthographic frame per view over ALL position sets so that
// every rendered variant shares the same camera (comparable at a glance).
void fitSharedFrames(std::vector<Camera>& cameras, const std::vector<const std::vector<D3>*>& positionSets);

struct Rgb {
    int width = 0, height = 0;
    std::vector<uint8_t> pixels;  // RGB, row major from the top
};
Rgb renderSurface(const RenderSurface& surface, const Camera& camera, ShadeMode mode, bool wireOverlay);

// Writes through PAE's dependency-free PNG writer. Returns false when the
// prototype was built without PAE_REPO.
bool writePngFile(const std::string& path, const Rgb& image);

// Cross-check of this rasterizer against PAE's own ClayRenderer: renders the
// same surface with renderClayView() and returns the silhouette IoU.
double claySilhouetteIoUWithPae(const PolyMesh& mesh, const Camera& camera);

} // namespace pae::ht
