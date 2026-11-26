# Skin weights (generated)

| Set | Bones | Mean influences (after Top-4) | Mass discarded by Top-4 | Unweighted verts | Generation | Dominant-bone agreement with MakeHuman set | Mean L1 vs MakeHuman set |
|---|---:|---:|---:|---:|---|---:|---:|
| MakeHuman default_weights.mhw (license PENDING) + deform rig | 102 | 1.574 | 0.575 % | 0 | 68.5 ms | 100.0 % | 0.000 |
| PAE distance weights + deform rig | 102 | 2.414 | 12.872 % | 0 | 40.6 ms | 63.3 % | 0.886 |
| PAE heat-diffusion weights + deform rig | 102 | 1.871 | 0.004 % | 0 | 564.4 ms | 60.1 % | 0.831 |
| MakeHuman weights remapped to parents + core rig | 20 | 1.574 | 0.000 % | 0 | 68.5 ms | 52.2 % | 0.948 |
| PAE heat weights remapped to parents + core rig | 20 | 1.871 | 0.000 % | 0 | 564.4 ms | 30.9 % | 1.373 |

MakeHuman set: 57107 entries, index base detected as 0, 9464 entries belong to facial bones that are not in the deform rig (dropped), mean influences before Top-4 2.487.

PAE generators (host, no external code): **distance** = inverse-square distance to bone segments, cut at one local bone radius (radius = median plain distance of the vertices closest to that bone), two Laplacian smoothing passes on the mesh graph, normalise, Top-4. **heat** = bone-heat diffusion `(I + tL)h = e` on the uniform mesh graph (t = 8, 60 Jacobi iterations, uniform edge weights, NOT cotangent), per-bone max normalisation, power 4, normalise, Top-4. Neither method is a bounded-biharmonic solve; that comparison is NOT RUN.

**Weight fallback (MakeHuman set, deterministic, host).** 175 body vertices end up without any influence because every one of their source weights sits on a facial bone that the deform rig does not keep. Rule `kept-ancestor`: each dropped bone is replaced by its nearest kept ancestor in the source hierarchy (all facial bones resolve to `head` here) and the masses are merged, Top-4, renormalised. Rule `nearest-bone`: a vertex with no usable source entry is bound to the nearest bone segment in the rest pose. Result: body 175 kept-ancestor + 0 nearest-bone, auxiliary ids 648 + 0, 0 ids still unweighted; vertices that already had a kept source weight are not touched. The per-vertex rule is stored in the template (format 2). NOT changed: body vertices that carry both a kept and a facial-bone influence keep only the kept part, renormalised (the facial part is discarded).
