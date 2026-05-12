/**
 * @file deadzone_test.cpp
 * @brief Unit tests for the software deadzone math used in JoystickHandler.
 *
 * These tests cover:
 *   - Bidirectional (stick) deadzone: scaled removal from centre
 *   - Unidirectional (trigger) deadzone: scaled removal from bottom
 *   - Per-axis DZ computation: minimum deadzone for a single axis
 *   - "Zero Out DZ" computation: maximum absolute resting value across non-trigger axes
 *   - Edge cases: zero input, full-scale input, exact deadzone boundary, INT16_MIN
 *
 * No SDL or ImGui dependency is required; the math is reproduced directly.
 * Build with: cmake -DBUILD_TESTS=ON .. && cmake --build . && ctest --output-on-failure
 */

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

// ============================================================
// Deadzone math mirrored from JoystickHandler.h
// ============================================================

/**
 * @brief Applies scaled bidirectional (stick) deadzone.
 * @param rawSdlValue  SDL 16-bit axis value (-32768 to 32767).
 * @param deadzoneLimit  Deadzone threshold on the same 0-32767 scale.
 * @return Normalised float in [-1.0, 1.0]; 0.0 inside the deadzone.
 */
static float applyStickDeadzone(int16_t rawSdlValue, int16_t deadzoneLimit)
{
    float normalizedValue = rawSdlValue / 32767.0f;
    float dzFloat         = deadzoneLimit / 32767.0f;

    if (std::abs(normalizedValue) <= dzFloat) {
        return 0.0f;
    }
    float sign = (normalizedValue > 0.0f) ? 1.0f : -1.0f;
    return sign * ((std::abs(normalizedValue) - dzFloat) / (1.0f - dzFloat));
}

/**
 * @brief Applies scaled unidirectional (trigger) deadzone — Windows/Linux convention.
 * @param rawSdlValue  SDL 16-bit axis value (-32768 = fully released, 32767 = fully pressed).
 * @param deadzoneLimit  Deadzone threshold on a 0-32767 scale.
 * @return Normalised float in [0.0, 1.0]; 0.0 inside the deadzone.
 */
static float applyTriggerDeadzone(int16_t rawSdlValue, int16_t deadzoneLimit)
{
    float normalizedValue = (rawSdlValue + 32768.0f) / 65535.0f;
    float dzFloat         = deadzoneLimit / 65535.0f;

    if (normalizedValue <= dzFloat) {
        return 0.0f;
    }
    return (normalizedValue - dzFloat) / (1.0f - dzFloat);
}

/**
 * @brief Computes the minimum deadzone [0, 0.25] needed to silence a single
 *        non-trigger axis at its current resting position.
 *        This is the value applied by the [Zero] button and "Zero Out DZ" mode.
 * @param rawSdlValue  Raw SDL axis value from a non-trigger axis.
 * @return Minimum deadzone as a normalised float, clamped to [0, 0.25].
 */
static float computeAxisDeadzone(int16_t rawSdlValue)
{
    float dz = std::abs(static_cast<int>(rawSdlValue)) / 32767.0f;
    return std::min(dz, 0.25f);
}

/**
 * @brief Computes the minimum deadzone [0, 0.25] needed to silence a set of
 *        non-trigger axes at their current resting positions.
 *        Useful for deriving a single global DZ that covers all axes.
 * @param rawValues  Vector of raw SDL axis values from non-trigger axes.
 * @return Minimum deadzone as a normalised float, clamped to [0, 0.25].
 */
static float computeMinDeadzone(const std::vector<int16_t>& rawValues)
{
    int maxRaw = 0;
    for (int16_t v : rawValues) {
        int absVal = std::abs(static_cast<int>(v));
        if (absVal > maxRaw) {
            maxRaw = absVal;
        }
    }
    return std::min(maxRaw / 32767.0f, 0.25f);
}

// ============================================================
// Tiny test harness
// ============================================================

static int g_passed = 0;
static int g_failed = 0;

static void check(bool condition, const char* description)
{
    if (condition) {
        ++g_passed;
        printf("  PASS  %s\n", description);
    } else {
        ++g_failed;
        printf("  FAIL  %s\n", description);
    }
}

/** Floating-point near-equality with a configurable epsilon. */
static bool nearEq(float a, float b, float eps = 1e-4f)
{
    return std::abs(a - b) < eps;
}

// ============================================================
// Test suites
// ============================================================

