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
TUNABLE(RfpDepth, 9, 4, 12, 0.5)
TUNABLE(RfpMargin, 108, 60, 200, 7)
TUNABLE(RfpMarginImproving, 54, 20, 120, 5)

// Razoring
TUNABLE(RazorDepth, 3, 1, 4, 0.5)
TUNABLE(RazorMargin, 326, 200, 700, 25)

// Null move pruning (NMP)
TUNABLE(NmpDepth, 4, 2, 5, 0.5)
TUNABLE(NmpBase, 5, 2, 6, 0.5)
TUNABLE(NmpDepthMult, 272, 128, 512, 20)
TUNABLE(NmpEvalDiv, 185, 100, 400, 15)
TUNABLE(NmpEvalMax, 3, 1, 6, 0.5)

// ProbCut
TUNABLE(ProbCutDepth, 3, 2, 8, 0.5)
TUNABLE(ProbCutMargin, 106, 80, 300, 10)
TUNABLE(ProbCutImproving, 75, 0, 120, 6)
TUNABLE(ProbCutTtDepth, 4, 1, 5, 0.5)

// Move count pruning, in 1/1024
TUNABLE(LmpBase, 4440, 1024, 6144, 256)
TUNABLE(LmpImproving, 1030, 512, 2048, 64)
TUNABLE(LmpNotImproving, 131, 128, 1024, 40)

// Futility pruning
TUNABLE(FpLmrDepth, 6, 3, 10, 0.5)
TUNABLE(FpBase, 91, 30, 200, 8)
TUNABLE(FpDepthMargin, 119, 60, 200, 7)

// Captures futility pruning
TUNABLE(CapFpDepth, 9, 4, 12, 0.5)
TUNABLE(CapFpMargin, 82, 50, 200, 8)

// SEE Pruning
TUNABLE(SeeDepth, 10, 4, 12, 0.5)
TUNABLE(SeeTacticalMargin, 76, 40, 200, 8)
TUNABLE(SeeQuietMargin, 40, 20, 120, 5)

// Futility pruning history weights, in 1/1024
TUNABLE(FpHistMult, 23, 0, 64, 2.5)
TUNABLE(CapFpHistMult, 101, 0, 192, 8)

// History pruning
TUNABLE(HpDepth, 7, 2, 10, 0.5)
TUNABLE(HpMargin, 353, 256, 8192, 350)

// Aspiration window
TUNABLE(AspDepth, 4, 2, 8, 0.5)
TUNABLE(AspDelta, 7, 3, 32, 2)
TUNABLE(AspScoreDiv, 102, 40, 300, 10)
TUNABLE(AspGrowth, 483, 128, 1024, 40)

// Internal Iterative Reduction (IIR)
TUNABLE(IirDepth, 10, 2, 12, 0.5)

// Check extension
TUNABLE(CheckExtDepth, 1, 0, 5, 0.5)

// Singular extensions
TUNABLE(SeDepth, 4, 3, 10, 0.5)
TUNABLE(SeTtPvDepth, 1, 0, 3, 0.5)
TUNABLE(SeTtDepth, 3, 1, 6, 0.5)
TUNABLE(SeMargin, 1150, 256, 3072, 100)
TUNABLE(SeSearchDepth, 475, 256, 768, 30)
TUNABLE(SeCorrMult, 263, 0, 768, 30)
TUNABLE(SeDepthIncMax, 11, 8, 18, 0.5)
TUNABLE(SeDepthIncMargin, 2, -20, 40, 3)
TUNABLE(SeDoubleMax, 7, 2, 16, 0.5)
TUNABLE(SeTripleMargin, 63, 10, 150, 6)
TUNABLE(SeNegTt, 1, -2, 2, 0.5)
TUNABLE(SeNegCut, 1, -2, 2, 0.5)

// Quiescence search
TUNABLE(QsFpMargin, 95, 30, 200, 8)
TUNABLE(QsSeeMargin, -5, -50, 100, 6)

// Correction history weights, in 1/1024
TUNABLE(CorrPawn, 1280, 256, 2048, 80)
TUNABLE(CorrNonPawn, 1150, 256, 2048, 80)
TUNABLE(CorrMinor, 1170, 256, 2048, 80)
TUNABLE(CorrMajor, 1150, 256, 2048, 80)
TUNABLE(CorrContPly2, 1260, 0, 2560, 80)
TUNABLE(CorrContPly3, 1150, 0, 2560, 80)
TUNABLE(CorrContPly4, 1110, 0, 2560, 80)
TUNABLE(CorrContPly5, 1160, 0, 2560, 80)
TUNABLE(CorrContPly6to8, 1250, 0, 2560, 80)
TUNABLE(CorrContPly9to11, 1200, 0, 2560, 80)
TUNABLE(CorrCont2Ply3, 1180, 0, 2560, 80)
TUNABLE(CorrCont2Ply4, 1030, 0, 2560, 80)
TUNABLE(CorrCont2Ply5, 934, 0, 2560, 80)
TUNABLE(CorrCont2Ply6to8, 1050, 0, 2560, 80)
TUNABLE(CorrCont2Ply9to11, 1050, 0, 2560, 80)

// Correction history update
TUNABLE(CorrUpdateBase, 2, 0, 4, 0.5)
TUNABLE(CorrUpdateMax, 13, 8, 32, 1.5)
TUNABLE(CorrLimit, 30, 16, 64, 3)

// Late move reduction (LMR)
TUNABLE(LmrQuietBase, 874, 250, 1250, 50)
TUNABLE(LmrQuietScale, 424, 250, 700, 22)
TUNABLE(LmrTacticalBase, 208, -200, 800, 50)
TUNABLE(LmrTacticalScale, 280, 100, 500, 20)
TUNABLE(LmrPv, 1280, 0, 2560, 120)
TUNABLE(LmrInCheck, 1130, 0, 2560, 120)
TUNABLE(LmrNoTtPv, 1020, 0, 2560, 120)
TUNABLE(LmrTtTactical, 1240, 0, 2560, 120)
TUNABLE(LmrCutNode, 1910, 512, 4096, 180)
TUNABLE(LmrNotImproving, 1020, 0, 2560, 120)
TUNABLE(LmrChildCutoffs, 1350, 0, 2560, 120)

// LMR history weights, in 1/1024
TUNABLE(LmrQuietHist, 226, 64, 1024, 24)
TUNABLE(LmrCaptHist, 376, 64, 1024, 24)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
