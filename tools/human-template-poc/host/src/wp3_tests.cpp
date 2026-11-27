// VR-1B-WP3-HOST-01 unit tests: measurement protocol primitives.
// Pure synthetic geometry; no MakeHuman assets required.

#include "ht_common.h"
#include "ht_obj.h"
#include "wp3_measure.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace pae::ht;

namespace {

int gPass = 0, gFail = 0;
void check(bool ok, const char* name, const std::string& detail = "") {
    if (ok) { ++gPass; std::printf("[PASS] %s\n", name); }
    else { ++gFail; std::printf("[FAIL] %s %s\n", name, detail.c_str()); }
}
bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// Closed unit box [0,1]^3 as 6 quad faces (outward winding).
PolyMesh unitBox() {
    PolyMesh m;
    m.pos = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    auto face = [&](std::vector<int> v) { PolyFace f; f.v = std::move(v); f.vt.assign(f.v.size(), -1); m.faces.push_back(f); };
    face({0,3,2,1});  // bottom (y=0), outward -Y
    face({4,5,6,7});  // top (y=1), outward +Y
    face({0,1,5,4});  // front z=0
    face({2,3,7,6});  // back z=1
    face({1,2,6,5});  // right x=1
    face({0,4,7,3});  // left x=0
    return m;
}

} // namespace

int main() {
    const PolyMesh box = unitBox();

    // Horizontal cut at y=0.5: square perimeter 4, breadth 1, depth 1, one closed loop.
    const pae::wp3::Section mid = pae::wp3::cutSection(box.pos, box.faces, {0,1,0}, {0,0.5,0});
    check(near(mid.perimeter, 4.0, 1e-6), "box_mid_perimeter", std::to_string(mid.perimeter));
    check(mid.loops.size() == 1 && mid.openChains == 0, "box_mid_closed_loop");
    const pae::wp3::SectionLoop* ml = mid.largestArea();
    check(ml && near(ml->breadth, 1.0, 1e-6) && near(ml->depth, 1.0, 1e-6), "box_mid_breadth_depth");
    check(ml && near(ml->area, 1.0, 1e-6), "box_mid_area", ml ? std::to_string(ml->area) : "null");

    // Cut near the top: still a unit square.
    const pae::wp3::Section hi = pae::wp3::cutSection(box.pos, box.faces, {0,1,0}, {0,0.999,0});
    check(near(hi.perimeter, 4.0, 1e-6), "box_high_perimeter");

    // No intersection above the box.
    const pae::wp3::Section none = pae::wp3::cutSection(box.pos, box.faces, {0,1,0}, {0,2.0,0});
    check(near(none.perimeter, 0.0, 1e-9) && none.points == 0, "box_no_intersection");

    // Oblique 45-degree cut through the centre: rectangle 1 x sqrt(2).
    const pae::wp3::Section obl = pae::wp3::cutSection(box.pos, box.faces, {0,1,1}, {0.5,0.5,0.5});
    check(near(obl.perimeter, 2.0 + 2.0 * std::sqrt(2.0), 1e-6), "box_oblique_perimeter", std::to_string(obl.perimeter));
    check(obl.openChains == 0 && obl.loops.size() == 1, "box_oblique_closed");

    // Two disjoint boxes: the section must separate loops; largestArea picks
    // the unit box, nearestTo picks the small box near its own centre.
    PolyMesh two = box;
    const size_t off = two.pos.size();
    for (const D3& p : box.pos) two.pos.push_back(D3{p.x * 0.2 + 3.0, p.y * 0.2, p.z * 0.2});
    for (const PolyFace& f : box.faces) {
        PolyFace g;
        for (int v : f.v) g.v.push_back(static_cast<int>(v + off));
        g.vt.assign(g.v.size(), -1);
        two.faces.push_back(g);
    }
    const pae::wp3::Section both = pae::wp3::cutSection(two.pos, two.faces, {0,1,0}, {0,0.1,0});
    check(both.loops.size() == 2, "two_box_loop_count", std::to_string(both.loops.size()));
    const pae::wp3::SectionLoop* big = both.largestArea();
    check(big && near(big->area, 1.0, 1e-6), "two_box_largest_area");
    const pae::wp3::SectionLoop* small = both.nearestTo({3.1, 0.1, 0.1});
    check(small && near(small->area, 0.04, 1e-6), "two_box_nearest", small ? std::to_string(small->area) : "null");

    // Region linkage: synthetic deltas on a 4-vertex body + 1 helper.
    std::vector<Region> regions = {Region::Thorax, Region::Thorax, Region::UpperArm, Region::UpperArm};
    std::vector<int> vertex = {0, 1, 2, 3, 4};
    std::vector<D3> delta = {{0.01,0,0},{0.03,0,0},{0.10,0,0},{0.20,0,0},{0.50,0,0}};
    const pae::wp3::RegionLinkage l = pae::wp3::regionLinkage(regions, vertex, delta, 4);
    check(near(l.mean[static_cast<int>(Region::Thorax)], 0.02, 1e-9), "linkage_thorax_mean");
    check(near(l.max[static_cast<int>(Region::UpperArm)], 0.20, 1e-9), "linkage_arm_max");
    check(near(l.helperMean, 0.50, 1e-9), "linkage_helper_mean");

    std::printf("== summary: %d passed, %d failed ==\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}