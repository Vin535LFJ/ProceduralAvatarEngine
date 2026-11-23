# G2 — Fair Comparison / PoC-A acceptance report

**Engineering status:** evidence collected, reproducible, all host tests green.
**GATE-03 status:** **NOT CLOSED** — owner review and threshold approval are
outstanding. No route is approved and no visual result is marked ACCEPTED.

## 1. Scope and boundary

- Routes compared: R1 (SDF + Surface Nets, reference), R2 (fixed quad template
  projected on the anatomy SDF), R3 (fixed quad template on a cubic B-spline
  envelope). All three were `FEASIBLE` at G1, so G2 has ≥2 feasible routes.
- Topology scope: T1 torso canonical topology + T2 shoulder-root boundary.
  T3–T5 remain deferred (SPEC §10.1).
- Parameter groups: Baseline, Chest Width-Depth, Waist-Pelvis Ratio (SPEC §8.4).
- Every route uses the same Shape Model, landmark skeleton, target reference,
  sampling grid (52 rings × 104 segments), distance metric, coordinate system
  and camera. The earlier DEF-02 skeleton mismatch and arm-contaminated
  reference were fixed at G1 and guarded by `routes_share_one_skeleton`.
- Worktree isolation held: detached worktree at `99941a9`, no commit, no push,
  original workspace not written. ABI/frozen contracts unchanged.

## 2. Evidence inventory

| Artifact | Content |
|---|---|
| `evidence/g2/fair-comparison.md` | Full Layer A / Layer B tables, 3 groups × 3 routes |
| `evidence/g2/clay/index.md` | 63 grey-clay PNGs (3 groups × 3 routes × 7 views) |
| `evidence/g2/smooth-union-sign-derivation.md` | Derivation + dual-implementation test record |
| `tests/unit/poc_fair_comparison_probe.cpp` | Reproducible probe; writes both reports |
| `tests/unit/poc_routes_tests.cpp` | Route/topology regression suite |
| `tests/unit/smooth_union_sign_tests.cpp` | Smooth-union sign unification test |
| Host CTest | **11/11 passing** (10 original + 1 new) |

Reproduce from the worktree:

```
cmake --build build-host --parallel 4
ctest --test-dir build-host --output-on-failure
./build-host/pae_tests/poc_fair_comparison_probe.exe evidence/g2
```

## 3. The decision-critical result: error attribution

Baseline group, metres:

| Route | E1 mean | E2 mean | E2 max | E3 mean |
|---|---|---|---|---|
| R1 | 0.00440 | 0.00098 | 0.02310 | 0.00149 |
| R2 | 0.00440 | 0.00098 | 0.02310 | 0.00149 |
| R3 | 0.03272 | **0.00040** | 0.00100 | 0.03211 |

Interpretation (SPEC §8.2 — the layers must not be mixed):

- **E1** is a *reference-fidelity* number, not a route-quality number. R1 and
  R2 share the same continuous SDF, so their identical E1 measures the coarse
  reference mesh against the field it came from.
