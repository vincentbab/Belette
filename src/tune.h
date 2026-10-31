#ifndef TUNE_H_INCLUDED
#define TUNE_H_INCLUDED

#ifdef TUNE
#include <string>
#include <vector>
#endif

namespace Belette {

#ifdef TUNE
struct TunableParam;

inline std::vector<TunableParam*>& tunableParams() {
    static std::vector<TunableParam*> params;
    return params;
}

struct TunableParam {
    TunableParam(const char *name_, int &value_, int min_, int max_, double step_)
    : name(name_), value(value_), defaultValue(value_), min(min_), max(max_), step(step_) {
        tunableParams().push_back(this);
    }

    std::string name;
    int &value;
    int defaultValue;
    int min;
    int max;
    double step;
};

#define TUNABLE(name, value, min, max, step) \
    inline int name = value; \
    inline TunableParam name##Param(#name, name, min, max, step);
#else
#define TUNABLE(name, value, min, max, step) \
    constexpr int name = value;
#endif

// History bonus
TUNABLE(BonusMainMult, 155, 48, 256, 10)
TUNABLE(BonusMainBase, 81, 0, 256, 12)
TUNABLE(BonusMainMax, 1600, 512, 3072, 128)
TUNABLE(BonusContMult, 137, 48, 256, 10)
TUNABLE(BonusContBase, 88, 0, 256, 12)
TUNABLE(BonusContMax, 1640, 512, 3072, 128)
TUNABLE(BonusPawnMult, 153, 48, 256, 10)
TUNABLE(BonusPawnBase, 72, 0, 256, 12)
TUNABLE(BonusPawnMax, 1530, 512, 3072, 128)
TUNABLE(BonusCaptMult, 103, 48, 256, 10)
TUNABLE(BonusCaptBase, 73, 0, 256, 12)
TUNABLE(BonusCaptMax, 1570, 512, 3072, 128)

// History malus
TUNABLE(MalusMainMult, 117, 48, 256, 10)
TUNABLE(MalusMainBase, 39, 0, 256, 12)
TUNABLE(MalusMainMax, 1600, 512, 3072, 128)
TUNABLE(MalusContMult, 167, 48, 256, 10)
TUNABLE(MalusContBase, 9, -128, 256, 12)
TUNABLE(MalusContMax, 1470, 512, 3072, 128)
TUNABLE(MalusPawnMult, 127, 48, 256, 10)
TUNABLE(MalusPawnBase, 79, 0, 256, 12)
TUNABLE(MalusPawnMax, 1460, 512, 3072, 128)
TUNABLE(MalusCaptMult, 139, 48, 256, 10)
TUNABLE(MalusCaptBase, 37, 0, 256, 12)
TUNABLE(MalusCaptMax, 1690, 512, 3072, 128)

// History gravity, in units of 1024
TUNABLE(GravityCont, 11, 4, 24, 1)
TUNABLE(GravityCapt, 8, 4, 24, 1)

// Quiet ordering weights, in 1/1024
TUNABLE(OrderMain, 2170, 512, 4096, 200)
TUNABLE(OrderCont1, 1060, 256, 2560, 100)
TUNABLE(OrderCont2, 1440, 256, 2560, 100)
TUNABLE(OrderCont4, 974, 256, 2560, 100)
TUNABLE(OrderPawn, 2410, 512, 4096, 200)

// Evasion ordering weight, in 1/1024
TUNABLE(EvasionCont, 760, 256, 2560, 100)

// statScore weights, in 1/1024
TUNABLE(StatMain, 1810, 512, 4096, 200)
TUNABLE(StatCont1, 1240, 256, 2560, 100)
TUNABLE(StatCont2, 1110, 256, 2560, 100)

// Move picker
TUNABLE(GoodQuietThreshold, -597, -16000, 8000, 1000)
TUNABLE(ThreatMinor, 13900, 0, 30000, 1500)
TUNABLE(ThreatRook, 25600, 0, 50000, 2500)
TUNABLE(ThreatQueen, 50900, 0, 100000, 5000)
TUNABLE(CheckBonus, 8900, 0, 20000, 1000)
TUNABLE(TacticalMvv, 9, 2, 20, 1)
TUNABLE(BadCaptureSee, 50, 0, 150, 8)
TUNABLE(BadCaptureHist, 39, 0, 128, 6)

// Futility pruning, in 1/1024
TUNABLE(FpHistMult, 18, 0, 64, 2.5)
TUNABLE(CapFpHistMult, 76, 0, 192, 8)

// History pruning
TUNABLE(HpDepth, 5, 2, 8, 0.5)
TUNABLE(HpMargin, 3220, 1024, 8192, 350)

// Late move reduction (LMR), in 1/1024
TUNABLE(LmrQuietHist, 229, 64, 512, 24)
TUNABLE(LmrCaptHist, 293, 64, 512, 24)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
