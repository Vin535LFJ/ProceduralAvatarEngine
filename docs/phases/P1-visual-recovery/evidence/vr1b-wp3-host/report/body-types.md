# WP3 body types (host study, generated)

alpha solved per parameter from the calibrated asymmetric slopes, clamped to the authored [-1, +1] range (clamped requests are flagged). Proxy parameters (armShape/legShape) use direct authored-range alpha. stature is the standalone macro height target: anthropometric correctness UNVERIFIED by MakeHuman; quality columns show whether the combination stays clean.

| Body type | alpha per parameter | Stature req/ach (m) | Bideltoid req/ach (m) | Waist req/ach (m) | Hip req/ach (m) | Chest W x D ach (m) | Flips | Edge p99 | Volume ratio |
|---|---|---|---|---|---|---|---:|---:|---:|
| base | (none) | - / 1.6659 | - / 0.5595 | - / 0.2971 | - / 0.3699 | 0.2757 x 0.2005 | 0 | 1.000 | 1.0000 |
| tall-athletic | stature+0.11 shoulderWidth+1.00 waistWidth-1.00 armShape+0.50 legShape+0.50  [shoulderWidth:clamp+ waistWidth:clamp- ] | +0.080 / +0.080 | +0.060 / +0.081 (!) | -0.020 / -0.008 (!) | - / 0.3885 | 0.2985 x 0.2044 | 0 | 1.214 | 1.1092 |
| petite | stature-0.27 shoulderWidth-1.00 hipWidth+0.17  [shoulderWidth:clamp- ] | -0.100 / -0.100 | -0.040 / -0.052 (!) | - / 0.2885 | +0.010 / -0.002 (!) | 0.2596 x 0.1958 | 0 | 1.140 | 0.8848 |
| heavy-set | waistWidth+1.00 hipWidth+1.00 ribcageWidth+0.73 chestDepth+0.72 armShape+0.40 legShape+0.40  [waistWidth:clamp+ hipWidth:clamp+ ] | - / 1.6659 | - / 0.6130 | +0.080 / +0.023 (!) | +0.060 / +0.065 | 0.3157 x 0.2418 | 0 | 1.215 | 1.2586 |
| v-taper | shoulderWidth+1.00 waistWidth-1.00 hipWidth-0.68 ribcageWidth+0.36  [shoulderWidth:clamp+ waistWidth:clamp- ] | - / 1.6659 | +0.080 / +0.075 (!) | -0.050 / -0.019 (!) | -0.020 / -0.020 | 0.3090 x 0.1996 | 0 | 1.211 | 0.9633 |