- **E2** isolates the cost of fixed-topology projection. R3 is the *best*
  projector of the three (0.40 mm vs R2's 0.98 mm). Its topology projection is
  not the problem.
- **E3** is the shape error of the mesh that would ship. R2 is **21.5× better**
  than R3 (1.49 mm vs 32.11 mm). R3's error is in its **envelope surface**, not
  in retopology. This attribution is only possible because E1/E2/E3 are
  recorded separately.
- R2 inherits R1's geometry and pays ~1 mm for the fixed quad template.

This conclusion is the *reverse direction* of the pre-DEF-02 result (R2/R3
ratio was 1.23× when the two routes sat on different skeletons). The earlier
comparison was measuring a skeleton offset, not representation fidelity.

## 4. Layer A findings

- **Landmark error (§13 #5):** R1/R2 are identically 0 — they share the same
  continuous field and grid, so this is a self-comparison and must NOT be read
  as a quality score. R3: mean 54.4 mm, p95 115.0 mm, max 126.8 mm (Baseline).
- **Normal deviation (§13 #7):** R1/R2 reference surfaces differ from the
  reference by 0.01° (self-comparison). R3 reference: mean 25.6°, p95 58.2°.
  Shipped mesh: R1/R2 mean 9.2°, p95 32.6°; R3 mean 27.1°, p95 67.2°.
  All flipped normals are confined to grid rings 0–1 (y ≈ 0.844–0.852 m),
  where the pelvis fan cap meets the side wall; rings 2–51 have none.
- **Section contour deviation (§13 #1):** R1/R2 0 (self-comparison). R3:
  |deviation| mean 26.8 mm, p95 102.2 mm, max 136.8 mm.
- **7-view silhouette (§13 #1):** recorded for every route/group against R1,
  with band clipping so no route is charged for anatomy it never produced.
- **Grey clay:** 63 images with a shared camera, light, band and per-view
  framing. Owner holds the independent visual veto.

## 5. Layer B findings

- **Topology:** every route has components=1, non-manifold=0, degenerate=0 and
  no unregistered boundary edges. R2/R3 have exactly one closed shoulder-root
  crop loop (32 edges); R1 has none because it is a closed SDF mesh.
- **Stable IDs / connectivity:** R2 and R3 keep vertex count 833, vertex IDs
  and face adjacency constant across all three parameter groups. R1 changes
  count 5108 → 5234 → 5314, so ID-based comparison is undefined.
- **Face quality:** R2 has 0 slivers in all groups (mean min angle 81.6–83.0°,
  orthogonal-quad ratio 0.897–0.935). R3 has 20–28 slivers; R1 has 22–36.
- **Seam smoothness (§13 #7, contract value ≤5°):** **FAIL for all routes.**
  Crop-ring max dihedral is 47.66° (R1/R2) and 16.13° (R3) in the Baseline
  group. This is a contract-existing threshold, not a TBD one, and is
  reported as a real defect rather than folded into a quality claim.
- **T2:** the shoulder root is one explicit closed crop ring at the chest
  landmark, with no separate shoulder island and no tube substitute. It is a
  *connection boundary*, not a full shoulder joint topology (T3 deferred).

## 6. Skinning contract (SPEC §11.1)

- Weight legality: **yes** for all routes/groups — ≤2 influences, no negative
  weights, sum within 1e-3, all bone IDs in [0,21].
- CPU deformation: 22/22 synthetic rigid-bone probes land exactly on the bone
  palette transform (max error 0); bind-pose residual ≤2.4e-7 m.
- **GPU numerical parity: `[UNVERIFIED]`** — explicitly not a G2 kill.
- The PoC weight assigner reproduces the production formula and is checked
  against production weights; no production generator change was made.

## 7. §13 threshold baselines (TBD values are NOT PASS)

| # | Metric | Measured G2 baseline | Threshold status |
|---|---|---|---|
| 1 | Thorax silhouette | p50/p95/max per route × 7 views × 3 groups in `fair-comparison.md` | **TBD** — awaiting owner + DEC-05 |
| 2 | Abdomen volume | 0.023452 m³ (R2), 0.026083 m³ (R3) vs 0.044832 m³ (R1), see caveat below | **TBD** |
| 3 | Waist compression | waist/chest width 0.4821 (Baseline), 0.4767, 0.4478 | **TBD** |
| 4 | Pelvis proportion | iliac width / pelvic depth 1.5977 (Baseline), 1.5977, 2.6687 | **TBD** |
| 5 | Landmark error | R3 mean 0.05443 m, p95 0.11503 m, max 0.12684 m; R1/R2 self-comparison | **TBD** |
| 6 | Reference surface distance | E1 table above, sample counts recorded (5408 / 5408 / 2066) | **TBD** |
| 7 | Normal deviation | shipped-mesh p50 3.32°, p95 32.57°, max 114.78° (R1/R2); 22.62°/67.15°/121.98° (R3) | ≤5° smooth seam **FAIL (contract)**; remainder TBD |
| 8 | Surface volume delta | volume table across 3 groups | **TBD** |
| 9 | Parameter region impact | R2 chest group: 33 stable vertices >1 mm; R3 waist group: 250 stable vertices >1 mm (max 14.86 mm) | **TBD** (1 mm is a measurement guard only) |
| 10 | Topology stability | R2/R3 IDs + adjacency constant, 833 verts; R1 5108→5234→5314 | components=1 / boundary=crop / nonmanifold=0 **CLEAR**; ratios TBD |

**Volume caveat:** R1 is a closed SDF mesh, while R2/R3 are open at the
shoulder-root crop and capped at the pelvis. Their signed volumes are over
different vertical extents, so the −47.69% delta is **not** a body-volume
error measurement. It is recorded as a baseline only; a like-for-like volume
comparison remains an open measurement gap.

## 8. G2 kill / stop checklist (SPEC §6.3)

| Trigger | Status |
|---|---|
| Visual P0 defect / owner veto | **NOT YET EVALUATED** — evidence delivered, owner decision pending |
| Topology instability (components≠1, unregistered boundary/non-manifold) | **CLEAR** |
| Hardcoded or assigned metrics / fake PASS | **CLEAR** — all values computed; no PASS on TBD |
| Parameter declared but never reaching vertices | **PARTIALLY VERIFIED** from G0/G1; G2 shows real mesh deltas for R2/R3; not upgraded to VERIFIED |
| Device UNVERIFIED reported as PASS | **CLEAR** — no device claims at G2 |
| Counts used as quality ranking | **CLEAR** — vertex/triangle counts listed as cost only |
| GPU parity `[UNVERIFIED]` | **NOT A KILL** per §11.1 — carried to G3/G4 |

## 9. Why GATE-03 is not closed

1. G2 entry criteria require §13 thresholds to be approved; only the ≤5°
   smooth-seam contract value exists. All other thresholds are TBD and need
   owner approval against DEC-05.
2. The owner visual Gate has not been exercised. The AI must not self-mark
   `VISUALLY ACCEPTED`.
3. A contract-value failure remains open: the shoulder-root seam is not
   smooth (§13 #7) on any route.
4. Decision-critical attribution is ready (R2 vs R3 differs by 21.5× in E3,
   and the error is in R3's surface rather than its projection), but G3
   requires a signed owner decision record; the AI does not choose a route.

## 10. Carry-forward to G3

- Decision record must include the GPU-parity risk and a G4 completion plan.
- Open measurement gaps: like-for-like volume over a common band; owner
  approval of all TBD thresholds; owner visual verdict on the 63 clay images.
- Open defects: crop-ring seam smoothness; R3 surface error; cross-region
  parameter influence; pelvis-cap normal flips.
- Production source changed once at G2: `SdfFusion.cpp` smooth-union / smooth-
  subtraction signs, derived and dual-implementation tested. APK hash changed
  from `088476fc...` to `c1cace9d...`; both functions have no production
  caller, and on-device re-verification is **NOT RUN**.
