#pragma once

#include "motion/poseframe/PoseFrame.h"
#include "skeleton/Skeleton.h"

namespace pae {

class IPoseProvider {
public:
    virtual ~IPoseProvider() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool pollLatest(PoseFrame& out) = 0;
};

} // namespace pae
