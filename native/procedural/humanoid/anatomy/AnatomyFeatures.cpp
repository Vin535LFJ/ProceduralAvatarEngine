#include "AnatomyFeatures.h"

namespace pae::anatomy {

AnatomyFeatureSet baseMaleFeatures() {
    AnatomyFeatureSet f;

    // Thorax: flat-deep ovoid, flatter at the front (sternum) than the back.
    f.ribcage.sectionScale = Vec2{1.00f, 0.92f};
    f.ribcage.squash = 1.25f;
    f.ribcage.blend = 0.055f;

    // Abdomen: slightly boxy, narrower than the thorax.
    f.abdomen.sectionScale = Vec2{0.86f, 0.90f};
    f.abdomen.squash = 1.20f;
    f.abdomen.blend = 0.050f;

    // Pelvis: wider than deep, squared at the iliac crest.
    f.pelvis.sectionScale = Vec2{1.08f, 0.92f};
    f.pelvis.squash = 1.30f;
    f.pelvis.blend = 0.048f;

    // Lumbar: the lower back mass that carries the waist into the hips.
    f.lumbar.sectionScale = Vec2{0.74f, 0.86f};
    f.lumbar.bellyAt = 0.35f;
    f.lumbar.bellyGain = 1.20f;
    f.lumbar.groove = 0.35f;
    f.lumbar.blend = 0.040f;

    f.neck.sectionScale = Vec2{0.94f, 1.0f};
    f.neck.blend = 0.045f;

    f.head.sectionScale = Vec2{1.0f, 0.96f};

    // Pec major: a THIN, WIDE fan from the sternum onto the humerus. This is
    // the shape that reads as "chest" rather than "front of an ellipsoid".
    f.pectoralis.sectionScale = Vec2{1.55f, 0.42f};
    f.pectoralis.bellyAt = 0.40f;
    f.pectoralis.bellyGain = 1.28f;
    f.pectoralis.squash = 1.35f;
    f.pectoralis.groove = 0.25f;   // separates the sternal and clavicular heads
    f.pectoralis.blend = 0.026f;

    // Clavicle ridge: narrow, pushed up and back toward the acromion.
    f.clavicle.sectionScale = Vec2{1.0f, 0.85f};
    f.clavicle.blend = 0.020f;

    // Trapezius: broad sheet from the neck base to the shoulder spine.
    f.trapezius.sectionScale = Vec2{1.70f, 0.55f};
    f.trapezius.bellyAt = 0.50f;
    f.trapezius.squash = 1.20f;
    f.trapezius.blend = 0.030f;

    // Deltoid: the shoulder cap, slightly deeper than wide.
    f.deltoid.sectionScale = Vec2{1.0f, 1.05f};
    f.deltoid.blend = 0.055f;

    // Biceps: proximal-heavy spindle on the front of the upper arm.
    f.biceps.sectionScale = Vec2{1.0f, 0.95f};
    f.biceps.bellyAt = 0.38f;
    f.biceps.bellyGain = 1.45f;
    f.biceps.squash = 1.15f;
    f.biceps.blend = 0.024f;

    // Triceps: broader, flatter mass on the back of the upper arm.
    f.triceps.sectionScale = Vec2{1.0f, 0.85f};
    f.triceps.bellyAt = 0.45f;
    f.triceps.bellyGain = 1.30f;
    f.triceps.blend = 0.024f;

    // Brachialis: small lateral head under the deltoid.
    f.brachialis.sectionScale = Vec2{0.8f, 0.9f};
    f.brachialis.bellyAt = 0.50f;
    f.brachialis.blend = 0.020f;

    f.elbow.sectionScale = Vec2{1.0f, 0.95f};
    f.elbow.blend = 0.026f;

    // Forearm: the flexor group is the medial (body-side) belly, the extensor
    // the lateral one, and the pinch between them is what makes a forearm read
    // as a forearm instead of a tube.
    f.forearmFlexor.sectionScale = Vec2{1.0f, 0.90f};
    f.forearmFlexor.bellyAt = 0.34f;
    f.forearmFlexor.bellyGain = 1.40f;
    f.forearmFlexor.groove = 0.30f;
    f.forearmFlexor.blend = 0.026f;

    f.forearmExtensor.sectionScale = Vec2{1.0f, 0.88f};
    f.forearmExtensor.bellyAt = 0.42f;
    f.forearmExtensor.bellyGain = 1.30f;
    f.forearmExtensor.blend = 0.026f;

    f.wrist.sectionScale = Vec2{1.0f, 0.85f};
    f.wrist.blend = 0.024f;

    // Rectus abdominis: two vertical ridges separated from the rest of the
    // abdominal wall so the front of the belly has segment structure.
    f.rectus.sectionScale = Vec2{0.34f, 0.85f};
    f.rectus.squash = 1.20f;
    f.rectus.groove = 0.40f;
    f.rectus.blend = 0.030f;

    // Intercostal grooves: shallow separations between the ribs.
    f.intercostal.sectionScale = Vec2{0.16f, 0.55f};
    f.intercostal.blend = 0.014f;

    return f;
}

} // namespace pae::anatomy