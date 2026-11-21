#pragma once

#include "motion/providers/IPoseProvider.h"

#include <mutex>

namespace pae {

// Capacity-1 latest-value provider used by the Kotlin/ML Kit JNI adapter.
class ExternalPoseProvider : public IPoseProvider {
public:
    bool start() override;
    void stop() override;
    bool pollLatest(PoseFrame& out) override;

    void push(const PoseFrame& frame);
    void clear(); // drop the buffered frame (used on seek/reset)
    bool hasFrame() const;

private:
    mutable std::mutex mutex_;
    PoseFrame latest_{};
    bool hasFrame_ = false;
    bool running_ = false;
};

} // namespace pae
