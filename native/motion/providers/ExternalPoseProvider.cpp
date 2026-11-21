#include "ExternalPoseProvider.h"

namespace pae {

bool ExternalPoseProvider::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = true;
    return true;
}

void ExternalPoseProvider::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    hasFrame_ = false;
}

bool ExternalPoseProvider::pollLatest(PoseFrame& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_ || !hasFrame_) return false;
    out = latest_;
    return true;
}

void ExternalPoseProvider::push(const PoseFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_ = frame; // overwrite: never queue stale frames
    hasFrame_ = true;
}

void ExternalPoseProvider::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    hasFrame_ = false;
}

bool ExternalPoseProvider::hasFrame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hasFrame_;
}

} // namespace pae