static void testStickDeadzone()
{
    printf("\n[Stick Deadzone]\n");

    // Zero input with zero deadzone → zero output
    check(nearEq(applyStickDeadzone(0, 0), 0.0f),
          "zero input, zero DZ → 0.0");

    // Full positive deflection, zero deadzone → 1.0
    check(nearEq(applyStickDeadzone(32767, 0), 1.0f),
          "full positive deflection, zero DZ → 1.0");

    // Full negative deflection, zero deadzone → exactly -1.0
    check(applyStickDeadzone(-32767, 0) < -0.999f,
          "full negative deflection (-32767), zero DZ → -1.0");

    // Inside deadzone → exactly 0.0
    check(nearEq(applyStickDeadzone(100, 1000), 0.0f),
          "value inside DZ → 0.0");

    // Exactly at deadzone boundary → 0.0 (not-strictly-greater)
    int16_t limit = 3277; // ~10%
    int16_t onBoundary = 3277;
    check(nearEq(applyStickDeadzone(onBoundary, limit), 0.0f),
          "value at DZ boundary → 0.0");

    // Just above deadzone → small positive
    check(applyStickDeadzone(3278, limit) > 0.0f,
          "value just above DZ → positive");

    // Scaled deadzone: at 50% input, 10% DZ → (0.5 - 0.1) / 0.9 ≈ 0.4444
    int16_t raw50pct = static_cast<int16_t>(32767 / 2);
    float expected = (0.5f - 0.1f) / 0.9f;
    check(nearEq(applyStickDeadzone(raw50pct, 3277), expected, 1e-3f),
          "50%% input, 10%% DZ → scaled ~0.444");

    // Negative side mirrors positive
    int16_t negRaw50pct = static_cast<int16_t>(-32767 / 2);
    check(nearEq(applyStickDeadzone(negRaw50pct, 3277), -expected, 1e-3f),
          "negative 50%% input, 10%% DZ → scaled ~-0.444");

    // INT16_MIN should not overflow and result in a large negative (it maps to ≈ -1.0003)
    float minResult = applyStickDeadzone(static_cast<int16_t>(-32768), 0);
    check(minResult <= -0.999f,
          "INT16_MIN (-32768) with zero DZ → near -1.0, no overflow");
}

static void testTriggerDeadzone()
{
    printf("\n[Trigger Deadzone]\n");

    // Fully released (-32768) → 0.0
    check(nearEq(applyTriggerDeadzone(static_cast<int16_t>(-32768), 0), 0.0f),
          "fully released trigger → 0.0");

    // Fully pressed (32767) → 1.0
    check(nearEq(applyTriggerDeadzone(32767, 0), 1.0f),
          "fully pressed trigger → 1.0");

    // Inside deadzone → 0.0
    // -32000 normalises to 768/65535 ≈ 0.01172; a 2% DZ (dzFloat ≈ 0.02) covers it.
    {
        int16_t dz2pct = static_cast<int16_t>(0.02f * 65535.0f);
        check(nearEq(applyTriggerDeadzone(static_cast<int16_t>(-32000), dz2pct), 0.0f, 1e-3f),
              "near-resting trigger inside 2%% DZ → 0.0");
    }

    // With a 10% deadzone (of 65535): threshold ≈ 6554 raw from bottom
    // normalised bottom = (-32768 + 32768) / 65535 = 0.0, DZ = 6554/65535 ≈ 0.1
    int16_t dz10 = static_cast<int16_t>(0.1f * 65535.0f); // ≈ 6554 on the 0-32767 scale
    // At 50% normalised (raw ≈ -32768 + 32767 = -1 off centre), which maps to exactly 0.5:
    int16_t mid = static_cast<int16_t>(-32768 + static_cast<int>(65535 * 0.5f));
    float result = applyTriggerDeadzone(mid, dz10);
    check(result > 0.0f && result < 1.0f,
          "mid-travel trigger with 10%% DZ → between 0 and 1");
}

static void testMinDeadzoneComputation()
{
    printf("\n[Min DZ Computation]\n");

    // No axes → 0
    check(nearEq(computeMinDeadzone({}), 0.0f),
          "empty axis list → 0.0");

    // Single axis at rest (0) → 0
    check(nearEq(computeMinDeadzone({0}), 0.0f),
          "single axis at 0 → 0.0");

    // Single axis with drift of 4096 → 4096/32767 ≈ 0.125
    check(nearEq(computeMinDeadzone({static_cast<int16_t>(-4096)}), 4096.0f / 32767.0f, 1e-4f),
          "drift of -4096 → 0.125");

    // Multiple axes: worst case is 5982 (≈ 18.3%)
    std::vector<int16_t> axes = {
        static_cast<int16_t>(-512),
        static_cast<int16_t>(521),
        static_cast<int16_t>(5982)
    };
    float expected = 5982.0f / 32767.0f;
    check(nearEq(computeMinDeadzone(axes), expected, 1e-4f),
          "multi-axis: worst-case 5982 dominates");

    // Capped at 0.25 even when drift is extreme
    check(nearEq(computeMinDeadzone({32767}), 0.25f),
          "extreme drift (32767) clamped to 0.25");

    // INT16_MIN abs = 32768 → also clamped to 0.25 (no overflow)
    check(nearEq(computeMinDeadzone({static_cast<int16_t>(-32768)}), 0.25f),
          "INT16_MIN (-32768) clamped to 0.25, no overflow");

    // Negative and positive same absolute value → same result
    check(nearEq(computeMinDeadzone({static_cast<int16_t>(-4422)}),
                 computeMinDeadzone({static_cast<int16_t>(4422)}), 1e-6f),
          "symmetric drift: -4422 and +4422 produce identical min DZ");
}

