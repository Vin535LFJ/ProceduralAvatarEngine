#pragma once

// PAEHumanTemplate: the versioned, offline-compiled human template of the
// host-only proof. Binary container "PAHT", version 2.
//
// It stores the QUAD topology (no triangulation), stable vertex ids, face
// corner UVs, anatomical region labels, landmarks, shape targets, the Rig V2
// definition, Top-N skin weights with a per-vertex fallback flag (source /
// kept-ancestor / nearest-bone) and the full asset provenance.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_rig.h"

#include <string>
#include <vector>

namespace pae::ht {

struct SourceFile {
    std::string path;     // relative to the asset root
    std::string sha256;
    long sizeBytes = 0;
    std::string role;     // "base-mesh", "rig", "weights", "target", "license-text"
    // "CC0-1.0", "PENDING_CONFIRMATION (CC0 declared in file only)" (rig + weights), "LICENSE_UNVERIFIED"
    std::string license;
    std::string basis;    // why this license applies
};

struct TemplateProvenance {
    std::string sourceRepo;
    std::string sourceCommit;
    std::string sourceLicenseSummary;
    std::string converterVersion;
    std::string convertedAtUtc;
    double unitScaleToMetres = 0.1;
    std::string unitNote;
    std::vector<SourceFile> files;
};

struct Landmark {
    enum class Kind : uint8_t { JointCenter = 0, Surface = 1, Derived = 2 };
    Kind kind = Kind::JointCenter;
    std::string name;
    D3 position;
    std::string source;
    std::string confidence;  // "source-defined", "auto-derived"
};

struct TemplateShapeTarget {
    std::string id, name, region, relPath, sha256;
    double rangeMin = 0.0, rangeMax = 1.0;
    uint8_t side = 0;  // 0 centre, 1 left, 2 right, 3 left+right pair
    std::vector<int> vertex;
    std::vector<D3> delta;
};

struct HumanTemplate {
    std::string name = "PAEHumanTemplate";
    std::string topologyVersion;
    std::string units = "m";
    std::string upAxis = "+Y";
    std::string forwardAxis = "+Z";
    PolyMesh topology;  // body: pos in metres (stable vertex id order), quad faces
    // Helper + joint vertices of the source OBJ (positions only, stable ids
    // continue right after the body). Shape targets address them too and the
    // rig's joint centres are the mean of joint-cube vertices, so they are
    // part of the template even though they are never rendered.
    std::vector<D3> auxPositions;
    std::vector<Region> regions;
    std::vector<Landmark> landmarks;
    std::vector<TemplateShapeTarget> shapeTargets;
    Rig rig;
    SkinWeights weights;
    TemplateProvenance provenance;
};

std::string serializeTemplate(const HumanTemplate& t);
bool parseTemplate(const std::string& bytes, HumanTemplate& out, std::string& error);

} // namespace pae::ht
