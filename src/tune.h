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
TUNABLE(BonusMainMult, 145, 48, 256, 10)
TUNABLE(BonusMainBase, 73, 0, 256, 12)
TUNABLE(BonusMainMax, 1470, 512, 3072, 128)
TUNABLE(BonusContMult, 134, 48, 256, 10)
TUNABLE(BonusContBase, 91, 0, 256, 12)
TUNABLE(BonusContMax, 1453, 512, 3072, 128)
TUNABLE(BonusPawnMult, 165, 48, 256, 10)
TUNABLE(BonusPawnBase, 75, 0, 256, 12)
TUNABLE(BonusPawnMax, 1400, 512, 3072, 128)
TUNABLE(BonusCaptMult, 93, 48, 256, 10)
TUNABLE(BonusCaptBase, 59, 0, 256, 12)
TUNABLE(BonusCaptMax, 1268, 512, 3072, 128)

// History malus
TUNABLE(MalusMainMult, 112, 48, 256, 10)
TUNABLE(MalusMainBase, 0, 0, 256, 12)
TUNABLE(MalusMainMax, 1629, 512, 3072, 128)
TUNABLE(MalusContMult, 217, 48, 256, 10)
TUNABLE(MalusContBase, 6, 0, 256, 12)
TUNABLE(MalusContMax, 698, 512, 3072, 128)
TUNABLE(MalusPawnMult, 115, 48, 256, 10)
TUNABLE(MalusPawnBase, 98, 0, 256, 12)
TUNABLE(MalusPawnMax, 1551, 512, 3072, 128)
TUNABLE(MalusCaptMult, 147, 48, 256, 10)
TUNABLE(MalusCaptBase, 59, 0, 256, 12)
TUNABLE(MalusCaptMax, 1667, 512, 3072, 128)

// History gravity, in units of 1024
TUNABLE(GravityCont, 12, 4, 24, 1)
TUNABLE(GravityCapt, 7, 4, 24, 1)

// Quiet ordering weights, in 1/1024
TUNABLE(OrderMain, 1857, 512, 4096, 200)
TUNABLE(OrderCont1, 1106, 256, 2560, 100)
TUNABLE(OrderCont2, 1645, 256, 2560, 100)
TUNABLE(OrderCont4, 1146, 256, 2560, 100)
TUNABLE(OrderPawn, 2821, 512, 4096, 200)

// Evasion ordering weight, in 1/1024
TUNABLE(EvasionCont, 858, 256, 2560, 100)

// statScore weights, in 1/1024
TUNABLE(StatMain, 764, 512, 4096, 200)
TUNABLE(StatCont1, 1396, 256, 2560, 100)
TUNABLE(StatCont2, 1220, 256, 2560, 100)

// Move picker
TUNABLE(GoodQuietThreshold, -769, -16000, 8000, 1000)
TUNABLE(ThreatMinor, 11182, 0, 30000, 1500)
TUNABLE(ThreatRook, 26199, 0, 50000, 2500)
TUNABLE(ThreatQueen, 51975, 0, 100000, 5000)
TUNABLE(CheckBonus, 9124, 0, 20000, 1000)
TUNABLE(TacticalMvv, 9, 2, 20, 1)
TUNABLE(BadCaptureSee, 45, 0, 150, 8)
TUNABLE(BadCaptureHist, 40, 0, 128, 6)

// Reverse futility pruning (RFP)
TUNABLE(RfpDepth, 9, 4, 12, 0.5)
TUNABLE(RfpMargin, 119, 60, 200, 7)
TUNABLE(RfpMarginImproving, 46, 20, 120, 5)

// Razoring
TUNABLE(RazorDepth, 3, 1, 4, 0.5)
TUNABLE(RazorMargin, 344, 200, 700, 25)

// Null move pruning (NMP)
TUNABLE(NmpDepth, 5, 2, 5, 0.5)
TUNABLE(NmpBase, 5, 2, 6, 0.5)
TUNABLE(NmpDepthMult, 297, 128, 512, 20)
TUNABLE(NmpEvalDiv, 183, 100, 400, 15)
TUNABLE(NmpEvalMax, 3, 1, 6, 0.5)

// ProbCut
TUNABLE(ProbCutDepth, 4, 2, 8, 0.5)
TUNABLE(ProbCutMargin, 99, 80, 300, 10)
TUNABLE(ProbCutImproving, 54, 0, 120, 6)
TUNABLE(ProbCutTtDepth, 4, 1, 5, 0.5)

// Move count pruning, in 1/1024
TUNABLE(LmpBase, 5370, 1024, 6144, 256)
TUNABLE(LmpImproving, 1094, 512, 2048, 64)
TUNABLE(LmpNotImproving, 83, 32, 1024, 40)

// Futility pruning
TUNABLE(FpLmrDepth, 6, 3, 10, 0.5)
TUNABLE(FpBase, 93, 30, 200, 8)
TUNABLE(FpDepthMargin, 116, 60, 200, 7)

// Captures futility pruning
TUNABLE(CapFpDepth, 9, 4, 12, 0.5)
TUNABLE(CapFpMargin, 90, 50, 200, 8)

// SEE Pruning
TUNABLE(SeeDepth, 10, 4, 12, 0.5)
TUNABLE(SeeTacticalMargin, 87, 40, 200, 8)
TUNABLE(SeeQuietMargin, 30, 20, 120, 5)

// Futility pruning history weights, in 1/1024
TUNABLE(FpHistMult, 23, 0, 64, 2.5)
TUNABLE(CapFpHistMult, 112, 0, 192, 8)

