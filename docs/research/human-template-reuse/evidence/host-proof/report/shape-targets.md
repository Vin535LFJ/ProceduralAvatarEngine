# Shape target application (generated)

Formula: `V = V0 + sum(alpha_i * dV_i)`, applied to the WHOLE OBJ (body + helper + joint vertices), because the targets also encode helper motion and the rig's joint centres are means of joint-cube vertices. All values in metres on the pinned `base.obj` body.

Measurement planes are PAE's own definition for this proof (not MakeHuman's): bideltoid = shoulder-joint height - 2.5 % stature over shoulder-girdle + upper-arm vertices; chest = 72 % stature over thorax vertices; waist = 60 % stature over abdomen vertices; hip = hip-joint height over pelvis + thigh vertices; band half-width 1.5 % stature. Region labels are heuristic (nearest bone with radius normalisation; trunk zones by height).

| Parameter | Target files (+ / -) | Rows | Mean displacement per region at +1.0 (mm, top 5) | Share of |displacement| on helper geometry | Named measure: base / +1 / -1 | Delta (+ / -) | Stature delta (+ / -) | Folded faces (+ / -) | Edge stretch p99 (+ / -) | Volume ratio (+ / -) |
|---|---|---:|---|---:|---|---|---|---|---|---|
| shoulderWidth (`bideltoid width`) | measure-shoulder-dist-incr.target / measure-shoulder-dist-decr.target | 5568 | hand 24.9, forearm 24.9, upper-arm 24.8, shoulder-girdle 11.9, thorax 1.7 | 17.6 % | bideltoid width: 0.5595 / 0.6093 / 0.5325 | +0.0498 / -0.0270 | +0.0000 / +0.0000 | 0 / 0 | 1.122 / 1.064 | 1.0170 / 0.9893 |
| ribcageWidth (`chest width`) | torso-scale-horiz-incr.target / torso-scale-horiz-decr.target | 7196 | forearm 34.2, hand 34.2, upper-arm 34.0, shoulder-girdle 21.8, thorax 13.9 | 20.1 % | chest width: 0.2757 / 0.3307 / 0.2207 | +0.0550 / -0.0550 | +0.0000 / +0.0000 | 0 / 0 | 1.193 / 1.238 | 1.0522 / 0.9340 |
| chestDepth (`chest depth`) | torso-scale-depth-incr.target / torso-scale-depth-decr.target | 2651 | thorax 20.8, abdomen 11.6, shoulder-girdle 6.5, upper-arm 1.5, pelvis 0.1 | 42.7 % | chest depth: 0.2005 / 0.2561 / 0.1703 | +0.0557 / -0.0302 | +0.0000 / +0.0000 | 0 / 0 | 1.181 / 1.125 | 1.0782 / 0.9520 |
| waistWidth (`waist width`) | measure-waist-circ-incr.target / measure-waist-circ-decr.target | 1518 | abdomen 17.7, pelvis 3.2, thorax 1.3 | 43.9 % | waist width: 0.2971 / 0.3139 / 0.2803 | +0.0168 / -0.0168 | +0.0000 / +0.0000 | 0 / 0 | 1.085 / 1.095 | 1.0489 / 0.9548 |
| hipWidth (`hip width`) | measure-hips-circ-incr.target / measure-hips-circ-decr.target | 1418 | pelvis 18.6, thigh 1.3, abdomen 0.4 | 51.7 % | hip width: 0.3699 / 0.4289 / 0.3405 | +0.0590 / -0.0294 | +0.0000 / +0.0000 | 0 / 0 | 1.153 / 1.081 | 1.0870 / 0.9611 |
| armShape (`no named measure`) | l-upperarm-muscle-incr.target + r-upperarm-muscle-incr.target / l-upperarm-muscle-decr.target + r-upperarm-muscle-decr.target | 638 | upper-arm 3.4, shoulder-girdle 0.2, thorax 0.0 | 36.5 % | - | - | +0.0000 / +0.0000 | 0 / 0 | 1.085 / 1.053 | 1.0082 / 0.9961 |
| legShape (`no named measure`) | l-upperleg-muscle-incr.target + r-upperleg-muscle-incr.target / l-upperleg-muscle-decr.target + r-upperleg-muscle-decr.target | 948 | thigh 3.5, pelvis 1.5 | 47.4 % | - | - | +0.0000 / +0.0000 | 0 / 0 | 1.055 / 1.063 | 1.0353 / 0.9705 |
| headShape (`no named measure`) | head-scale-horiz-incr.target / head-scale-horiz-decr.target | 5240 | head 9.1, neck 3.7, thorax 0.1 | 15.2 % | - | - | +0.0000 / +0.0000 | 0 / 0 | 1.215 / 1.271 | 1.0122 / 0.9877 |
| stature (`stature`) | male-young-averagemuscle-averageweight-maxheight.target / male-young-averagemuscle-averageweight-minheight.target | 19150 | head 687.9, neck 669.8, shoulder-girdle 628.9, thorax 600.8, upper-arm 563.2 | 28.0 % | stature: 1.6659 / 2.3884 / 1.2995 | +0.7225 / -0.3664 | +0.7225 / -0.3664 | 0 / 0 | 1.876 / 1.687 | 2.3986 / 0.6132 |

Combined (the eight local parameters at +1.0): stature 1.6659, bideltoid 0.6872, chest 0.3440 x 0.2574, waist 0.3203, hip 0.4433; folded faces 0, edge stretch p99 1.409, volume ratio 1.3632.  

Overshoot (the same eight at +1.5, outside the authored 0..1 range): bideltoid 0.7523, chest 0.3783 x 0.2863, waist 0.3319, hip 0.4800; folded faces 0, edge stretch p99 1.618, volume ratio 1.5704.  

Not verified: self-intersection after targets, parameter ranges as authored by MakeHuman (sliders), targets beyond +/-1.0, macro combinations (gender / age / race / muscle / weight), faces and expressions.
