add_executable(pae_core_tests unit/core_tests.cpp)
target_include_directories(pae_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_core_tests PRIVATE pae_core)
add_test(NAME pae_core_tests COMMAND pae_core_tests)

add_executable(pae_geometry_tests unit/geometry_tests.cpp)
target_include_directories(pae_geometry_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_geometry_tests PRIVATE pae_core)
add_test(NAME pae_geometry_tests COMMAND pae_geometry_tests)

add_executable(pae_fusion_tests unit/fusion_tests.cpp)
target_include_directories(pae_fusion_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_fusion_tests PRIVATE pae_core)
add_test(NAME pae_fusion_tests COMMAND pae_fusion_tests)

# G2 (SPEC 6.2 note): smooth-union sign, derived and tested before any edit.
add_executable(pae_smooth_union_sign_tests unit/smooth_union_sign_tests.cpp)
target_include_directories(pae_smooth_union_sign_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_smooth_union_sign_tests PRIVATE pae_core)
add_test(NAME pae_smooth_union_sign_tests COMMAND pae_smooth_union_sign_tests)

add_executable(pae_face_hair_tests unit/face_hair_tests.cpp)
target_include_directories(pae_face_hair_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_face_hair_tests PRIVATE pae_core)
add_test(NAME pae_face_hair_tests COMMAND pae_face_hair_tests)

add_executable(pae_anatomy_tests unit/anatomy_tests.cpp)
target_include_directories(pae_anatomy_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_anatomy_tests PRIVATE pae_core)
add_test(NAME pae_anatomy_tests COMMAND pae_anatomy_tests)

add_executable(pae_canonical_skin_tests unit/p1_continuous_gray_body_tests.cpp)
target_include_directories(pae_canonical_skin_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_canonical_skin_tests PRIVATE pae_core)
add_test(NAME pae_canonical_skin_tests COMMAND pae_canonical_skin_tests)

add_executable(pae_render_data_tests unit/p1_render_data_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/renderer/gl/SkinnedGpuMesh.cpp)
target_include_directories(pae_render_data_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_render_data_tests PRIVATE pae_core)
add_test(NAME pae_render_data_tests COMMAND pae_render_data_tests)

add_executable(pae_humanoid_tests unit/humanoid_tests.cpp)
target_include_directories(pae_humanoid_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_humanoid_tests PRIVATE pae_core)
add_test(NAME pae_humanoid_tests COMMAND pae_humanoid_tests)

add_executable(pae_skinning_tests unit/skinning_tests.cpp)
target_include_directories(pae_skinning_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_skinning_tests PRIVATE pae_core)
add_test(NAME pae_skinning_tests COMMAND pae_skinning_tests)

add_executable(pae_motion_tests unit/motion_tests.cpp)
target_include_directories(pae_motion_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_motion_tests PRIVATE pae_core)
add_test(NAME pae_motion_tests COMMAND pae_motion_tests)

add_executable(pae_character_patch_tests unit/character_patch_tests.cpp)
target_include_directories(pae_character_patch_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_character_patch_tests PRIVATE pae_core)
add_test(NAME pae_character_patch_tests COMMAND pae_character_patch_tests)

add_executable(pae_proportion_probe unit/proportion_probe.cpp)
target_link_libraries(pae_proportion_probe PRIVATE pae_core)

# G1 (AUTH-07): PoC-A geometry routes R1/R2/R3 + the unified Shape Model
# parameter contract. These sources are compiled ONLY into the host PoC
# targets so the production Android build stays untouched.
set(PAE_POC_SHAPE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/ShapeModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/QuadTemplate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/PocRoutes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/TopologyAudit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/SurfaceMetrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/CameraProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/LayerAMetrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/SkinWeights.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/ImageWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/ClayRenderer.cpp
)

add_executable(pae_poc_route_tests unit/poc_routes_tests.cpp ${PAE_POC_SHAPE_SOURCES})
target_include_directories(pae_poc_route_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_route_tests PRIVATE pae_core)
add_test(NAME pae_poc_route_tests COMMAND pae_poc_route_tests)

add_executable(poc_route_probe unit/poc_route_probe.cpp ${PAE_POC_SHAPE_SOURCES})
target_link_libraries(poc_route_probe PRIVATE pae_core)

# G2 (GATE-03): the fair-comparison probe - same input, same reference, same
# sampling, same camera for every route; E1/E2/E3 measured separately.
add_executable(poc_fair_comparison_probe unit/poc_fair_comparison_probe.cpp ${PAE_POC_SHAPE_SOURCES})
target_link_libraries(poc_fair_comparison_probe PRIVATE pae_core)

