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
TUNABLE(BonusMainMult, 133, 48, 256, 10)
TUNABLE(BonusMainBase, 88, 0, 256, 12)
TUNABLE(BonusMainMax, 1309, 512, 3072, 128)
TUNABLE(BonusContMult, 130, 48, 256, 10)
TUNABLE(BonusContBase, 117, 0, 256, 12)
TUNABLE(BonusContMax, 1503, 512, 3072, 128)
TUNABLE(BonusPawnMult, 153, 48, 256, 10)
TUNABLE(BonusPawnBase, 69, 0, 256, 12)
TUNABLE(BonusPawnMax, 1286, 512, 3072, 128)
TUNABLE(BonusCaptMult, 97, 48, 256, 10)
TUNABLE(BonusCaptBase, 55, 0, 256, 12)
TUNABLE(BonusCaptMax, 1196, 512, 3072, 128)

// History malus
TUNABLE(MalusMainMult, 118, 48, 256, 10)
TUNABLE(MalusMainBase, 13, 0, 256, 12)
TUNABLE(MalusMainMax, 1632, 512, 3072, 128)
TUNABLE(MalusContMult, 225, 48, 256, 10)
TUNABLE(MalusContBase, 6, 0, 256, 12)
TUNABLE(MalusContMax, 531, 512, 3072, 128)
TUNABLE(MalusPawnMult, 111, 48, 256, 10)
TUNABLE(MalusPawnBase, 87, 0, 256, 12)
TUNABLE(MalusPawnMax, 1540, 512, 3072, 128)
TUNABLE(MalusCaptMult, 154, 48, 256, 10)
TUNABLE(MalusCaptBase, 53, 0, 256, 12)
TUNABLE(MalusCaptMax, 1555, 512, 3072, 128)

// History gravity, in units of 1024
TUNABLE(GravityCont, 14, 4, 24, 1)
TUNABLE(GravityCapt, 8, 4, 24, 1)

// Quiet ordering weights, in 1/1024
TUNABLE(OrderMain, 1915, 512, 4096, 200)
TUNABLE(OrderCont1, 1324, 256, 2560, 100)
TUNABLE(OrderCont2, 1637, 256, 2560, 100)
TUNABLE(OrderCont4, 1114, 256, 2560, 100)
TUNABLE(OrderPawn, 2849, 512, 4096, 200)

// Evasion ordering weight, in 1/1024
TUNABLE(EvasionCont, 890, 256, 2560, 100)

// statScore weights, in 1/1024
TUNABLE(StatMain, 644, 512, 4096, 200)
TUNABLE(StatCont1, 1340, 256, 2560, 100)
TUNABLE(StatCont2, 1290, 256, 2560, 100)

// Move picker
TUNABLE(GoodQuietThreshold, -1478, -16000, 8000, 1000)
TUNABLE(ThreatMinor, 11010, 0, 30000, 1500)
TUNABLE(ThreatRook, 28343, 0, 50000, 2500)
TUNABLE(ThreatQueen, 51436, 0, 100000, 5000)
TUNABLE(CheckBonus, 9905, 0, 20000, 1000)
TUNABLE(TacticalMvv, 10, 2, 20, 1)
TUNABLE(BadCaptureSee, 49, 0, 150, 8)
TUNABLE(BadCaptureHist, 42, 0, 128, 6)

// Reverse futility pruning (RFP)
TUNABLE(RfpDepth, 9, 4, 12, 0.5)
TUNABLE(RfpMargin, 113, 60, 200, 7)
TUNABLE(RfpMarginImproving, 51, 20, 120, 5)

// Razoring
TUNABLE(RazorDepth, 4, 1, 4, 0.5)
TUNABLE(RazorMargin, 351, 200, 700, 25)

// Null move pruning (NMP)
TUNABLE(NmpDepth, 5, 2, 5, 0.5)
TUNABLE(NmpBase, 5, 2, 6, 0.5)
TUNABLE(NmpDepthMult, 317, 128, 512, 20)
TUNABLE(NmpEvalDiv, 169, 100, 400, 15)
TUNABLE(NmpEvalMax, 4, 1, 6, 0.5)

// ProbCut
TUNABLE(ProbCutDepth, 3, 2, 8, 0.5)
TUNABLE(ProbCutMargin, 90, 80, 300, 10)
TUNABLE(ProbCutImproving, 55, 0, 120, 6)
TUNABLE(ProbCutTtDepth, 4, 1, 5, 0.5)

// Move count pruning, in 1/1024
TUNABLE(LmpBase, 5581, 1024, 6144, 256)
TUNABLE(LmpImproving, 1014, 512, 2048, 64)
TUNABLE(LmpNotImproving, 103, 32, 1024, 40)

// Futility pruning
TUNABLE(FpLmrDepth, 6, 3, 10, 0.5)
TUNABLE(FpBase, 79, 30, 200, 8)
TUNABLE(FpDepthMargin, 119, 60, 200, 7)

// Captures futility pruning
TUNABLE(CapFpDepth, 10, 4, 12, 0.5)
TUNABLE(CapFpMargin, 87, 50, 200, 8)

// SEE Pruning
TUNABLE(SeeDepth, 11, 4, 12, 0.5)
TUNABLE(SeeTacticalMargin, 95, 40, 200, 8)
TUNABLE(SeeQuietMargin, 36, 20, 120, 5)

// Futility pruning history weights, in 1/1024
TUNABLE(FpHistMult, 26, 0, 64, 2.5)
TUNABLE(CapFpHistMult, 111, 0, 192, 8)

