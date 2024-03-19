#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include <memory>
#include "chess.h"
#include "position.h"
#include "evaluate.h"
#include "movegen.h"
#include "tt.h"
#include "utils.h"

namespace Belette {

struct SearchLimits {
    TimeMs timeLeft[NB_SIDE] = {0};
    TimeMs increment[NB_SIDE] = {0};
    int movesToGo = 0;
    int maxDepth = 0;
    size_t maxNodes = 0;
    TimeMs maxTime = 0;
    MoveList searchMoves;
};

struct SearchData {
    SearchData(const Position& pos_, const SearchLimits& limits_)
    : position(pos_), limits(limits_), nbNodes(0), killerMoves{MOVE_NONE}, counterMoves{MOVE_NONE} { 
        start();
    }

    void initAllocatedTime();

    inline TimeMs getElapsed() { return now() - startTime; }
    inline void start() {
        startTime = now();
        initAllocatedTime();
    }
    
    inline bool useTournamentTime() { return !!(limits.timeLeft[WHITE] | limits.timeLeft[WHITE]); }
    inline bool useFixedTime() { return limits.maxTime > 0; }
    inline bool useTimeLimit() { return useTournamentTime() || useTimeLimit(); }
    inline bool useNodeCountLimit() { return limits.maxNodes > 0; }

    inline bool shouldStop() {
        // Check time every 1024 nodes for performance reason
        if (nbNodes % 1024 != 0)  return false;
        
        TimeMs elapsed = now() - startTime;

        if (useTournamentTime() && elapsed >= allocatedTime)
            return true;
        if (useFixedTime() && (elapsed > limits.maxTime))
            return true;
        if (useNodeCountLimit() && nbNodes >= limits.maxNodes)
            return true;
        
        return false;
    }

    inline void clearKillers(int ply) {
        assert(ply < MAX_PLY);
        killerMoves[ply][0] = killerMoves[ply][1] = MOVE_NONE;
    }

    inline void updateKillers(Move move, int ply) {
        assert(ply < MAX_PLY);

        if (killerMoves[ply][0] != move) {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;
        }
    }

    inline void updateCounter(Move move) {
        Move prevMove = position.previousMove();
        if (isValidMove(prevMove))
            counterMoves[position.getPieceAt(moveTo(prevMove))][moveTo(prevMove)] = move;
    }

    inline Move getCounter() const {
        Move prevMove = position.previousMove();
        if (prevMove == MOVE_NONE) return MOVE_NONE;

        return counterMoves[position.getPieceAt(moveTo(prevMove))][moveTo(prevMove)];
    }

    Position position;
    SearchLimits limits;
    size_t nbNodes;
    int selDepth;

    TimeMs startTime;
    TimeMs lastCheck;
    TimeMs allocatedTime;

    Move killerMoves[MAX_PLY][2];
    Move counterMoves[NB_PIECE][NB_SQUARE];
};

struct SearchEvent {
    SearchEvent(int depth_, int selDepth_, const MoveList &pv_, Score bestScore_, size_t nbNode_, TimeMs elapsed_, size_t hashfull_): 
        depth(depth_), selDepth(selDepth_), pv(pv_), bestScore(bestScore_), nbNodes(nbNode_), elapsed(elapsed_), hashfull(hashfull_) { }

    int depth;
    int selDepth;
    const MoveList &pv;
    Score bestScore;
    size_t nbNodes;
    TimeMs elapsed;
    size_t hashfull;
};

enum class NodeType {
    Root,
    PV,
    NonPV
};

class Engine {
public:
    Engine() = default;
    virtual ~Engine() = default;

    inline Position &position() { return rootPosition; }
    inline const Position &position() const { return rootPosition; }

    void search(const SearchLimits &limits);
    void stop();
    void waitForSearchFinish();
    inline bool isSearching() { return searching; }
    inline bool searchAborted() { return aborted; }
    inline void setHashSize(size_t size) { tt.resize(size); }
    inline void newGame() { tt.clear(); }

protected:
    virtual void onSearchProgress(const SearchEvent &event) = 0;
    virtual void onSearchFinish(const SearchEvent &event) = 0;

private:
    std::unique_ptr<SearchData> sd;
    Position rootPosition;
    TranspositionTable tt;
    bool aborted = true;
    bool searching = false;

    inline void idSearch() { rootPosition.getSideToMove() == WHITE ? idSearch<WHITE>() : idSearch<BLACK>(); }
    template<Side Me> void idSearch();

    template<Side Me, NodeType NT> Score pvSearch(Score alpha, Score beta, int depth, int ply, MoveList &pv);

    template<Side Me, NodeType NT> Score qSearch(Score alpha, Score beta, int depth, int ply, MoveList &pv);
};

} /* namespace Belette */

#endif /* ENGINE_H_INCLUDED */
