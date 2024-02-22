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
    SearchData(const Position& pos_, const SearchLimits& limits_): position(pos_), limits(limits_), nbNode(0) { }

    Position position;
    SearchLimits limits;
    size_t nbNode;
};

struct SearchEvent {
    SearchEvent(int depth_, const MoveList &pv_, Score bestScore_, size_t nbNode_): 
        depth(depth_), pv(pv_), bestScore(bestScore_), nbNode(nbNode_) { }

    int depth;
    const MoveList &pv;
    Score bestScore;
    size_t nbNode;
};

class Engine {
public:
    Engine() = default;
    virtual ~Engine() = default;

    inline Position &position() { return rootPosition; }
    inline const Position &position() const { return rootPosition; }

    void search(const SearchLimits &limits);
    void stop();
    inline bool isSearching() { return searching; }
    inline bool searchAborted() { return aborted; }

    virtual void onSearchProgress(const SearchEvent &event) = 0;
    virtual void onSearchFinish(const SearchEvent &event) = 0;

private:
    Position rootPosition;
    bool aborted = true;
    bool searching = false;

    inline void idSearch(SearchData sd) { rootPosition.getSideToMove() == WHITE ? idSearch<WHITE>(sd) : idSearch<BLACK>(sd); }
    template<Side Me> void idSearch(SearchData sd);

    template<Side Me> Score pvSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv);

    template<Side Me> Score qSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv);
};

} /* namespace BabChess */

#endif /* ENGINE_H_INCLUDED */
