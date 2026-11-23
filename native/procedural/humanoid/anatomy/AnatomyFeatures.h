#pragma once

// M13: anatomical feature system for the upper body.
//
// M12 composed the body from a handful of ellipsoids and capsules blended
// together, which describes VOLUME but not FORM: the chest has no pectoral
// mass, the back no trapezius/latissimus, the arm no biceps/triceps bellies,
// the abdomen no rectus separation. This module names the anatomical regions
// and gives each its own shape controls, so the composed field can express
// real (cartoonised) musculature instead of a smooth balloon.
//
// Every feature is still an implicit primitive anchored to the BodyJoints
// bind landmarks, so the single-field / watertight extraction / mirror
// symmetry / field-based skin weights of M12 all keep working.

#include "core/math/Math.h"
#include "procedural/humanoid/Humanoid.h"

namespace pae::anatomy {

// Shape controls for one anatomical feature. All of them are multipliers over
// the anchors, so a preset can be scaled without touching the anchors.
struct MuscleShape {
    // Cross-section ellipse axes, as multipliers of the local radius.
    Vec2 sectionScale{1.0f, 1.0f};
    // Belly thickness: the muscle swells at this fraction along its axis
    // (0.5 = symmetric, <0.5 = proximal-heavy like biceps).
    float bellyAt = 0.45f;
    float bellyGain = 1.35f;
    // Cross-section fullness: >1 squares the section off (broader, flatter).
    float squash = 1.0f;
    // Section roll about the muscle axis (radians).
    float roll = 0.0f;
    // Lateral pinch: 0..1 carves the sides of the feature in, which is what
    // separates adjacent muscles (rectus segments, forearm groups).
    float groove = 0.0f;
    // Blend radius against the neighbouring features.
    float blend = 0.028f;
};

// Named anatomical features of the upper body. Base (male) preset; other
// presets (female / child / stylised) reuse the same fields with different
// values, so no feature is hard-wired to one body type.
struct AnatomyFeatureSet {
    // Axial column
    MuscleShape ribcage;      // thorax
    MuscleShape abdomen;      // abdominal wall
    MuscleShape pelvis;
    MuscleShape lumbar;       // waist -> hip transition mass
    MuscleShape neck;
    MuscleShape head;

    // Paired (+X modelled, mirrored)
    MuscleShape pectoralis;   // pec major fan: sternum -> humerus
    MuscleShape clavicle;     // sternum -> acromion ridge
    MuscleShape trapezius;    // neck base -> shoulder spine sheet
    MuscleShape deltoid;      // shoulder cap (anterior/lateral/posterior)
    MuscleShape biceps;       // upper arm anterior belly
    MuscleShape triceps;      // upper arm posterior belly
    MuscleShape brachialis;  // lateral upper-arm head
    MuscleShape elbow;
    MuscleShape forearmFlexor; // medial forearm group
    MuscleShape forearmExtensor; // lateral forearm group
    MuscleShape wrist;

    // Vertical midline features
    MuscleShape rectus;       // rectus abdominis segments (front midline)
    MuscleShape intercostal;  // rib separation grooves (front/side)
};

// Base male anatomy preset. Everything is intentionally readable at this
// figure scale rather than anatomically exact.
AnatomyFeatureSet baseMaleFeatures();

} // namespace pae::anatomy