// History pruning
TUNABLE(HpDepth, 8, 2, 10, 0.5)
TUNABLE(HpMargin, 377, 64, 2048, 100)
TUNABLE(PrunePawn, 1119, 0, 2048, 100)

// Aspiration window
TUNABLE(AspDepth, 4, 2, 8, 0.5)
TUNABLE(AspDelta, 6, 3, 32, 2)
TUNABLE(AspScoreDiv, 81, 40, 300, 10)
TUNABLE(AspGrowth, 438, 128, 1024, 40)

// Internal Iterative Reduction (IIR)
TUNABLE(IirDepth, 10, 2, 12, 0.5)

// Check extension
TUNABLE(CheckExtDepth, 1, 0, 5, 0.5)

// Singular extensions
TUNABLE(SeDepth, 4, 3, 10, 0.5)
TUNABLE(SeTtPvDepth, 1, 0, 3, 0.5)
TUNABLE(SeTtDepth, 3, 1, 6, 0.5)
TUNABLE(SeMargin, 1144, 256, 3072, 100)
TUNABLE(SeSearchDepth, 478, 256, 768, 30)
TUNABLE(SeCorrMult, 254, 0, 768, 30)
TUNABLE(SeDepthIncMax, 11, 8, 18, 0.5)
TUNABLE(SeDepthIncMargin, 4, -20, 40, 3)
TUNABLE(SeDoubleMax, 7, 2, 16, 0.5)
TUNABLE(SeTripleMargin, 62, 10, 150, 6)
TUNABLE(SeNegTt, 1, -2, 2, 0.5)
TUNABLE(SeNegCut, 1, -2, 2, 0.5)

// Quiescence search
TUNABLE(QsFpMargin, 89, 30, 200, 8)
TUNABLE(QsSeeMargin, 10, -50, 100, 6)

// Correction history weights, in 1/1024
TUNABLE(CorrPawn, 1556, 256, 2048, 80)
TUNABLE(CorrNonPawn, 1175, 256, 2048, 80)
TUNABLE(CorrMinor, 1293, 256, 2048, 80)
TUNABLE(CorrMajor, 1056, 256, 2048, 80)
TUNABLE(CorrContPly2, 1424, 0, 2560, 80)
TUNABLE(CorrContPly3, 1195, 0, 2560, 80)
TUNABLE(CorrContPly4, 988, 0, 2560, 80)
TUNABLE(CorrContPly5, 1064, 0, 2560, 80)
TUNABLE(CorrContPly6to8, 1282, 0, 2560, 80)
TUNABLE(CorrContPly9to11, 1397, 0, 2560, 80)
TUNABLE(CorrCont2Ply3, 1161, 0, 2560, 80)
TUNABLE(CorrCont2Ply4, 1020, 0, 2560, 80)
TUNABLE(CorrCont2Ply5, 1138, 0, 2560, 80)
TUNABLE(CorrCont2Ply6to8, 1262, 0, 2560, 80)
TUNABLE(CorrCont2Ply9to11, 1157, 0, 2560, 80)

// Correction history update
TUNABLE(CorrUpdateBase, 2, 0, 4, 0.5)
TUNABLE(CorrUpdateMax, 13, 8, 32, 1.5)
TUNABLE(CorrLimit, 37, 16, 64, 3)

// Late move reduction (LMR)
TUNABLE(LmrQuietBase, 944, 250, 1250, 50)
TUNABLE(LmrQuietScale, 436, 250, 700, 22)
TUNABLE(LmrTacticalBase, 134, -200, 800, 50)
TUNABLE(LmrTacticalScale, 283, 100, 500, 20)
TUNABLE(LmrPv, 1660, 0, 2560, 120)
TUNABLE(LmrInCheck, 1253, 0, 2560, 120)
TUNABLE(LmrNoTtPv, 1111, 0, 2560, 120)
TUNABLE(LmrTtTactical, 1364, 0, 2560, 120)
TUNABLE(LmrCutNode, 1986, 512, 4096, 180)
TUNABLE(LmrNotImproving, 1341, 0, 2560, 120)
TUNABLE(LmrChildCutoffs, 1537, 0, 2560, 120)
TUNABLE(LmrDeeperMargin, 29, 10, 150, 6)
TUNABLE(LmrShallowerMargin, 6, 0, 50, 3)

// LMR history weights, in 1/1024
TUNABLE(LmrQuietHist, 203, 64, 1024, 24)
TUNABLE(LmrCaptHist, 343, 64, 1024, 24)

// Time management
TUNABLE(TmHorizon, 50, 20, 80, 4)
TUNABLE(TmOptBase, 193, 40, 400, 16)
TUNABLE(TmOptGrowth, 27, 5, 80, 4)
TUNABLE(TmOptCap, 176, 100, 400, 16)
TUNABLE(TmMaxScale, 716, 300, 1000, 40)
TUNABLE(TmHardFrac, 770, 500, 950, 25)
TUNABLE(TmStabBase, 1397, 900, 1800, 50)
TUNABLE(TmStabSlope, 55, 20, 120, 6)
TUNABLE(TmInstab, 369, 0, 3000, 150)
TUNABLE(TmEffortBase, 1439, 1100, 2000, 50)
TUNABLE(TmEffortMul, 1299, 900, 1800, 50)
TUNABLE(TmFallPrev, 14, 0, 50, 3)
TUNABLE(TmFallIter, 6, 0, 50, 3)
TUNABLE(TmFallMin, 652, 300, 1000, 40)
TUNABLE(TmFallMax, 1840, 1000, 2500, 80)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