# PoC-SM (AUTH-08): the v2 Shape Model slice.
#
# AL-2  CharacterDefinition  - the five-layer contract
# AL-3  SharedSkinEnvelope    - one continuous outer surface
# AL-5  SeamAuditV2           - the shoulder-root seam under the OQ-03
#                              fixed definition
#
# These sources live in native/procedural/shape/v2/ and are compiled ONLY
# into the host PoC targets. The production Android build never sees them
# (AUTH-08 FB-5 / FB-7).
set(PAE_POC_SHAPE_V2_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/CharacterDefinition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/SharedSkinEnvelope.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/SeamAuditV2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/CrossBoundarySeamAudit.cpp
)

# The contract test needs the contract only; the v2 module is self-contained
# and does not link against the frozen G2 shape sources.
add_executable(pae_poc_sm_contract_tests unit/poc_sm_contract_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/CharacterDefinition.cpp)
target_include_directories(pae_poc_sm_contract_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_sm_contract_tests PRIVATE pae_core)
add_test(NAME pae_poc_sm_contract_tests COMMAND pae_poc_sm_contract_tests)

add_executable(pae_poc_sm_envelope_tests unit/poc_sm_envelope_tests.cpp
    ${PAE_POC_SHAPE_SOURCES} ${PAE_POC_SHAPE_V2_SOURCES})
target_include_directories(pae_poc_sm_envelope_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_sm_envelope_tests PRIVATE pae_core)
add_test(NAME pae_poc_sm_envelope_tests COMMAND pae_poc_sm_envelope_tests)

add_executable(pae_poc_sm_seam_tests unit/poc_sm_seam_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/SeamAuditV2.cpp)
target_include_directories(pae_poc_sm_seam_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_sm_seam_tests PRIVATE pae_core)
add_test(NAME pae_poc_sm_seam_tests COMMAND pae_poc_sm_seam_tests)

add_executable(poc_sm_measure_probe unit/poc_sm_measure_probe.cpp
    ${PAE_POC_SHAPE_SOURCES} ${PAE_POC_SHAPE_V2_SOURCES})
target_link_libraries(poc_sm_measure_probe PRIVATE pae_core)

# Round 2 AL-5 experiment: independent CROSS-BOUNDARY seam audit. This is a
# separate tool; the signed OQ-03 ring-band tool above is not modified.
add_executable(pae_poc_sm_cross_seam_tests unit/poc_sm_cross_seam_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/CrossBoundarySeamAudit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/SeamAuditV2.cpp)
target_include_directories(pae_poc_sm_cross_seam_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_sm_cross_seam_tests PRIVATE pae_core)
add_test(NAME pae_poc_sm_cross_seam_tests COMMAND pae_poc_sm_cross_seam_tests)

add_executable(poc_sm_cross_seam_probe unit/poc_sm_cross_seam_probe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/CrossBoundarySeamAudit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../native/procedural/shape/v2/SeamAuditV2.cpp)
target_link_libraries(poc_sm_cross_seam_probe PRIVATE pae_core)

# Round 2 AL-6: complete single-parameter differential test and evidence probe.
add_executable(pae_poc_sm_param_diff_tests unit/poc_sm_param_diff_tests.cpp
    ${PAE_POC_SHAPE_V2_SOURCES})
target_include_directories(pae_poc_sm_param_diff_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_sm_param_diff_tests PRIVATE pae_core)
add_test(NAME pae_poc_sm_param_diff_tests COMMAND pae_poc_sm_param_diff_tests)

add_executable(poc_sm_param_probe unit/poc_sm_param_probe.cpp
    ${PAE_POC_SHAPE_V2_SOURCES})
target_link_libraries(poc_sm_param_probe PRIVATE pae_core)

# Round 2: five-layer semantic contract checks (landmarks are NOT placed).
add_executable(pae_poc_sm_semantics_tests unit/poc_sm_semantics_tests.cpp
    ${PAE_POC_SHAPE_V2_SOURCES})
target_include_directories(pae_poc_sm_semantics_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_poc_sm_semantics_tests PRIVATE pae_core)
add_test(NAME pae_poc_sm_semantics_tests COMMAND pae_poc_sm_semantics_tests)

# Round 2 gray-clay evidence: 7 fixed comparison views + 2 close-ups.
add_executable(poc_sm_views_probe unit/poc_sm_views_probe.cpp
    ${PAE_POC_SHAPE_SOURCES} ${PAE_POC_SHAPE_V2_SOURCES})
target_link_libraries(poc_sm_views_probe PRIVATE pae_core)
