#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include "chess.h"
#include "position.h"
#include "evaluate.h"
#include "move.h"

namespace BabChess {

struct SearchLimits {
    int timeLeft[NB_SIDE] = {0};
    int increment[NB_SIDE] = {0};
    int movesToGo = 0;
    int maxDepth = 0;
    int maxNodes = 0;
    int maxTime = 0;
};

struct SearchData {
    SearchData(const Position& pos_, const SearchLimits& limits_): position(pos_), limits(limits_), nodes(0) { }

    Position position;
    SearchLimits limits;
    size_t nodes;
};

struct SearchEvent {
    SearchEvent(int depth_, const MoveList &pv_, Score bestScore_): 
        depth(depth_), pv(pv_), bestScore(bestScore_) { }

    int depth;
    const MoveList &pv;
    Score bestScore;
};

class Engine {
public:
    Engine() = default;

    inline Position &position() { return rootPosition; }
    inline const Position &position() const { return rootPosition; }

    void search(const SearchLimits &limits);
    void stop();

    virtual void onSearchProgress(const SearchEvent &event) = 0;
    virtual void onSearchFinish(const SearchEvent &event) = 0;

private:
    Position rootPosition;
    bool stopSearch;

    inline void idSearch(SearchData &sd) { rootPosition.getSideToMove() == WHITE ? idSearch<WHITE>(sd) : idSearch<BLACK>(sd); }
    template<Side Me> void idSearch(SearchData &sd);

    template<Side Me> Score pvSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv);

    template<Side Me> Score qSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv);
};

} /* namespace BabChess */

#endif /* ENGINE_H_INCLUDED */
