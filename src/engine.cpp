#include <iostream>
#include "engine.h"
#include "move.h"
#include "evaluate.h"

using namespace std;

namespace BabChess {

// Search entry point
void Engine::search(const SearchLimits &limits) {
    SearchData data(position(), limits);

    // TODO: setup threads

    idSearch(data);
}

void Engine::stop() {
    stopSearch = true;
}

// Iterative deepening loop
template<Side Me>
void Engine::idSearch(SearchData &sd) {

    const int MaxDepth = sd.limits.maxDepth > 0 ? std::min(MAX_PLY, sd.limits.maxDepth) : MAX_PLY;

    MoveList bestPv;
    Score bestScore;
    int completedDepth = 0;

    for (int depth = 1; depth <= MaxDepth; depth++) {
        Score alpha = -SCORE_INFINITE, beta = SCORE_INFINITE;
        MoveList pv;

        Score score = pvSearch<Me>(sd, alpha, beta, depth, 0, pv);

        completedDepth = depth;
        bestPv = pv;
        bestScore = score;

        onSearchProgress(SearchEvent(depth, pv, bestScore));
    }

    onSearchFinish(SearchEvent(completedDepth, bestPv, bestScore));
}

void updatePv(MoveList &pv, Move move, const MoveList &childPv) {
    pv.clear();
    pv.push_back(move);
    // TODO: replace with pv.insert()
    for(auto m : childPv) {
        pv.push_back(m);
    }
}

// Root move search
template<Side Me>
Score Engine::pvSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv) {
    if (depth <= 0) {
        return qSearch<Me>(sd, alpha, beta, depth, ply, pv);
    }

    // TODO: check draw with : 50move, 3-fold, insufficient material

    Score bestScore = -SCORE_INFINITE;
    Position &pos = sd.position;

    MoveList childPv;

    int nbMove = 0;
    enumerateLegalMoves<Me>(pos, [&](Move move, auto doMove, auto undoMove) -> bool {
        nbMove++;

        (pos.*doMove)(move);
        Score score = -pvSearch<~Me>(sd, -beta, -alpha, depth-1, ply+1, childPv);
        (pos.*undoMove)(move);

        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                alpha = score;
                updatePv(pv, move, childPv);

                if (alpha >= beta) {
                    return false;
                }
            }
        }

        return true;
    });

    // Checkmate / Stalemate detection
    if (nbMove == 0) {
        return pos.inCheck() ? -SCORE_MATE + ply : SCORE_DRAW;
    }

    return bestScore;
}

// Quiescence search
template<Side Me>
Score Engine::qSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv) {
    Score bestScore = -SCORE_MATE + ply;
    Position &pos = sd.position;
    
    // TODO: check draw with : 50move, 3-fold, insufficient material

    Score eval = evaluate<Me>(pos);

    // Standing Pat
    if (!pos.inCheck()) {
        if (eval >= beta)
            return eval;

        if (eval > alpha)
            alpha = eval;

        bestScore = eval;
    }


    /*if (depth < 0)
        cout << "d=" << depth << " " << pos.debugHistory() << endl;*/
    
    MoveList childPv;

    int nbMove = 0;
    enumerateLegalMoves<Me, NON_QUIET_MOVES>(pos, [&](Move move, auto doMove, auto undoMove) -> bool {
        nbMove++;

        (pos.*doMove)(move);
        Score score = -qSearch<~Me>(sd, -beta, -alpha, depth-1, ply+1, childPv);
        (pos.*undoMove)(move);

        if (score > bestScore) {
            bestScore = score;

            if (score > alpha) {
                alpha = score;
                updatePv(pv, move, childPv);

                if (alpha >= beta) {
                    return false;
                }
            }
        }

        return true;
    });

    return bestScore;
}

} /* namespace BabChess */