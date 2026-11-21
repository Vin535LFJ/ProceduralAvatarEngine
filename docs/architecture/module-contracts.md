# Module Contracts — M1 冻结接口

## 公共数据结构（C++17，`pae` 命名空间）
```cpp
enum class CoordinateSpace : uint8_t { NormalizedImage, PixelImage, ProviderWorld, AvatarLocal, EngineWorld };

struct PoseLandmark {
  uint16_t id; Vec3 position; float visibility; float presence; bool valid;
};
struct PoseFrame {
  int64_t timestampNs; CoordinateSpace coordinateSpace; uint32_t landmarkCount;
  std::array<PoseLandmark, 33> landmarks; float poseConfidence; bool hasPerson;
  uint32_t imageWidth, imageHeight; int rotationDegrees; bool mirrored;
};
struct BodyParameters { float height, headScale, headWidth, headHeight, shoulderWidth,
  chestWidth, waistWidth, hipWidth, torsoLength, neckLength, upperArmLength,
  lowerArmLength, armRadius, upperLegLength, lowerLegLength, legRadius,
  handScale, footLength, footWidth, stylization; uint32_t detailLevel; uint64_t seed; };
struct AppearanceParameters { Vec3 skinColor, hairColor, clothColor; };
struct AvatarParameters { BodyParameters body; AppearanceParameters appearance; uint64_t revision; };
struct SceneParameters { BackgroundMode mode; Vec3 colorA, colorB; uint64_t revision; };
struct Vertex { Vec3 position, normal; Vec2 uv; U16Vec4 joints; Vec4 weights; Vec4 color; };
struct SubMesh { uint32_t firstIndex, indexCount; uint16_t materialId; };
struct MeshData { std::vector<Vertex> vertices; std::vector<uint32_t> indices;
  std::vector<SubMesh> subMeshes; Aabb bounds; };
```

## Provider 接口
```cpp
class IPoseProvider {
public:
  virtual ~IPoseProvider() = default;
  virtual bool start(const PoseProviderConfig&) = 0;
  virtual void stop() = 0;
  virtual bool pollLatest(PoseFrame& out) = 0;
};
```
Kotlin ML Kit 适配通过 JNI 调用 `nativePushPose(float[] landmarks, confidence, hasPerson, timestampNs, imageW, imageH, rotation, mirrored)` 写入容量 1 的缓冲；Motion Runtime 只读该缓冲。

## 骨架 ID（固定 0–21）
Root, Hips, Spine, Chest, Neck, Head, LShoulder, LUpperArm, LLowerArm, LHand, RShoulder, RUpperArm, RLowerArm, RHand, LUpperLeg, LLowerLeg, LFoot, LToe, RUpperLeg, RLowerLeg, RFoot, RToe。

## 渲染命令（有界队列，容量 64）
`SurfaceCreated{ANativeWindow*, w, h}`, `SurfaceChanged{w,h}`, `SurfaceDestroyed`, `SetAvatar{params}`, `SetScene{params}`, `SetBonePalette{BonePalette}`, `SetQuality{level}`, `Shutdown`。Render Thread 独占消费；队列满时合并/丢弃可过期命令（Pose 只保留最新）。

## 引擎 API（JNI 边界）
`nativeInit`, `nativeSurfaceCreated(window,w,h)`, `nativeSurfaceChanged`, `nativeSurfaceDestroyed`, `nativePushPose`, `nativeSetAvatarParameters`, `nativeSetSceneParameters`, `nativeSetQuality`, `nativeGetStats`, `nativeShutdown`。
