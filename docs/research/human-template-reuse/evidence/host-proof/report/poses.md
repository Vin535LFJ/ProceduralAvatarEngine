# LBS poses (generated)

Host linear blend skinning on the base body. Rotation axes are given in the rest world frame (+Y up, +Z forward, +X = character's left). Columns: folded faces (normal flip vs rest), 99th-percentile edge stretch/squash, worst edge ratio, volume ratio, skinning time, displacement of the probe joint. The core rig has no finger or toe bones, so its finger/ankle rows are effectively a bind pose and only prove that the remap is well-formed.

| Pose | Weight set | Folded faces | Edge p99 | Edge min / max | Volume ratio | Skinning | Probe joint displacement (x, y, z) |
|---|---|---:|---:|---|---:|---:|---|
| bind | MakeHuman default_weights.mhw (license PENDING) + deform rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.3 ms | +0.000 / +0.000 / +0.000 |
| bind | PAE distance weights + deform rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.3 ms | +0.000 / +0.000 / +0.000 |
| bind | PAE heat-diffusion weights + deform rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.2 ms | +0.000 / +0.000 / +0.000 |
| bind | MakeHuman weights remapped to parents + core rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.2 ms | +0.000 / +0.000 / +0.000 |
| bind | PAE heat weights remapped to parents + core rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.2 ms | +0.000 / +0.000 / +0.000 |
| shoulder-abduction-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 0 | 1.112 | 0.443 / 3.186 | 0.9975 | 0.3 ms | +0.085 / +0.405 / +0.000 |
| shoulder-abduction-L | PAE distance weights + deform rig | 1 | 1.189 | 0.518 / 10.231 | 1.0226 | 0.3 ms | +0.085 / +0.405 / +0.000 |
| shoulder-abduction-L | PAE heat-diffusion weights + deform rig | 15 | 1.005 | 0.232 / 5.697 | 1.0018 | 0.3 ms | +0.085 / +0.405 / +0.000 |
| shoulder-abduction-L | MakeHuman weights remapped to parents + core rig | 0 | 1.112 | 0.443 / 3.186 | 0.9975 | 0.2 ms | +0.085 / +0.405 / -0.000 |
| shoulder-abduction-L | PAE heat weights remapped to parents + core rig | 15 | 1.005 | 0.232 / 5.697 | 1.0018 | 0.2 ms | +0.085 / +0.405 / -0.000 |
| elbow-flexion-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 12 | 1.016 | 0.276 / 2.208 | 0.9985 | 0.2 ms | +0.000 / +0.240 / -0.048 |
| elbow-flexion-L | PAE distance weights + deform rig | 10 | 1.081 | 0.596 / 1.507 | 0.9976 | 0.3 ms | +0.000 / +0.240 / -0.048 |
| elbow-flexion-L | PAE heat-diffusion weights + deform rig | 15 | 1.002 | 0.591 / 1.638 | 0.9985 | 0.2 ms | +0.000 / +0.240 / -0.048 |
| elbow-flexion-L | MakeHuman weights remapped to parents + core rig | 12 | 1.016 | 0.276 / 2.208 | 0.9985 | 0.2 ms | +0.000 / +0.240 / -0.048 |
| elbow-flexion-L | PAE heat weights remapped to parents + core rig | 15 | 1.002 | 0.591 / 1.638 | 0.9985 | 0.2 ms | +0.000 / +0.240 / -0.048 |
| hip-flexion-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 1 | 1.101 | 0.307 / 2.038 | 0.9885 | 0.3 ms | +0.000 / +0.218 / +0.351 |
| hip-flexion-L | PAE distance weights + deform rig | 0 | 1.131 | 0.413 / 3.630 | 0.9808 | 0.2 ms | +0.000 / +0.218 / +0.351 |
| hip-flexion-L | PAE heat-diffusion weights + deform rig | 19 | 1.009 | 0.265 / 4.193 | 0.9905 | 0.2 ms | +0.000 / +0.218 / +0.351 |
| hip-flexion-L | MakeHuman weights remapped to parents + core rig | 1 | 1.101 | 0.307 / 2.038 | 0.9885 | 0.2 ms | +0.000 / +0.218 / +0.351 |
| hip-flexion-L | PAE heat weights remapped to parents + core rig | 19 | 1.009 | 0.265 / 4.193 | 0.9905 | 0.2 ms | +0.000 / +0.218 / +0.351 |
| knee-flexion-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 6 | 1.029 | 0.207 / 1.579 | 0.9961 | 0.2 ms | +0.000 / +0.232 / -0.307 |
| knee-flexion-L | PAE distance weights + deform rig | 5 | 1.108 | 0.610 / 1.584 | 0.9903 | 0.2 ms | +0.000 / +0.232 / -0.307 |
| knee-flexion-L | PAE heat-diffusion weights + deform rig | 16 | 1.001 | 0.170 / 1.994 | 0.9912 | 0.2 ms | +0.000 / +0.232 / -0.307 |
| knee-flexion-L | MakeHuman weights remapped to parents + core rig | 6 | 1.029 | 0.207 / 1.579 | 0.9961 | 0.2 ms | +0.000 / +0.232 / -0.307 |
| knee-flexion-L | PAE heat weights remapped to parents + core rig | 16 | 1.001 | 0.170 / 1.994 | 0.9912 | 0.2 ms | +0.000 / +0.232 / -0.307 |
| wrist-twist-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 0 | 1.009 | 0.475 / 2.482 | 0.9998 | 0.2 ms | -0.024 / +0.051 / +0.034 |
| wrist-twist-L | PAE distance weights + deform rig | 0 | 1.014 | 0.552 / 1.739 | 0.9997 | 0.2 ms | -0.024 / +0.051 / +0.034 |
| wrist-twist-L | PAE heat-diffusion weights + deform rig | 0 | 1.001 | 0.239 / 2.865 | 0.9999 | 0.2 ms | -0.024 / +0.051 / +0.034 |
| wrist-twist-L | MakeHuman weights remapped to parents + core rig | 0 | 1.009 | 0.475 / 2.482 | 0.9998 | 0.2 ms | probe bone absent |
| wrist-twist-L | PAE heat weights remapped to parents + core rig | 0 | 1.001 | 0.239 / 2.865 | 0.9999 | 0.2 ms | probe bone absent |
| ankle-plantarflexion-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 0 | 1.014 | 0.498 / 1.264 | 1.0000 | 0.2 ms | +0.000 / -0.060 / -0.035 |
| ankle-plantarflexion-L | PAE distance weights + deform rig | 0 | 1.028 | 0.729 / 1.439 | 0.9987 | 0.2 ms | +0.000 / -0.060 / -0.035 |
| ankle-plantarflexion-L | PAE heat-diffusion weights + deform rig | 0 | 1.001 | 0.410 / 1.697 | 0.9994 | 0.2 ms | +0.000 / -0.060 / -0.035 |
| ankle-plantarflexion-L | MakeHuman weights remapped to parents + core rig | 0 | 1.014 | 0.498 / 1.264 | 1.0000 | 0.2 ms | probe bone absent |
| ankle-plantarflexion-L | PAE heat weights remapped to parents + core rig | 0 | 1.001 | 0.410 / 1.697 | 0.9994 | 0.2 ms | probe bone absent |
| finger-curl-L | MakeHuman default_weights.mhw (license PENDING) + deform rig | 714 | 1.190 | 0.212 / 3.503 | 0.9996 | 0.2 ms | -0.010 / +0.001 / +0.003 |
| finger-curl-L | PAE distance weights + deform rig | 603 | 1.204 | 0.160 / 3.656 | 0.9995 | 0.2 ms | -0.010 / +0.001 / +0.003 |
| finger-curl-L | PAE heat-diffusion weights + deform rig | 696 | 1.160 | 0.304 / 3.275 | 0.9998 | 0.2 ms | -0.010 / +0.001 / +0.003 |
| finger-curl-L | MakeHuman weights remapped to parents + core rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.2 ms | probe bone absent |
| finger-curl-L | PAE heat weights remapped to parents + core rig | 0 | 1.000 | 1.000 / 1.000 | 1.0000 | 0.2 ms | probe bone absent |

Not measured: self-intersection (shoulder/axilla, hip/groin, knee), dual-quaternion skinning, pose correctives, twist propagation between twist bones. Joint-centre displacement is a kinematic check, not a visual one.
