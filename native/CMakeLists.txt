cmake_minimum_required(VERSION 3.20)
project(ProceduralAvatarEngineCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PAE_HOST "Build host-side unit tests instead of Android targets" OFF)

set(PAE_CORE_SOURCES
    core/diagnostics/Log.cpp
    core/config/EngineConfig.cpp
    procedural/MeshValidator.cpp
    procedural/primitives/Primitives.cpp
    procedural/surfaces/ParametricSurfaces.cpp
    procedural/fusion/SdfFusion.cpp
    procedural/humanoid/Humanoid.cpp
    procedural/humanoid/canonical/CanonicalTopology.cpp
    procedural/humanoid/canonical/CanonicalSkinGenerator.cpp
    procedural/humanoid/canonical/ContinuousSkinGenerator.cpp
    procedural/humanoid/FaceGenerator.cpp
    procedural/humanoid/HeadAnatomy.cpp
    procedural/humanoid/HairGenerator.cpp
    procedural/humanoid/ClothingGenerator.cpp
    procedural/humanoid/anatomy/AnatomyField.cpp
    procedural/humanoid/anatomy/HumanAnatomyGenerator.cpp
    procedural/MeshTopology.cpp
    procedural/character/BodyPatchGenerator.cpp
    skeleton/Skeleton.cpp
    skeleton/Skinning.cpp
    skeleton/weights/WeightGenerator.cpp
    motion/providers/SyntheticPoseProvider.cpp
    motion/providers/ExternalPoseProvider.cpp
    motion/retargeting/Retargeting.cpp
    motion/MotionRuntime.cpp
)

add_library(pae_core STATIC ${PAE_CORE_SOURCES})
target_include_directories(pae_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

if(PAE_HOST)
    target_compile_definitions(pae_core PUBLIC PAE_HOST_BUILD=1)
    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests ${CMAKE_CURRENT_BINARY_DIR}/pae_tests)
else()
    target_compile_definitions(pae_core PUBLIC PAE_ANDROID=1)

    set(PAE_RENDER_SOURCES
        renderer/egl/EglContextManager.cpp
        renderer/gl/GlCapabilities.cpp
        renderer/gl/ShaderProgram.cpp
        renderer/gl/GpuMesh.cpp
        renderer/gl/SkinnedGpuMesh.cpp
        renderer/gl/BonePaletteUbo.cpp
        renderer/stats/RenderStats.cpp
        renderer/RenderThread.cpp
        runtime/EngineRuntime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/main/cpp/jni_bridge.cpp
    )

    add_library(pae_native SHARED ${PAE_RENDER_SOURCES})
    target_include_directories(pae_native PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    )
    target_link_libraries(pae_native pae_core EGL GLESv3 android log)
endif()