// History pruning
TUNABLE(HpDepth, 9, 2, 10, 0.5)
TUNABLE(HpMargin, 405, 64, 2048, 100)
TUNABLE(PrunePawn, 1080, 0, 2048, 100)

// Aspiration window
TUNABLE(AspDepth, 3, 2, 8, 0.5)
TUNABLE(AspDelta, 5, 3, 32, 2)
TUNABLE(AspScoreDiv, 74, 40, 300, 10)
TUNABLE(AspGrowth, 401, 128, 1024, 40)

// Internal Iterative Reduction (IIR)
TUNABLE(IirDepth, 10, 2, 12, 0.5)

// Check extension
TUNABLE(CheckExtDepth, 1, 0, 5, 0.5)

// Singular extensions
TUNABLE(SeDepth, 4, 3, 10, 0.5)
TUNABLE(SeTtPvDepth, 1, 0, 3, 0.5)
TUNABLE(SeTtDepth, 3, 1, 6, 0.5)
TUNABLE(SeMargin, 1127, 256, 3072, 100)
TUNABLE(SeSearchDepth, 480, 256, 768, 30)
TUNABLE(SeCorrMult, 311, 0, 768, 30)
TUNABLE(SeDepthIncMax, 11, 8, 18, 0.5)
TUNABLE(SeDepthIncMargin, 5, -20, 40, 3)
TUNABLE(SeDoubleMax, 6, 2, 16, 0.5)
TUNABLE(SeTripleMargin, 59, 10, 150, 6)
TUNABLE(SeNegTt, 1, -2, 2, 0.5)
TUNABLE(SeNegCut, 1, -2, 2, 0.5)

// Quiescence search
TUNABLE(QsFpMargin, 75, 30, 200, 8)
TUNABLE(QsSeeMargin, 16, -50, 100, 6)

// Correction history weights, in 1/1024
TUNABLE(CorrPawn, 1731, 256, 2048, 80)
TUNABLE(CorrNonPawn, 1183, 256, 2048, 80)
TUNABLE(CorrMinor, 1283, 256, 2048, 80)
TUNABLE(CorrMajor, 1153, 256, 2048, 80)
TUNABLE(CorrContPly2, 1518, 0, 2560, 80)
TUNABLE(CorrContPly3, 1148, 0, 2560, 80)
TUNABLE(CorrContPly4, 994, 0, 2560, 80)
TUNABLE(CorrContPly5, 1161, 0, 2560, 80)
TUNABLE(CorrContPly6to8, 1307, 0, 2560, 80)
TUNABLE(CorrContPly9to11, 1534, 0, 2560, 80)
TUNABLE(CorrCont2Ply3, 1134, 0, 2560, 80)
TUNABLE(CorrCont2Ply4, 979, 0, 2560, 80)
TUNABLE(CorrCont2Ply5, 1162, 0, 2560, 80)
TUNABLE(CorrCont2Ply6to8, 1452, 0, 2560, 80)
TUNABLE(CorrCont2Ply9to11, 1246, 0, 2560, 80)

// Correction history update
TUNABLE(CorrUpdateBase, 1, 0, 4, 0.5)
TUNABLE(CorrUpdateMax, 11, 8, 32, 1.5)
TUNABLE(CorrLimit, 35, 16, 64, 3)

// Late move reduction (LMR)
TUNABLE(LmrQuietBase, 922, 250, 1250, 50)
TUNABLE(LmrQuietScale, 442, 250, 700, 22)
TUNABLE(LmrTacticalBase, 172, -200, 800, 50)
TUNABLE(LmrTacticalScale, 237, 100, 500, 20)
TUNABLE(LmrPv, 1582, 0, 2560, 120)
TUNABLE(LmrInCheck, 1286, 0, 2560, 120)
TUNABLE(LmrNoTtPv, 1048, 0, 2560, 120)
TUNABLE(LmrTtTactical, 1436, 0, 2560, 120)
TUNABLE(LmrCutNode, 2031, 512, 4096, 180)
TUNABLE(LmrNotImproving, 1381, 0, 2560, 120)
TUNABLE(LmrChildCutoffs, 1731, 0, 2560, 120)
TUNABLE(LmrDeeperMargin, 27, 10, 150, 6)
TUNABLE(LmrShallowerMargin, 6, 0, 50, 3)

// LMR history weights, in 1/1024
TUNABLE(LmrQuietHist, 203, 64, 1024, 24)
TUNABLE(LmrCaptHist, 330, 64, 1024, 24)

// Time management
TUNABLE(TmHorizon, 51, 20, 80, 4)
TUNABLE(TmOptBase, 213, 40, 400, 16)
TUNABLE(TmOptGrowth, 27, 5, 80, 4)
TUNABLE(TmOptCap, 193, 100, 400, 16)
TUNABLE(TmMaxScale, 670, 300, 1000, 40)
TUNABLE(TmHardFrac, 771, 500, 950, 25)
TUNABLE(TmStabBase, 1360, 900, 1800, 50)
TUNABLE(TmStabSlope, 61, 20, 120, 6)
TUNABLE(TmInstab, 311, 0, 3000, 150)
TUNABLE(TmEffortBase, 1438, 1100, 2000, 50)
TUNABLE(TmEffortMul, 1340, 900, 1800, 50)
TUNABLE(TmFallPrev, 10, 0, 50, 3)
TUNABLE(TmFallIter, 7, 0, 50, 3)
TUNABLE(TmFallMin, 636, 300, 1000, 40)
TUNABLE(TmFallMax, 1861, 1000, 2500, 80)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
