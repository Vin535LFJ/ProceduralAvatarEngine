#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace pae {

// Rolling render statistics with frame-time percentiles (M9).
class RenderStats {
public:
    static constexpr uint32_t kFrameTimeHistory = 256;

    void beginFrame();
    void endFrame(uint32_t drawCalls, uint32_t triangles);
    void setBufferBytes(uint64_t bytes);

    uint64_t frameCount() const { return frameCount_; }
    float fps() const { return fps_; }
    float lastFrameMs() const { return lastFrameMs_; }
    uint32_t drawCalls() const { return drawCalls_; }
    uint32_t triangles() const { return triangles_; }
    std::string toString() const;

private:
    void recordFrameMsLocked(float intervalMs);
    float percentileLocked(float fraction) const; // requires lock held

    mutable std::mutex mutex_;
    std::chrono::steady_clock::time_point frameStart_;
    std::chrono::steady_clock::time_point lastBegin_;
    std::array<float, kFrameTimeHistory> frameTimesMs_{};
    uint32_t frameTimeCount_ = 0;
    uint32_t frameTimeCursor_ = 0;
    uint64_t frameCount_ = 0;
    uint32_t drawCalls_ = 0;
    uint32_t triangles_ = 0;
    uint64_t bufferBytes_ = 0;
    float lastFrameMs_ = 0.0f;
    float fps_ = 0.0f;
};

} // namespace pae
