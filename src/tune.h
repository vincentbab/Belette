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

// Reverse futility pruning (RFP)
TUNABLE(RfpDepth, 8, 4, 12, 0.5)
TUNABLE(RfpMargin, 120, 60, 200, 7)
TUNABLE(RfpMarginImproving, 60, 20, 120, 5)

// Razoring
TUNABLE(RazorDepth, 2, 1, 4, 0.5)
TUNABLE(RazorMargin, 400, 200, 700, 25)

// Null move pruning (NMP)
TUNABLE(NmpDepth, 3, 2, 5, 0.5)
TUNABLE(NmpBase, 4, 2, 6, 0.5)
TUNABLE(NmpDepthMult, 256, 128, 512, 20)
TUNABLE(NmpEvalDiv, 200, 100, 400, 15)
TUNABLE(NmpEvalMax, 3, 1, 6, 0.5)

// ProbCut
TUNABLE(ProbCutDepth, 5, 3, 8, 0.5)
TUNABLE(ProbCutMargin, 180, 80, 300, 10)
TUNABLE(ProbCutImproving, 60, 0, 120, 6)
TUNABLE(ProbCutTtDepth, 3, 1, 5, 0.5)

// Move count pruning, in 1/1024
TUNABLE(LmpBase, 3072, 1024, 6144, 256)
TUNABLE(LmpImproving, 1024, 512, 2048, 64)
TUNABLE(LmpNotImproving, 512, 256, 1024, 40)

// Futility pruning
TUNABLE(FpLmrDepth, 6, 3, 10, 0.5)
TUNABLE(FpBase, 100, 30, 200, 8)
TUNABLE(FpDepthMargin, 120, 60, 200, 7)

// Captures futility pruning
TUNABLE(CapFpDepth, 8, 4, 12, 0.5)
TUNABLE(CapFpMargin, 100, 50, 200, 8)

// SEE Pruning
TUNABLE(SeeDepth, 8, 4, 12, 0.5)
TUNABLE(SeeTacticalMargin, 100, 40, 200, 8)
TUNABLE(SeeQuietMargin, 60, 20, 120, 5)

// Futility pruning history weights, in 1/1024
TUNABLE(FpHistMult, 19, 0, 64, 2.5)
TUNABLE(CapFpHistMult, 95, 0, 192, 8)

// History pruning
TUNABLE(HpDepth, 6, 2, 8, 0.5)
TUNABLE(HpMargin, 1420, 256, 8192, 350)

// Late move reduction (LMR), in 1/1024
TUNABLE(LmrQuietHist, 236, 64, 512, 24)
TUNABLE(LmrCaptHist, 402, 64, 512, 24)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
