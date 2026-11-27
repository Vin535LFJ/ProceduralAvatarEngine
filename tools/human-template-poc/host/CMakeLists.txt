cmake_minimum_required(VERSION 3.16)
project(pae_human_template_proof CXX)

# P1-VR-ARCH-1 host-only prototype (chain A). It lives in tools/human-template-poc
# and is NOT referenced by the Android build: it must not touch the Android
# production generator, LegacyRigV1 or the canonical/RenderMesh path.
#
# PAE_REPO points at the engine checkout this folder belongs to (build.ps1
# defaults to the repository root). Only three host evidence helpers are
# compiled from it (ImageWriter, CameraProtocol, ClayRenderer) so the proof uses
# the engine's own PNG writer and fixed seven-view camera protocol. Nothing in
# the engine's native/ tree is modified.

set(PAE_REPO "" CACHE PATH "Path to a ProceduralAvatarEngine checkout (read-only)")
option(PAE_HT_BUILD_TESTS "Build the assert-based host tests" ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT MSVC)
  add_compile_options(-O2 -Wall -Wextra)
endif()

set(PAE_HT_SOURCES
  src/ht_common.cpp
  src/ht_json.cpp
  src/ht_obj.cpp
  src/ht_topology.cpp
  src/ht_target.cpp
  src/ht_measure.cpp
  src/ht_proof.cpp
  src/ht_rig.cpp
  src/ht_template.cpp
  src/ht_render.cpp
)

add_library(pae_ht STATIC ${PAE_HT_SOURCES})
target_include_directories(pae_ht PUBLIC src)

add_executable(pae_ht_pipeline src/pipeline_main.cpp)
target_link_libraries(pae_ht_pipeline PRIVATE pae_ht)

if(PAE_HT_BUILD_TESTS)
  add_executable(pae_ht_tests src/tests_main.cpp)
  target_link_libraries(pae_ht_tests PRIVATE pae_ht)
  enable_testing()
  add_test(NAME pae_ht_tests COMMAND pae_ht_tests)
endif()

# PAE evidence helpers (read-only use of the engine's host code).
if(PAE_REPO)
  set(PAE_NATIVE "${PAE_REPO}/native")
  add_library(pae_evidence STATIC
    "${PAE_NATIVE}/procedural/shape/ImageWriter.cpp"
    "${PAE_NATIVE}/procedural/shape/CameraProtocol.cpp"
    "${PAE_NATIVE}/procedural/shape/ClayRenderer.cpp"
  )
  target_include_directories(pae_evidence PUBLIC "${PAE_NATIVE}")
  target_compile_definitions(pae_evidence PUBLIC PAE_HT_WITH_PAE_EVIDENCE=1)
  target_link_libraries(pae_ht PUBLIC pae_evidence)

  # WP1 compiler: emits the productized PAHT v3 container. The v3 format module
  # is compiled directly from native/ (same read-only pattern as pae_evidence);
  # it is also part of pae_core for the Android loader and host unit tests.
  add_executable(pae_ht_compiler
    src/wp1_compiler_main.cpp
    "${PAE_NATIVE}/procedural/human_template/PAEHumanTemplateFormat.cpp"
  )
  target_include_directories(pae_ht_compiler PRIVATE "${PAE_NATIVE}")
  target_link_libraries(pae_ht_compiler PRIVATE pae_ht)

  # WP3 host-only shape study: parameter calibration + named body types.
  add_executable(pae_wp3_shape_study
    src/wp3_shape_study_main.cpp
    "${PAE_NATIVE}/procedural/human_template/PAEHumanTemplateFormat.cpp"
    "${PAE_NATIVE}/procedural/human_template/HumanTemplateLoader.cpp"
  )
  target_include_directories(pae_wp3_shape_study PRIVATE "${PAE_NATIVE}")
  target_link_libraries(pae_wp3_shape_study PRIVATE pae_ht)

  # VR-1B-WP3-HOST-01: full parameterization experiment. Reuses PAE's own
  # topology / self-intersection checkers read-only (compiled directly, the
  # pae_evidence pattern); no native/ source is modified.
  set(PAE_WP3_CHECKER_SOURCES
    "${PAE_NATIVE}/procedural/MeshTopology.cpp"
    "${PAE_NATIVE}/procedural/humanoid/canonical/CanonicalTopology.cpp"
  )
  add_executable(pae_wp3_experiment
    src/wp3_experiment_main.cpp
    src/wp3_measure.cpp
    src/wp3_quality.cpp
    "${PAE_NATIVE}/procedural/human_template/PAEHumanTemplateFormat.cpp"
    "${PAE_NATIVE}/procedural/human_template/HumanTemplateLoader.cpp"
    ${PAE_WP3_CHECKER_SOURCES}
  )
  target_include_directories(pae_wp3_experiment PRIVATE "${PAE_NATIVE}")
  target_link_libraries(pae_wp3_experiment PRIVATE pae_ht)
  if(WIN32)
    target_link_libraries(pae_wp3_experiment PRIVATE psapi)
  endif()

  add_executable(pae_wp3_tests
    src/wp3_tests.cpp
    src/wp3_measure.cpp
  )
  target_include_directories(pae_wp3_tests PRIVATE "${PAE_NATIVE}")
  target_link_libraries(pae_wp3_tests PRIVATE pae_ht)
  add_test(NAME pae_wp3_tests COMMAND pae_wp3_tests)

  # WP4-HOST-RIG-01: PAE-owned Rig V2 + joint resolver + weights + CPU LBS.
  # Reuses PAE's topology / self-intersection checkers read-only (pae_evidence
  # pattern); no native/ source is modified, no Android path is touched.
  set(PAE_WP4_SOURCES
    src/wp4_rig.cpp
    src/wp4_skin.cpp
    src/wp4_weights.cpp
    src/wp4_poses.cpp
    src/wp4_quality.cpp
    src/wp3_quality.cpp
    ${PAE_WP3_CHECKER_SOURCES}
  )
  add_executable(pae_wp4_experiment
    src/wp4_main.cpp
    ${PAE_WP4_SOURCES}
  )
  target_include_directories(pae_wp4_experiment PRIVATE "${PAE_NATIVE}")
  target_link_libraries(pae_wp4_experiment PRIVATE pae_ht)

  add_executable(pae_wp4_tests
    src/wp4_tests.cpp
    src/wp4_rig.cpp
    src/wp4_skin.cpp
    src/wp4_weights.cpp
    src/wp4_quality.cpp
  )
  target_include_directories(pae_wp4_tests PRIVATE "${PAE_NATIVE}")
  target_link_libraries(pae_wp4_tests PRIVATE pae_ht)
  add_test(NAME pae_wp4_tests COMMAND pae_wp4_tests)
else()
  message(STATUS "PAE_REPO not set: PNG output and PAE clay cross-check are disabled")
endif()
