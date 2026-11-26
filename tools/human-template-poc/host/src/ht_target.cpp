#include "ht_target.h"

#include <algorithm>
#include <fstream>

namespace pae::ht {

namespace {

struct RawRow {
    long index = 0;
    D3 d;
};

// Parses the data rows of a target file. Rows are "<index> <a> <b> <c>";
// anything whose first token is not an integer is a category header line
// (recorded, not interpreted). Comment and quote lines are skipped.
bool readRawRows(const std::string& path, std::vector<RawRow>& rows, std::vector<std::string>& headers,
                 long& commentLines, long& malformed, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot open " + path;
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        const std::vector<std::string> t = tokenize(line);
        if (t.empty()) continue;
        if (t[0][0] == '#' || t[0][0] == '"') { ++commentLines; continue; }
        long index = 0;
        D3 d;
        const bool numeric = t.size() >= 4 && parseLong(t[0], index) && parseDouble(t[1], d.x) &&
                             parseDouble(t[2], d.y) && parseDouble(t[3], d.z);
        if (numeric) {
            rows.push_back({index, d});
        } else if (parseLong(t[0], index) && t.size() < 4) {
            ++malformed;  // an index with fewer than three offsets is not a valid row
        } else {
            headers.push_back(t[0]);  // category header: recorded, never interpreted
        }
    }
    return true;
}

} // namespace

bool readTarget(const std::string& path, const std::string& relPath, long bodyVertexCount, int indexBase,
                ShapeTarget& out, std::string& error) {
    out = ShapeTarget{};
    out.relPath = relPath;
    const size_t slash = relPath.find_last_of("/\\");
    const std::string stem = relPath.substr(slash + 1);
    out.name = stem.substr(0, stem.find_last_of('.'));
    out.indexBase = indexBase;
    std::vector<RawRow> rows;
    if (!readRawRows(path, rows, out.categoryHeaders, out.commentLines, out.malformedLines, error)) return false;
    out.sha256 = sha256HexFile(path);
    out.dataRows = static_cast<long>(rows.size());
    std::vector<std::pair<long, D3>> merged;
    merged.reserve(rows.size());
    for (const RawRow& r : rows) {
        const long id = static_cast<long>(r.index) - indexBase;
        if (id < 0 || id >= bodyVertexCount) {
            ++out.rowsOutsideBody;
            continue;
        }
        merged.push_back({id, r.d});
    }
    std::sort(merged.begin(), merged.end(),
              [](const std::pair<long, D3>& a, const std::pair<long, D3>& b) { return a.first < b.first; });
    for (const auto& m : merged) {
        // A vertex listed twice is summed, which is what an additive morph does.
        if (!out.vertex.empty() && out.vertex.back() == static_cast<int>(m.first)) {
            out.delta.back() += m.second;
        } else {
            out.vertex.push_back(static_cast<int>(m.first));
            out.delta.push_back(m.second);
        }
    }
    return true;
}

IndexBaseProbe probeIndexBase(const std::string& path, const std::vector<D3>& objPositions) {
    IndexBaseProbe p;
    std::vector<RawRow> rows;
    std::vector<std::string> headers;
    long comments = 0, malformed = 0;
    std::string error;
    readRawRows(path, rows, headers, comments, malformed, error);
    for (int base = 0; base <= 1; ++base) {
        D3 lo{1e30, 1e30, 1e30}, hi{-1e30, -1e30, -1e30};
        long outOfRange = 0;
        for (const RawRow& r : rows) {
            const long id = r.index - base;
            if (id < 0 || id >= static_cast<long>(objPositions.size())) {
                ++outOfRange;
                continue;
            }
            const D3& v = objPositions[static_cast<size_t>(id)];
            lo.x = std::min(lo.x, v.x); lo.y = std::min(lo.y, v.y); lo.z = std::min(lo.z, v.z);
            hi.x = std::max(hi.x, v.x); hi.y = std::max(hi.y, v.y); hi.z = std::max(hi.z, v.z);
        }
        const double spread = (hi - lo).length();
        if (base == 0) { p.spreadBase0 = spread; p.outOfRangeBase0 = outOfRange; }
        else { p.spreadBase1 = spread; p.outOfRangeBase1 = outOfRange; }
    }
    return p;
}

char dominantAxis(const ShapeTarget& t) {
    double ax = 0, ay = 0, az = 0;
    for (const D3& d : t.delta) { ax += std::fabs(d.x); ay += std::fabs(d.y); az += std::fabs(d.z); }
    if (ax >= ay && ax >= az) return 'x';
    if (ay >= az) return 'y';
    return 'z';
}

void applyTargets(const std::vector<D3>& base,
                  const std::vector<std::pair<const ShapeTarget*, double>>& weighted,
                  std::vector<D3>& out) {
    out = base;
    for (const auto& wt : weighted) {
        if (!wt.first || wt.second == 0.0) continue;
        const ShapeTarget& t = *wt.first;
        for (size_t i = 0; i < t.vertex.size(); ++i)
            out[static_cast<size_t>(t.vertex[i])] += t.delta[i] * wt.second;
    }
}

DisplacementStats displacementStats(const std::vector<D3>& base, const std::vector<D3>& deformed) {
    DisplacementStats s;
    for (size_t i = 0; i < base.size() && i < deformed.size(); ++i) {
        const double len = (deformed[i] - base[i]).length();
        if (len <= 1e-9) continue;
        ++s.movedVertices;
        s.l1 += len;
        s.maxDisp = std::max(s.maxDisp, len);
    }
    s.meanDisp = s.movedVertices ? s.l1 / static_cast<double>(s.movedVertices) : 0.0;
    return s;
}

} // namespace pae::ht