static void testMinDeadzoneZeroesAxes()
{
    printf("\n[Min DZ silences drifting axes]\n");

    // After applying the computed min DZ to each axis, all should read 0.0
    std::vector<int16_t> resting = {
        static_cast<int16_t>(-512),
        static_cast<int16_t>(521),
        static_cast<int16_t>(5982)
    };
    float minDZ = computeMinDeadzone(resting);
    int16_t dzLimit = static_cast<int16_t>(minDZ * 32767.0f);

    bool allZero = true;
    for (int16_t v : resting) {
        if (!nearEq(applyStickDeadzone(v, dzLimit), 0.0f, 1e-3f)) {
            allZero = false;
        }
    }
    check(allZero, "computed min DZ silences all resting stick axes");
}

static void testPerAxisDeadzone()
{
    printf("\n[Per-Axis DZ (Zero Out DZ)]\n");

    // Axis at rest (0) → DZ = 0
    check(nearEq(computeAxisDeadzone(0), 0.0f),
          "axis at 0 → per-axis DZ 0.0");

    // Drift of +100 → 100/32767 ≈ 0.00305
    {
        float expected = 100.0f / 32767.0f;
        check(nearEq(computeAxisDeadzone(100), expected, 1e-5f),
              "drift of +100 → per-axis DZ 100/32767");
    }

    // Drift of -4096 → 4096/32767 ≈ 0.125 (12.5%)
    {
        float expected = 4096.0f / 32767.0f;
        check(nearEq(computeAxisDeadzone(static_cast<int16_t>(-4096)), expected, 1e-5f),
              "drift of -4096 → per-axis DZ ≈ 0.125");
    }

    // Extreme value (full scale) → clamped to 0.25
    check(nearEq(computeAxisDeadzone(32767), 0.25f),
          "full-scale drift clamped to 0.25");

    // INT16_MIN (-32768): abs = 32768 → clamped to 0.25, no overflow
    check(nearEq(computeAxisDeadzone(static_cast<int16_t>(-32768)), 0.25f),
          "INT16_MIN (-32768) clamped to 0.25, no overflow");

    // Symmetry: negative and positive same magnitude → same DZ
    check(nearEq(computeAxisDeadzone(static_cast<int16_t>(-781)),
                 computeAxisDeadzone(static_cast<int16_t>(781)), 1e-6f),
          "symmetric drift: -781 and +781 produce identical per-axis DZ");

    // Per-axis DZ silences the axis that produced it
    {
        int16_t raw = static_cast<int16_t>(-4096);
        float dz = computeAxisDeadzone(raw);
        int16_t dzLimit = static_cast<int16_t>(dz * 32767.0f);
        check(nearEq(applyStickDeadzone(raw, dzLimit), 0.0f, 1e-3f),
              "per-axis DZ from -4096 silences that axis");
    }

    // Each axis in a set can be zeroed with its own individual DZ
    {
        struct Axis { int16_t raw; };
        std::vector<Axis> axes = {
            {static_cast<int16_t>(-512)},
            {static_cast<int16_t>(5982)},
            {static_cast<int16_t>(781)}
        };
        bool allZero = true;
        for (auto& ax : axes) {
            float dz = computeAxisDeadzone(ax.raw);
            int16_t dzLimit = static_cast<int16_t>(dz * 32767.0f);
            if (!nearEq(applyStickDeadzone(ax.raw, dzLimit), 0.0f, 1e-3f)) {
                allZero = false;
            }
        }
        check(allZero, "per-axis DZ individually silences each drifting axis");
    }
}

// ============================================================
// Entry point
// ============================================================

int main()
{
    printf("=== HIDTester Deadzone Unit Tests ===\n");

    testStickDeadzone();
    testTriggerDeadzone();
    testMinDeadzoneComputation();
    testMinDeadzoneZeroesAxes();
    testPerAxisDeadzone();

    printf("\n---\nResults: %d passed, %d failed\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}
