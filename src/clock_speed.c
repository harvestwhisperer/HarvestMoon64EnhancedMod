// Configurable in-game clock speed.
//
// Original updateClock() advances the lock by exactly 10 in-game seconds per call,
// then runs a single carry pass. The carry logic resets fields to 0 (it never
// subtracts 60) and each rollover is a single `if`, so it only works for exact
// 10-second steps. To scale time, we replay that atomic 10-second step
// N times rather than using one larger increment, accumulating fractional steps
// across calls so speeds below 1.0 (and non-integer speeds) work. At scale 1.0
// this is bit-identical to the original, including when toggleMonthlyLetterBits() /
// setupNewYear() fire.

#include "common.h"
#include "modding.h"

#include "game/time.h"

#include "mod_config.h"

extern void toggleMonthlyLetterBits(void);
extern void setupNewYear(void);

static double s_time_step_accumulator = 0.0;

// The carry/normalization pass from original updateClock(), without the +10.
static void normalize_clock(void) {

    if (gSeconds >= 60) {
        gSeconds = 0;
        gMinutes++;
    }

    if (gMinutes >= 60) {
        gMinutes = 0;
        gHour++;
        if (gHour == 6) {
            gDayOfMonth++;
            gDayOfWeek++;
        }
    }

    if (gHour >= 24) {
        gHour = 0;
    }

    if (gDayOfWeek >= 7) {
        gDayOfWeek = SUNDAY;
    }

    if (gDayOfMonth >= 31) {
        gDayOfMonth = 1;
        gSeason++;
        toggleMonthlyLetterBits();
    }

    if (gSeason >= 5) {
        gSeason = SPRING;
        gYear++;
        setupNewYear();
    }

    if (gYear >= 100) {
        gYear = 99;
    }

    gSeasonTomorrow = gSeason;

    if ((gDayOfMonth + 1) >= 31) {
        gSeasonTomorrow = gSeason + 1;
    }

    if (gSeasonTomorrow >= 5) {
        gSeasonTomorrow = SPRING;
    }

}

// One original-scale tick: advance 10 in-game seconds and carry.
static void advance_clock_once(void) {
    gSeconds += 10;
    normalize_clock();
}

RECOMP_PATCH void updateClock(u8 incrementSeconds) {

    s32 steps;
    s32 i;

    if (incrementSeconds != TRUE || !config_enabled("enable_clock_speed")) {
        if (incrementSeconds == TRUE) {
            gSeconds += 10;
        }
        normalize_clock();
        return;
    }

    // Scaled advance: replay the atomic 10-second step `steps` times, carrying
    // the fractional remainder into the next call.
    s_time_step_accumulator += clock_speed_scale();

    steps = (s32)s_time_step_accumulator;
    s_time_step_accumulator -= steps;

    for (i = 0; i < steps; i++) {
        advance_clock_once();
    }

}
