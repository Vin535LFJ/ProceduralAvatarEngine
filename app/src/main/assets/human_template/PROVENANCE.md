# Distributed human template asset (VR-1B WP1/WP2)

- File: pae-human-base-mh-a8bc2d54.paht
- Format: PAHT v3 (payload SHA-256 header; no conversion timestamp — D1 fix)
- SHA-256: d3bcbbece3db11c0df79cb7202fdf1730b325108509d60ad9faa209a8a5e417b
- Bytes: 4054264
- Source: MakeHuman official assets @ commit a8bc2d54ff0ac92e78ff71431b1023eda42bf482
  - Base mesh + 18 shape targets: CC0-1.0 (LICENSE.md section C + per-file headers)
  - Rig: PAE HumanRigV2 core, 20 bones (PAE-generated; joint centres from CC0 joint-helper geometry)
  - Weights: PAE distance weights + Top-4 (PAE-generated; no third-party weight data)
  - default.mhskel / default_weights.mhw: host-only references, PENDING_CONFIRMATION, NOT distributed
- Compiler: tools/human-template-poc/host pae_ht_compiler (wp1_compiler_main.cpp)
- Policy: ADR-014 (ACCEPTED 2026-11-27); docs/phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md
- Reproducibility: two compiler runs on the pinned inputs produced byte-identical files (verified 2026-11-27).
- Known limitation: PAE distance weights show shoulder-abduction stretch in host posing tests;
  WP2 is STATIC rendering (identity palette) and does not exercise weights. Animation quality is a later WP.