// WP1 unit tests: PAHT v3 format (serialize/parse/validation/reproducibility).

#include "mini_test.hpp"

#include "procedural/human_template/HumanTemplateLoader.h"
#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <cstdio>
#include <string>

using namespace pae::human_template;

namespace {

HumanTemplate makeTinyTemplate() {
    HumanTemplate t;
    t.topologyVersion = "tiny-1";
    // One quad (0,1,2,3) in the XY plane at z=0, plus one aux vertex.
    t.topology.pos = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    t.topology.uv = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    PolyFace f;
    f.v = {0, 1, 2, 3};
    f.vt = {0, 1, 2, 3};
    t.topology.faces.push_back(f);
    t.auxPositions = {{0.5, 0.5, 0.5}};
    t.regions = {Region::Thorax, Region::Thorax, Region::Thorax, Region::Thorax};
    Landmark l;
    l.kind = Landmark::Kind::JointCenter;
    l.name = "tiny-joint";
    l.position = {0.5, 0.5, 0.5};
    l.source = "test";
    l.confidence = "source-defined";
    t.landmarks.push_back(l);
    ShapeTarget s;
    s.id = "width";
    s.name = "width";
    s.region = "thorax";
    s.relPath = "targets/tiny.target";
    s.sha256 = "0000000000000000000000000000000000000000000000000000000000000000";
    s.vertex = {1, 2};
    s.delta = {{0.1, 0, 0}, {0.1, 0, 0}};
    t.shapeTargets.push_back(s);
    RigBone root;
    root.name = "root";
    root.parent = -1;
    root.head = {0.5, 0, 0.5};
    root.tail = {0.5, 1, 0.5};
    root.role = BoneRole::Root;
    root.deform = true;
    t.rig.bones.push_back(root);
    RigBone child;
    child.name = "child";
    child.parent = 0;
    child.head = {0.5, 1, 0.5};
    child.tail = {0.5, 2, 0.5};
    child.role = BoneRole::Spine;
    child.deform = true;
    t.rig.bones.push_back(child);
    t.rig.name = "tiny-rig";
    t.rig.source = "test";
    t.rig.license = "PAE-generated";
    // Weights cover body (4) + aux (1) = 5 vertices.
    t.weights.vertexCount = 5;
    t.weights.maxInfluences = 4;
    t.weights.source = "test";
    t.weights.license = "PAE-generated";
    t.weights.generator = "pae-distance";
    t.weights.bone.assign(5, {0, 0, 0, 0});
    t.weights.w.assign(5, {1.0f, 0, 0, 0});
    t.weights.count.assign(5, 1);
    t.weights.fallback.assign(5, 0);
    t.weights.bone[4] = {1, 0, 0, 0};
    t.provenance.sourceRepo = "https://example.invalid/repo";
    t.provenance.sourceCommit = "0000000000000000000000000000000000000000";
    t.provenance.sourceLicenseSummary = "test";
    t.provenance.converterVersion = "test/1.0";
    t.provenance.unitNote = "metres";
    SourceFile sf;
    sf.path = "tiny.obj";
    sf.sha256 = "1111111111111111111111111111111111111111111111111111111111111111";
    sf.sizeBytes = 123;
    sf.role = "base-mesh";
    sf.license = "CC0-1.0";
    sf.basis = "test";
    t.provenance.files.push_back(sf);
    return t;
}

} // namespace

PAE_TEST_CASE(v3_sha256_known_vector) {
    const std::string abc = "abc";
    EXPECT_TRUE(sha256Hex(abc) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const std::string empty;
    EXPECT_TRUE(sha256Hex(empty) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

PAE_TEST_CASE(v3_roundtrip_byte_identical) {
    const HumanTemplate t = makeTinyTemplate();
    const std::string bytes = serializeTemplate(t);
    HumanTemplate back;
    std::string error;
    EXPECT_TRUE(parseTemplate(bytes, back, error));
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(serializeTemplate(back) == bytes);
    EXPECT_TRUE(back.topology.pos.size() == 4);
    EXPECT_TRUE(back.topology.faces.size() == 1);
    EXPECT_TRUE(back.auxPositions.size() == 1);
    EXPECT_TRUE(back.weights.vertexCount == 5);
    EXPECT_TRUE(back.rig.bones.size() == 2);
    EXPECT_TRUE(back.provenance.files.size() == 1);
    EXPECT_TRUE(back.provenance.files[0].sizeBytes == 123);
}

PAE_TEST_CASE(v3_serialize_deterministic) {
    const HumanTemplate t = makeTinyTemplate();
    EXPECT_TRUE(serializeTemplate(t) == serializeTemplate(t));
}

PAE_TEST_CASE(v3_reject_bad_magic) {
    std::string bytes = serializeTemplate(makeTinyTemplate());
    bytes[0] = 'X';
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(bytes, out, error));
    EXPECT_TRUE(error == "bad magic");
}

PAE_TEST_CASE(v3_reject_bad_version) {
    std::string bytes = serializeTemplate(makeTinyTemplate());
    bytes[4] = static_cast<char>(99);
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(bytes, out, error));
    EXPECT_TRUE(error == "unsupported template version");
}

PAE_TEST_CASE(v3_reject_payload_tamper) {
    const std::string bytes = serializeTemplate(makeTinyTemplate());
    std::string tampered = bytes;
    tampered[60] = static_cast<char>(tampered[60] ^ 0xFF);
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(tampered, out, error));
    EXPECT_TRUE(error == "payload hash mismatch");
}

