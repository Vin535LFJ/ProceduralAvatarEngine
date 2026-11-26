#include "ht_obj.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace pae::ht {

namespace {

// Resolves an OBJ index (1-based, or negative = relative to current end).
bool resolveIndex(const std::string& token, int count, int lineNo, int& out, int& negativeUses) {
    long raw = 0;
    if (!parseLong(token, raw)) return false;
    if (raw > 0) {
        if (raw > count) return false;
        out = static_cast<int>(raw - 1);
    } else if (raw < 0) {
        const long resolved = static_cast<long>(count) + raw;
        if (resolved < 0 || resolved >= count) return false;
        out = static_cast<int>(resolved);
        ++negativeUses;
    } else {
        (void)lineNo;
        return false;  // OBJ indices are never zero
    }
    return true;
}

} // namespace

bool readObj(const std::string& path, PolyMesh& out, std::string& error) {
    std::string text;
    if (!readFile(path, text)) {
        error = "cannot open " + path;
        return false;
    }
    out = PolyMesh{};
    out.sourcePath = path;
    out.sourceSha256 = sha256HexFile(path);

    std::istringstream in(text);
    std::string line;
    std::string currentGroup;
    std::string currentMtl;
    int lineNo = 0;
    std::map<std::string, bool> seenGroup;
    while (std::getline(in, line)) {
        ++lineNo;
        const std::vector<std::string> t = tokenize(line);
        if (t.empty()) continue;
        if (t[0] == "#") { ++out.commentLineCount; continue; }
        if (t[0] == "v") {
            if (t.size() < 4) { error = fmt("line %d: bad v", lineNo); return false; }
            D3 p;
            if (!parseDouble(t[1], p.x) || !parseDouble(t[2], p.y) || !parseDouble(t[3], p.z)) {
                error = fmt("line %d: bad v", lineNo);
                return false;
            }
            out.pos.push_back(p);
        } else if (t[0] == "vt") {
            if (t.size() < 3) { error = fmt("line %d: bad vt", lineNo); return false; }
            D2 uv;
            if (!parseDouble(t[1], uv.u) || !parseDouble(t[2], uv.v)) {
                error = fmt("line %d: bad vt", lineNo);
                return false;
            }
            out.uv.push_back(uv);
        } else if (t[0] == "vn") {
            ++out.normalCount;
        } else if (t[0] == "g") {
            std::string name;
            for (size_t i = 1; i < t.size(); ++i) name += (i > 1 ? " " : "") + t[i];
            currentGroup = name;
            if (!seenGroup.count(name)) {
                seenGroup[name] = true;
                out.groupOrder.push_back(name);
            }
        } else if (t[0] == "usemtl") {
            currentMtl = t.size() > 1 ? t[1] : "";
        } else if (t[0] == "f") {
            if (t.size() < 4) { error = fmt("line %d: face with <3 corners", lineNo); return false; }
            PolyFace f;
            for (size_t i = 1; i < t.size(); ++i) {
                const std::string& c = t[i];
                const size_t s1 = c.find('/');
                const size_t s2 = s1 == std::string::npos ? std::string::npos : c.find('/', s1 + 1);
                const std::string vi = c.substr(0, s1);
                const std::string ti = (s1 == std::string::npos) ? "" : c.substr(s1 + 1, (s2 == std::string::npos ? std::string::npos : s2 - s1 - 1));
                int v = -1, vt = -1;
                if (!resolveIndex(vi, static_cast<int>(out.pos.size()), lineNo, v, out.faceStatementsWithNegativeIndex)) {
                    error = fmt("line %d: bad vertex index '%s'", lineNo, c.c_str());
                    return false;
                }
                if (!ti.empty() && ti != "0") {
                    if (!resolveIndex(ti, static_cast<int>(out.uv.size()), lineNo, vt, out.faceStatementsWithNegativeIndex)) {
                        error = fmt("line %d: bad uv index '%s'", lineNo, c.c_str());
                        return false;
                    }
                }
                f.v.push_back(v);
                f.vt.push_back(vt);
            }
            out.faces.push_back(f);
            out.faceGroup.push_back(currentGroup);
            out.faceLine.push_back(lineNo);
            out.useMtl.push_back(currentMtl);
        }
        // mtllib / o / s / l / p are tolerated and ignored by this reader.
    }
    return true;
}

std::vector<int> PolyMesh::facesOfGroup(const std::string& group) const {
    std::vector<int> out;
    for (size_t i = 0; i < faceGroup.size(); ++i)
        if (faceGroup[i] == group) out.push_back(static_cast<int>(i));
    return out;
}

std::vector<std::string> PolyMesh::groupNamesWithPrefix(const std::string& prefix) const {
    std::vector<std::string> out;
    for (const std::string& g : groupOrder)
        if (g.compare(0, prefix.size(), prefix) == 0) out.push_back(g);
    return out;
}

void PolyMesh::extractGroup(const std::string& group, PolyMesh& out, std::vector<int>& usedVertex) const {
    out = PolyMesh{};
    out.sourcePath = sourcePath;
    out.sourceSha256 = sourceSha256;
    out.uv = uv;
    usedVertex.clear();
    std::vector<int> compact(pos.size(), -1);
    for (size_t i = 0; i < faceGroup.size(); ++i) {
        if (faceGroup[i] != group) continue;
        PolyFace f;
        for (size_t k = 0; k < faces[i].v.size(); ++k) {
            const int vi = faces[i].v[k];
            if (compact[vi] < 0) {
                compact[vi] = static_cast<int>(out.pos.size());
                out.pos.push_back(pos[vi]);
                usedVertex.push_back(vi);
            }
            f.v.push_back(compact[vi]);
            f.vt.push_back(faces[i].vt[k]);
        }
        out.faces.push_back(f);
        out.faceGroup.push_back(group);
        out.faceLine.push_back(faceLine[i]);
        out.useMtl.push_back(useMtl[i]);
    }
    out.groupOrder = {group};
    // First-encounter order is not the file order. Re-index so that compact id
    // k always means the k-th USED `v` line of the file (ascending stable ids).
    if (!usedVertex.empty()) {
        std::vector<int> sorted = usedVertex;
        std::sort(sorted.begin(), sorted.end());
        std::vector<int> toSorted(usedVertex.size());
        for (size_t old = 0; old < usedVertex.size(); ++old)
            toSorted[old] = static_cast<int>(std::lower_bound(sorted.begin(), sorted.end(), usedVertex[old]) - sorted.begin());
        PolyMesh ordered = out;
        ordered.pos.clear();
        for (int id : sorted) ordered.pos.push_back(pos[static_cast<size_t>(id)]);
        for (PolyFace& face : ordered.faces)
            for (int& v : face.v) v = toSorted[static_cast<size_t>(v)];
        out = ordered;
        usedVertex = sorted;
    }
}

bool writeTriangulatedObj(const std::string& path, const PolyMesh& mesh) {
    std::string text;
    text += "# PAE host-only template proof: neutral triangulated export\n";
    text += "# vertex order preserved from PAE template stable vertex ids\n";
    for (const D3& p : mesh.pos)
        text += fmt("v %.6f %.6f %.6f\n", p.x, p.y, p.z);
    for (const PolyFace& f : mesh.faces) {
        for (size_t k = 1; k + 1 < f.v.size(); ++k)
            text += fmt("f %d %d %d\n", f.v[0] + 1, f.v[k] + 1, f.v[k + 1] + 1);
    }
    return writeFile(path, text);
}

} // namespace pae::ht
