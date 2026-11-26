# Tooling manifest

Generated 2026-11-26 after the final in-repo run. The tools now live IN the repository under `tools/human-template-poc/` (host-only; not referenced by the Android build). SHA-256 is over the working-tree bytes at commit time.

Two independent evidence chains, both PAE C++17, neither containing MakeHuman/MPFB2 code:

- **Chain A** (`src/`, library `pae_ht` + `pae_ht_tests` + `pae_ht_pipeline`): own OBJ/target/rig/weights parsers, quad-preserving topology audit, Rig V2 (102 deform / 20 core), three weight sets, LBS poses, `.paht` template write/read. Reads three engine evidence helpers (ImageWriter, CameraProtocol, ClayRenderer) and modifies nothing.
- **Chain B** (`template_proof.cpp`, single file): links the engine host library `build-host/libpae_core.a` and uses its `Math`, `MeshData`, `MeshTopology` and `canonical::detectSelfIntersections`; exports OBJ / `weights.csv` / `base.paehuman` (`PAEHUM01`). Its 8 actions carry direction assertions (`DIRECTION_CHECK`). `render_proof.py` renders its OBJ files.

| File (repo-relative) | Bytes | SHA-256 |
|---|---:|---|
| `tools/human-template-poc/host/build.ps1` | 1442 | `03b31abb6ade35fd673573bf3c94764dae9e69d12b0be59590028f55fe30b99c` |
| `tools/human-template-poc/host/CMakeLists.txt` | 2270 | `4bc117a80d2b7bfba2081d15b2c1125e0c3075179e934cb99a74e9c332538b79` |
| `tools/human-template-poc/host/render_proof.py` | 7952 | `22781bc79dfe269daab83611df7e355abd674917694ba1012b2c011d91073a80` |
| `tools/human-template-poc/host/run_template_proof.ps1` | 4634 | `dd5d412846d99e2861604f91626ec853b09e8b0c351095a88a8555bd9d5d1cd1` |
| `tools/human-template-poc/host/src/ht_common.cpp` | 8557 | `6f545916c1c30d9de5766e8faed628eedf77091e058a87593b4cffed9bb807b3` |
| `tools/human-template-poc/host/src/ht_common.h` | 4275 | `ddb887ad3a42e8d909d980583a1e9cf57ec729c290e39eded16fbec7b8413c8d` |
| `tools/human-template-poc/host/src/ht_json.cpp` | 5780 | `95ecdc4353645267ac9dcda4c519868485ebddaeb8448a8033bfc4eb533910b5` |
| `tools/human-template-poc/host/src/ht_json.h` | 918 | `166259aee2e991b3124062ceffad9b9762c09449cca132a9727336b4c4e07b9a` |
| `tools/human-template-poc/host/src/ht_measure.cpp` | 5917 | `ef1145465aed86e58a607064d05f2e5e51ecd0af89d27cb73357c1d676859fa4` |
| `tools/human-template-poc/host/src/ht_measure.h` | 2129 | `6fc43c9da67167285e934bdfeb7c234467d0ff09eb68c080ff4cb380deb8ef4b` |
| `tools/human-template-poc/host/src/ht_obj.cpp` | 7212 | `d268638aeaa6fde9a911be8c1152f1f512bec892f58710a27d8b7ab413f010ea` |
| `tools/human-template-poc/host/src/ht_obj.h` | 1839 | `d3d259c80ae9b08ef39706c062285a5273ee60e98fa548f439ee36cff50fe0ef` |
| `tools/human-template-poc/host/src/ht_proof.cpp` | 12908 | `63116742082b921d2bddf690f4ab4b5fa59d9e87209151fe47ba110f1b4e1776` |
| `tools/human-template-poc/host/src/ht_proof.h` | 3565 | `bdba6f0a0d4f4d1803472d1fe333f172d52321c28350b9c66f36410eca9c10e1` |
| `tools/human-template-poc/host/src/ht_render.cpp` | 20116 | `8d018cd9e9dde8e9f7d75168cd68a500da51d9e3e5cfab0f9e0a9bdecebbd3d8` |
| `tools/human-template-poc/host/src/ht_render.h` | 2618 | `695b006b9d9b2d7f2c91c3408b94ee3cfb09b1371ac937b319a42852a51c3066` |
| `tools/human-template-poc/host/src/ht_rig.cpp` | 38993 | `8e03fb324d28fff907a4187fd950273b632119f066cec039d42dee6cc484c235` |
| `tools/human-template-poc/host/src/ht_rig.h` | 7391 | `5788eb113051ddd12633c58e26a91bb68e668342db9b211b5f44af21521a0bc0` |
| `tools/human-template-poc/host/src/ht_target.cpp` | 5410 | `369911916d1de15d67aee4f2d6913371443336ebe64b04fc2deb04de49a94466` |
| `tools/human-template-poc/host/src/ht_target.h` | 2091 | `0f8e43b8ab4beba3f7ccb2b23b186f01c26806c5f9f73f8bad2f678bafc1af5b` |
| `tools/human-template-poc/host/src/ht_template.cpp` | 13296 | `4aa2ca84478773282b92375899377d5db5a0a450ddbaa9595a1170eb9765d7e7` |
| `tools/human-template-poc/host/src/ht_template.h` | 2742 | `5e3b1e80f51faa88e23e8790a9d8ae0b8d2ba920b80454e65a0586f5f5f47595` |
| `tools/human-template-poc/host/src/ht_topology.cpp` | 15245 | `c8ebcfadd154fbb22410f0708775723160eedc7e22ab92b6165a58cf877e7f34` |
| `tools/human-template-poc/host/src/ht_topology.h` | 3340 | `d2f70afd2b105bef2fc5d01fd1d2a2ee83735ff8e8cfe3c498be970eaf0a0e51` |
| `tools/human-template-poc/host/src/pipeline_main.cpp` | 46199 | `024d956eced01dd2257522b19706070cbc8ff563919a53ebe9785b73dfb29b05` |
| `tools/human-template-poc/host/src/tests_main.cpp` | 41499 | `a1448d02905f9376dfc90d1ba06f57b9b77afb6a975791efce0c7b316f7a5138` |
| `tools/human-template-poc/host/template_proof.cpp` | 45552 | `3978457363dfc0944e228bbf7f314b7cafb9b253ec46ef748d39d51b185acbaf` |
| `tools/human-template-poc/host/tools/recompress_png.py` | 2867 | `9b2ebbe2a292df4af2835900135d1f8b4bcf43802649ff5260afedf72366bb68` |
| `tools/human-template-poc/host/tools/region_topology_facts.py` | 5210 | `8c3485e625073c41c96ea6112d4fbf41df5cd77b6dbc18404a7bd617b767c64f` |

Not committed (regenerated on every run, or third-party CC0 data that stays outside git): `build-host/human-template-poc/**` (executables, `.paht`, logs, images), `assets/makehuman-a8bc2d54/**` (pinned MakeHuman archive).