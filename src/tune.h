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
TUNABLE(BonusMainMult, 144, 48, 256, 10)
TUNABLE(BonusMainBase, 90, 0, 256, 12)
TUNABLE(BonusMainMax, 1560, 512, 3072, 128)
TUNABLE(BonusContMult, 161, 48, 256, 10)
TUNABLE(BonusContBase, 91, 0, 256, 12)
TUNABLE(BonusContMax, 1560, 512, 3072, 128)
TUNABLE(BonusPawnMult, 153, 48, 256, 10)
TUNABLE(BonusPawnBase, 59, 0, 256, 12)
TUNABLE(BonusPawnMax, 1340, 512, 3072, 128)
TUNABLE(BonusCaptMult, 89, 48, 256, 10)
TUNABLE(BonusCaptBase, 65, 0, 256, 12)
TUNABLE(BonusCaptMax, 1350, 512, 3072, 128)

// History malus
TUNABLE(MalusMainMult, 113, 48, 256, 10)
TUNABLE(MalusMainBase, 36, 0, 256, 12)
TUNABLE(MalusMainMax, 1710, 512, 3072, 128)
TUNABLE(MalusContMult, 202, 48, 256, 10)
TUNABLE(MalusContBase, 11, 0, 256, 12)
TUNABLE(MalusContMax, 1010, 512, 3072, 128)
TUNABLE(MalusPawnMult, 122, 48, 256, 10)
TUNABLE(MalusPawnBase, 75, 0, 256, 12)
TUNABLE(MalusPawnMax, 1600, 512, 3072, 128)
TUNABLE(MalusCaptMult, 125, 48, 256, 10)
TUNABLE(MalusCaptBase, 50, 0, 256, 12)
TUNABLE(MalusCaptMax, 1680, 512, 3072, 128)

// History gravity, in units of 1024
TUNABLE(GravityCont, 10, 4, 24, 1)
TUNABLE(GravityCapt, 8, 4, 24, 1)

// Quiet ordering weights, in 1/1024
TUNABLE(OrderMain, 2100, 512, 4096, 200)
TUNABLE(OrderCont1, 1010, 256, 2560, 100)
TUNABLE(OrderCont2, 1500, 256, 2560, 100)
TUNABLE(OrderCont4, 1140, 256, 2560, 100)
TUNABLE(OrderPawn, 2710, 512, 4096, 200)

// Evasion ordering weight, in 1/1024
TUNABLE(EvasionCont, 674, 256, 2560, 100)

// statScore weights, in 1/1024
TUNABLE(StatMain, 1530, 512, 4096, 200)
TUNABLE(StatCont1, 1340, 256, 2560, 100)
TUNABLE(StatCont2, 1330, 256, 2560, 100)

// Move picker
TUNABLE(GoodQuietThreshold, -1120, -16000, 8000, 1000)
TUNABLE(ThreatMinor, 11200, 0, 30000, 1500)
TUNABLE(ThreatRook, 23600, 0, 50000, 2500)
TUNABLE(ThreatQueen, 47800, 0, 100000, 5000)
TUNABLE(CheckBonus, 7600, 0, 20000, 1000)
TUNABLE(TacticalMvv, 8, 2, 20, 1)
TUNABLE(BadCaptureSee, 39, 0, 150, 8)
TUNABLE(BadCaptureHist, 39, 0, 128, 6)

// Futility pruning, in 1/1024
TUNABLE(FpHistMult, 19, 0, 64, 2.5)
TUNABLE(CapFpHistMult, 95, 0, 192, 8)

// History pruning
TUNABLE(HpDepth, 6, 2, 8, 0.5)
TUNABLE(HpMargin, 1420, 1024, 8192, 350)

// Late move reduction (LMR), in 1/1024
TUNABLE(LmrQuietHist, 236, 64, 512, 24)
TUNABLE(LmrCaptHist, 402, 64, 512, 24)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