PAE_TEST_CASE(v3_reject_truncated) {
    const std::string bytes = serializeTemplate(makeTinyTemplate());
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(bytes.substr(0, bytes.size() - 5), out, error));
    EXPECT_FALSE(error.empty());
}

PAE_TEST_CASE(v3_reject_trailing_bytes) {
    std::string bytes = serializeTemplate(makeTinyTemplate());
    bytes += "junk";
    HumanTemplate out;
    std::string error;
    // The hash gate catches appended bytes first (correct behaviour).
    EXPECT_FALSE(parseTemplate(bytes, out, error));
    EXPECT_TRUE(error == "payload hash mismatch");
    // Repair the hash so the structural trailing-bytes check is exercised.
    const std::string hash = sha256Hex(reinterpret_cast<const uint8_t*>(bytes.data()) + 40, bytes.size() - 40);
    for (int i = 0; i < 32; ++i) {
        const unsigned long v = std::stoul(hash.substr(static_cast<size_t>(i) * 2, 2), nullptr, 16);
        bytes[static_cast<size_t>(8 + i)] = static_cast<char>(v);
    }
    EXPECT_FALSE(parseTemplate(bytes, out, error));
    EXPECT_TRUE(error == "trailing bytes after template");
}

PAE_TEST_CASE(v3_reject_bad_face_index) {
    const std::string bytes = serializeTemplate(makeTinyTemplate());
    // Find the first face corner index: header(40) + 5 strings + pos/uv blocks.
    // Instead of hand-computing, tamper via re-serialization of a bad template.
    HumanTemplate bad = makeTinyTemplate();
    bad.topology.faces[0].v[2] = 99;
    const std::string badBytes = serializeTemplate(bad);
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(badBytes, out, error));
    EXPECT_TRUE(error == "face vertex index out of range");
}

PAE_TEST_CASE(v3_reject_bad_region) {
    HumanTemplate bad = makeTinyTemplate();
    bad.regions[1] = static_cast<Region>(200);
    const std::string badBytes = serializeTemplate(bad);
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(badBytes, out, error));
    EXPECT_TRUE(error == "bad region label");
}

PAE_TEST_CASE(v3_reject_weights_not_covering) {
    HumanTemplate bad = makeTinyTemplate();
    bad.weights.vertexCount = 4;  // body only, aux missing
    bad.weights.bone.resize(4);
    bad.weights.w.resize(4);
    bad.weights.count.resize(4);
    bad.weights.fallback.resize(4);
    const std::string badBytes = serializeTemplate(bad);
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(badBytes, out, error));
    EXPECT_TRUE(error == "weights do not cover the topology");
}

PAE_TEST_CASE(v3_reject_unknown_weight_bone) {
    HumanTemplate bad = makeTinyTemplate();
    bad.weights.bone[0] = {7, 0, 0, 0};
    const std::string badBytes = serializeTemplate(bad);
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(parseTemplate(badBytes, out, error));
    EXPECT_TRUE(error == "weight references unknown bone");
}

PAE_TEST_CASE(v3_load_missing_file) {
    HumanTemplate out;
    std::string error;
    EXPECT_FALSE(loadTemplateFile("Z:/definitely/not/a/template.paht", out, error));
    EXPECT_FALSE(error.empty());
}

#ifdef PAE_TEMPLATE_ASSET_PATH
PAE_TEST_CASE(v3_real_asset_load_and_counts) {
    HumanTemplate t;
    std::string error;
    const bool ok = loadTemplateFile(PAE_TEMPLATE_ASSET_PATH, t, error);
    EXPECT_TRUE(ok);
    if (!ok) {
        std::fprintf(stderr, "asset load error: %s\n", error.c_str());
        return;
    }
    EXPECT_TRUE(t.topology.pos.size() == 13380);
    EXPECT_TRUE(t.topology.faces.size() == 13378);
    EXPECT_TRUE(t.auxPositions.size() == 5778);
    EXPECT_TRUE(t.shapeTargets.size() == 18);
    EXPECT_TRUE(t.rig.bones.size() == 20);
    EXPECT_TRUE(t.regions.size() == 13380);
    EXPECT_TRUE(t.weights.vertexCount == 19158);
    EXPECT_TRUE(t.provenance.sourceCommit == "a8bc2d54ff0ac92e78ff71431b1023eda42bf482");
    // Roundtrip on the real asset.
    const std::string bytes = serializeTemplate(t);
    HumanTemplate back;
    EXPECT_TRUE(parseTemplate(bytes, back, error));
    EXPECT_TRUE(serializeTemplate(back) == bytes);
}
#endif

int main() { return pae::test::runAllTests(); }