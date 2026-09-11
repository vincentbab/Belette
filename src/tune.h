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

// Aspiration window
TUNABLE(AspDepth, 4, 1, 8, 0.5)
TUNABLE(AspDelta, 16, 6, 40, 2)
TUNABLE(AspScoreMult, 100, 0, 300, 15)
TUNABLE(AspGrowth, 512, 256, 1536, 64)

// Internal Iterative Reduction (IIR)
TUNABLE(IirDepth, 4, 2, 8, 0.5)

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

// Singular extensions
TUNABLE(SeDepth, 8, 4, 10, 0.5)
TUNABLE(SeTtDepthMargin, 3, 1, 5, 0.5)
TUNABLE(SeBetaMult, 32, 8, 64, 3)
TUNABLE(SeDoubleMargin, 20, 5, 50, 2.5)
TUNABLE(SeDoubleMax, 8, 4, 16, 0.5)

// Move count pruning
TUNABLE(LmpBase, 3072, 1024, 6144, 256)
TUNABLE(LmpImproving, 1024, 512, 2048, 64)
TUNABLE(LmpMult, 512, 256, 1024, 40)

// Futility pruning
TUNABLE(FpLmrDepth, 6, 3, 10, 0.5)
TUNABLE(FpBase, 100, 30, 200, 8)
TUNABLE(FpDepthMargin, 120, 60, 200, 7)
TUNABLE(FpHistMult, 64, 16, 128, 6)

// History pruning
TUNABLE(HpDepth, 4, 2, 6, 0.5)
TUNABLE(HpMargin, 4096, 1024, 8192, 350)

// SEE Pruning
TUNABLE(SeePruneDepth, 8, 4, 12, 0.5)
TUNABLE(SeeTacticalMargin, 100, 40, 200, 8)
TUNABLE(SeeQuietMargin, 60, 20, 120, 5)

// Late move reduction (LMR)
TUNABLE(LmrBase, 256, 0, 1024, 50)
TUNABLE(LmrScale, 460, 250, 700, 22)
TUNABLE(LmrPv, 1024, 0, 2048, 100)
TUNABLE(LmrInCheck, 1024, 0, 2048, 100)
TUNABLE(LmrNoTtPv, 1024, 0, 2048, 100)
TUNABLE(LmrTtTactical, 1024, 0, 2048, 100)
TUNABLE(LmrCutNode, 2048, 512, 3072, 128)
TUNABLE(LmrNotImproving, 1024, 0, 2048, 100)
TUNABLE(LmrHistMult, 1024, 256, 2048, 90)

// Quiescence
TUNABLE(QsFpMargin, 100, 30, 250, 10)
TUNABLE(QsSeeMargin, 0, -100, 0, 5)

// History
TUNABLE(HistBonusMult, 128, 48, 256, 10)
TUNABLE(HistBonusMax, 1536, 512, 3072, 128)
TUNABLE(HistMax, 8192, 4096, 16384, 600)
TUNABLE(HistMainMult, 1024, 512, 2048, 75)

// Correction history
TUNABLE(CorrWeightMax, 16, 8, 32, 1)

// Move ordering
TUNABLE(GoodQuietThreshold, -16000, -32000, -4000, 1400)
TUNABLE(BadCaptureSee, -50, -150, 0, 8)
TUNABLE(CheckBonus, 10000, 0, 20000, 1000)
TUNABLE(ThreatMinor, 15000, 0, 30000, 1500)
TUNABLE(ThreatRook, 25000, 0, 50000, 2500)
TUNABLE(ThreatQueen, 50000, 0, 100000, 5000)

} /* namespace Belette */

#endif /* TUNE_H_INCLUDED */
