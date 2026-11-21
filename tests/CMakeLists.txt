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

add_executable(pae_face_hair_tests unit/face_hair_tests.cpp)
target_include_directories(pae_face_hair_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)
target_link_libraries(pae_face_hair_tests PRIVATE pae_core)
add_test(NAME pae_face_hair_tests COMMAND pae_face_hair_tests)

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
