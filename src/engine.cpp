#include <iostream>
#include <thread>
#include "engine.h"
#include "move.h"
#include "evaluate.h"
#include "movepicker.h"

using namespace std;

namespace BabChess {

void updatePv(MoveList &pv, Move move, const MoveList &childPv) {
    pv.clear();
    pv.push_back(move);
    // TODO: replace with pv.insert()
    for(auto m : childPv) {
        pv.push_back(m);
    }
}

SearchData::SearchData(const Position& pos_, const SearchLimits& limits_):
    position(pos_), limits(limits_), nbNodes(0)
{
    startTime = now();
    initAllocatedTime();
}

inline void SearchData::initAllocatedTime() {
    int64_t moves = limits.movesToGo > 0 ? limits.movesToGo : 40;
    Side stm = position.getSideToMove();

    allocatedTime = limits.timeLeft[stm] / moves + limits.increment[stm];
}

// Search entry point
void Engine::search(const SearchLimits &limits) {
    if (searching) return;

    SearchData data(position(), limits);
    aborted = false;
    searching = true;
    
    tt.clear(); // TODO: update age instead of clear

    std::thread th([this, data] { this->idSearch(data); });
    th.detach();
}

void Engine::stop() {
    aborted = true;
}

// Iterative deepening loop
template<Side Me>
void Engine::idSearch(SearchData sd) {
    MoveList bestPv;
    Score bestScore;
    int depth, completedDepth = 0;

    for (depth = 1; depth < MAX_PLY; depth++) {
        Score alpha = -SCORE_INFINITE, beta = SCORE_INFINITE;
        MoveList pv;

        Score score = pvSearch<Me, true>(sd, alpha, beta, depth, 0, pv);

        if (depth > 1 && searchAborted()) break;

        bestPv = pv;
        bestScore = score;
        completedDepth = depth;

        onSearchProgress(SearchEvent(depth, pv, bestScore, sd.nbNodes, sd.getElapsed(), tt.usage()));

        if (sd.limits.maxDepth > 0 && depth >= sd.limits.maxDepth) break;
    }

    SearchEvent event(depth, bestPv, bestScore, sd.nbNodes, sd.getElapsed(), tt.usage());
    if (depth != completedDepth)
        onSearchProgress(event);
    onSearchFinish(event);

    searching = false;
}

// Negamax search
template<Side Me, bool RootNode>
Score Engine::pvSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv) {
    if (depth <= 0) {
        return qSearch<Me>(sd, alpha, beta, depth, ply, pv);
    }

    pv.clear();

    // Check if we should stop according to limits
    if (!RootNode && sd.shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by limits) exit here
    if (!RootNode && searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    Score alphaOrig = alpha;
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = MOVE_NONE;
    Position &pos = sd.position;
    bool inCheck = pos.inCheck();

    if (pos.isFiftyMoveDraw() || pos.isMaterialDraw() || pos.isRepetitionDraw()) {
        // "Random" between [-2,1], avoid blindness to 3-fold repetitions
        return 1-(sd.nbNodes & 2);
    }

    if (ply >= MAX_PLY) [[unlikely]] {
        return evaluate<Me>(pos); // TODO: check if we are in check ?
    }

    MoveList childPv;

    auto&&[ttHit, tte] = tt.get(pos.hash());

    int nbMoves = 0;
    MovePicker<MAIN, Me> mp(pos, ttHit ? tte->move() : MOVE_NONE);
    
    mp.enumerate([&](Move move, auto doMove, auto undoMove) -> bool {
        // Honor UCI searchmoves
        if (RootNode && sd.limits.searchMoves.size() > 0 && !sd.limits.searchMoves.contains(move))
            return true; // continue

        nbMoves++;
        sd.nbNodes++;

        (pos.*doMove)(move);
        Score score = -pvSearch<~Me, false>(sd, -beta, -alpha, depth-1, ply+1, childPv);
        (pos.*undoMove)(move);

        if (searchAborted()) return false; // break

        if (score > bestScore) {
            bestScore = score;
            
            if (bestScore > alpha) {
                bestMove = move;
                alpha = bestScore;
                updatePv(pv, move, childPv);

                if (alpha >= beta) {
                    return false; // break
                }
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    // Checkmate / Stalemate detection
    if (nbMoves == 0) {
        return inCheck ? -SCORE_MATE + ply : SCORE_DRAW;
    }

    // Update TT
    Bound ttBound = bestScore >= beta      ? BOUND_LOWER : 
                    bestScore <= alphaOrig ? BOUND_UPPER : BOUND_EXACT;
    tt.set(tte, pos.hash(), depth, ply, ttBound, bestMove, SCORE_NONE, bestScore, false);

    return bestScore;
}

// Quiescence search
template<Side Me>
Score Engine::qSearch(SearchData &sd, Score alpha, Score beta, int depth, int ply, MoveList &pv) {
    pv.clear();

    // Check if we should stop according to limits
    if (sd.shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by limits) exit here
    if (searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    // Default bestScore for mate detection, if InCheck and there is no move this score will be returned
    Score bestScore = -SCORE_MATE + ply;
    Move bestMove = MOVE_NONE;
    Position &pos = sd.position;

    if (pos.isFiftyMoveDraw() || pos.isMaterialDraw() || pos.isRepetitionDraw()) {
        // "Random" between [-2,1], avoid blindness to 3-fold repetitions
        return 1-(sd.nbNodes & 2);
    }

    if (ply >= MAX_PLY) [[unlikely]] {
        return evaluate<Me>(pos); // TODO: check if we are in check ?
    }

    bool inCheck = pos.inCheck();

    // Standing Pat
    if (!inCheck) {
        Score eval = evaluate<Me>(pos);

        if (eval >= beta)
            return eval;

        if (eval > alpha)
            alpha = eval;

        bestScore = eval;
    }
    
    MoveList childPv;

    auto&&[ttHit, tte] = tt.get(pos.hash());

    int nbMoves = 0;
    MovePicker<QUIESCENCE, Me> mp(pos, ttHit ? tte->move() : MOVE_NONE);

    //bool stopped = !enumerateLegalMoves<Me, NON_QUIET_MOVES>(pos, [&](Move move, auto doMove, auto undoMove) -> bool {
    mp.enumerate([&](Move move, auto doMove, auto undoMove) -> bool {
        nbMoves++;
        sd.nbNodes++;

        (pos.*doMove)(move);
        Score score = -qSearch<~Me>(sd, -beta, -alpha, depth-1, ply+1, childPv);
        (pos.*undoMove)(move);

        if (searchAborted()) return false; // break

        if (score > bestScore) {
            bestScore = score;
            

            if (bestScore > alpha) {
                bestMove = move;
                alpha = bestScore;
                updatePv(pv, move, childPv);

                if (alpha >= beta) {
                    return false; // break
                }
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    Bound ttBound = bestScore >= beta ? BOUND_LOWER : BOUND_UPPER;
    tt.set(tte, pos.hash(), inCheck, ply, ttBound, bestMove, SCORE_NONE, bestScore, false);

    return bestScore;
}

} /* namespace BabChess */