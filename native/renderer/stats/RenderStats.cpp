#include "RenderStats.h"

#include <algorithm>
#include <cstdio>

namespace pae {

void RenderStats::setBufferBytes(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    bufferBytes_ = bytes;
}

void RenderStats::beginFrame() {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastBegin_.time_since_epoch().count() != 0) {
        const float intervalMs = std::chrono::duration<float, std::milli>(now - lastBegin_).count();
        ++frameCount_;
        lastFrameMs_ = lastFrameMs_ <= 0.0f ? intervalMs : (lastFrameMs_ * 0.9f + intervalMs * 0.1f);
        fps_ = lastFrameMs_ > 0.0f ? 1000.0f / lastFrameMs_ : 0.0f;
        recordFrameMsLocked(intervalMs);
    }
    lastBegin_ = now;
    frameStart_ = now;
}

void RenderStats::recordFrameMsLocked(float intervalMs) {
    frameTimesMs_[frameTimeCursor_] = intervalMs;
    frameTimeCursor_ = (frameTimeCursor_ + 1u) % kFrameTimeHistory;
    if (frameTimeCount_ < kFrameTimeHistory) ++frameTimeCount_;
}

float RenderStats::percentileLocked(float fraction) const {
    if (frameTimeCount_ == 0) return 0.0f;
    float sorted[kFrameTimeHistory];
    std::copy(frameTimesMs_.begin(), frameTimesMs_.begin() + frameTimeCount_, sorted);
    std::sort(sorted, sorted + frameTimeCount_);
    const float scaled = fraction * static_cast<float>(frameTimeCount_ - 1);
    const auto index = static_cast<uint32_t>(scaled + 0.5f);
    return sorted[index < frameTimeCount_ ? index : frameTimeCount_ - 1];
}

void RenderStats::endFrame(uint32_t drawCalls, uint32_t triangles) {
    std::lock_guard<std::mutex> lock(mutex_);
    drawCalls_ = drawCalls;
    triangles_ = triangles;
}

std::string RenderStats::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    char buffer[320];
    std::snprintf(buffer, sizeof(buffer),
                  "frames=%llu fps=%.1f frame_ms=%.2f draw_calls=%u triangles=%u buffer_bytes=%llu "
                  "p50_ms=%.2f p95_ms=%.2f p99_ms=%.2f",
                  static_cast<unsigned long long>(frameCount_), fps_, lastFrameMs_,
                  drawCalls_, triangles_, static_cast<unsigned long long>(bufferBytes_),
                  percentileLocked(0.50f), percentileLocked(0.95f), percentileLocked(0.99f));
    return std::string(buffer);
}

} // namespace pae
