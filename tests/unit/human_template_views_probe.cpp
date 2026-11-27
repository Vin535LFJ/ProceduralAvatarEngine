// WP2 host evidence probe: load the distributed .paht template, build the
// static render mesh, and render the PAE CameraProtocol 7 fixed views with the
// engine's own software clay renderer. This is the HOST preview of what the
// Android GLES3 path must show; it is NOT the Android renderer.
//
// Usage: pae_human_template_views_probe <template.paht> <out-dir>

#include "procedural/human_template/BaseHumanRenderMeshBuilder.h"
#include "procedural/human_template/HumanTemplateLoader.h"
#include "procedural/shape/CameraProtocol.h"
#include "procedural/shape/ClayRenderer.h"
#include "procedural/shape/ImageWriter.h"

#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: pae_human_template_views_probe <template.paht> <out-dir>\n");
        return 2;
    }
    pae::human_template::HumanTemplate t;
    std::string error;
    if (!pae::human_template::loadTemplateFile(argv[1], t, error)) {
        std::fprintf(stderr, "[error] load: %s\n", error.c_str());
        return 1;
    }
    pae::MeshData mesh;
    pae::human_template::StaticRenderMeshReport report;
    if (!pae::human_template::buildStaticRenderMesh(t, mesh, report, error)) {
        std::fprintf(stderr, "[error] mesh: %s\n", error.c_str());
        return 1;
    }
    std::printf("[probe] verts=%u tris=%u quads=%u degenerate=%u\n",
                report.vertexCount, report.triangleCount, report.quadCount, report.degenerateTriangles);

    const std::vector<pae::shape::View>& views = pae::shape::fixedViews();
    const float yMin = mesh.bounds.min.y;
    const float yMax = mesh.bounds.max.y;
    const std::vector<const pae::MeshData*> meshes = {&mesh};
    const std::vector<pae::shape::ClayFrame> frames =
        pae::shape::computeSharedClayFrames(meshes, views, yMin, yMax, 480, 640);

    const std::string outDir = argv[2];
    int written = 0;
    for (size_t i = 0; i < views.size(); ++i) {
        const pae::shape::RgbImage img = pae::shape::renderClayView(mesh, views[i], frames[i], yMin, yMax);
        const std::string path = outDir + "/template-" + views[i].name + ".png";
        if (pae::shape::writePng(path, img)) {
            ++written;
            std::printf("[view] %s -> %s\n", views[i].name.c_str(), path.c_str());
        } else {
            std::fprintf(stderr, "[error] write failed: %s\n", path.c_str());
        }
    }
    std::printf("[done] %d/%zu views written\n", written, views.size());
    return written == static_cast<int>(views.size()) ? 0 : 1;
}