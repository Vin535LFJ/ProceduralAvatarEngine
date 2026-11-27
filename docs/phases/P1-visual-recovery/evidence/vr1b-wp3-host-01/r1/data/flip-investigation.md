# WP3-H-R1 shorter-pae quality investigation (generated)

Base mesh SHA-256: `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`.

## CSV column clarification

`variants-quality.csv` column order is: name,vertices,triangles,components,boundary_edges,nonmanifold,**degenerate**,dup_vertices,invalid_normals,self_intersecting_pairs,candidate_pairs,**flipped_faces**,edge_ratio_p99,...

The `2` in the shorter-pae row is in the **degenerate** column (degenerateTriangles), NOT flipped_faces. flipped_faces is **0** for baseline, taller-pae AND shorter-pae. The 18 report (flips=0) and height-study (uniform 1.55 flips=0) are therefore consistent with the CSV; there is no flipped-face contradiction.

## Re-run from code (shorter-pae = uniform scale to 1.55 m)

- compareSurfaces flipped_faces = **0**, minNormalDot = 1.000000 (a uniform scale with k>0 preserves every face orientation; Newell normal direction is scale-invariant).
- volumeRatio = 0.805483, edgeRatioP99 = 1.0748.

## Degenerate triangles at 1.55 m: 2 (this is the `2` in the CSV)

Root cause: `pae::analyzeTopology` (native/procedural/MeshTopology.cpp:45, read-only reuse) flags a triangle degenerate when its **float** cross-product lengthSq < **1e-14f** — an ABSOLUTE threshold (area < ~5e-8 m^2). Uniform downscale to 1.55 m (k=0.9304) shrinks every area by k^2=0.8656; the two smallest base-mesh triangles fall below that absolute threshold after float conversion. This is a scale-dependent threshold artifact of the checker, NOT a geometry defect and NOT a flipped face. It is outside the WP3-H-R1 whitelist (native/), so it is documented, not modified.

| quad face ID | tri | vertex ids | double area (m^2) | float crossLenSq | rest.deformed normal dot |
|---:|---:|---|---:|---:|---:|
| 320 | 0 | 428,500,501 | 4.763e-08 | 9.073e-15 | 1.000000 |
| 6947 | 1 | 7188,7250,7249 | 4.763e-08 | 9.073e-15 | 1.000000 |

Before/after area of the same triangles (double precision):

| quad face | tri | base area (m^2) | 1.55 m area (m^2) | ratio |
|---:|---:|---:|---:|---:|
| 320 | 0 | 5.501e-08 | 4.763e-08 | 0.8657 |
| 6947 | 1 | 5.501e-08 | 4.763e-08 | 0.8657 |

## Conclusion

flipped_faces = 0 (uniform scale cannot flip a face). The reported `2` is degenerateTriangles, a scale-dependent absolute-threshold artifact in PAE's own checker. Recommendation for WP3-Production: use a scale-relative degenerate threshold (or evaluate at native scale) so the census is stature-invariant. CSV and Markdown are unified: flips=0, degenerate=2 at 1.55 m.
