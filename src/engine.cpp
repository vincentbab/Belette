#include <iostream>
#include <thread>
#include <cmath>
#include "engine.h"
#include "movegen.h"
#include "evaluate.h"
#include "movepicker.h"

namespace Belette {

constexpr int LMR_FACTOR = 1024;

int Engine::LMRTable[2][MAX_PLY][MAX_MOVE];

void Engine::init() {
    for (int d=1; d<MAX_PLY; d++) {
        for (int m=1; m<MAX_MOVE; m++) {
            LMRTable[0][d][m] = int(LMR_FACTOR * (LmrQuietBase / 1000.0 + LmrQuietScale / 1000.0 * std::log(d) * std::log(m)));
            LMRTable[1][d][m] = int(LMR_FACTOR * (LmrTacticalBase / 1000.0 + LmrTacticalScale / 1000.0 * std::log(d) * std::log(m)));
        }
    }
}

void updatePv(PvList &pv, Move move, const PvList &childPv) {
    pv.clear();
    pv.push_back(move);
    pv.insert(childPv.begin(), childPv.end());
}

void SearchData::initAllocatedTime() {
    int64_t moves = limits.movesToGo > 0 ? limits.movesToGo + 5 : 30;
    Side stm = position.getSideToMove();

    TimeMs timeLeft = std::max<TimeMs>(1, limits.timeLeft[stm] - limits.moveOverhead);

    const TimeMs maxTime = 0.49 * timeLeft;
    softTimeLimit = std::min<TimeMs>(maxTime, timeLeft / moves + 0.9 * limits.increment[stm]);
    hardTimeLimit = std::min<TimeMs>(maxTime, 5 * softTimeLimit);
}

Engine::~Engine() {
    abortSearch();
}

void Engine::waitForSearchFinish() {
    if (searchThread.joinable()) {
        searchThread.join();
    }
}

// Search entry point
void Engine::search(const SearchLimits &limits) {
    abortSearch();

    moveHistory.clearAllKillers();

    sd = std::make_unique<SearchData>(position(), limits, moveHistory);
    aborted = false;
    searching = true;
    
    tt.newSearch();

    searchThread = std::thread([this] {
        this->idSearch();
    });
}

void Engine::stop() {
    aborted = true;
}

void Engine::ponderHit() {
    if (searching && sd) {
        sd->timeOrigin = now();
        sd->pondering = false;
    }
}

void Engine::abortSearch() {
    stop();
    waitForSearchFinish();
}

// Iterative deepening loop
template<Side Me, bool Frc>
void Engine::idSearch() {
    PvList bestPv;
    Score bestScore = SCORE_DRAW;
    int depth, searchDepth, completedDepth = 0;

    MoveList rootMoves;
    generateLegalMoves<Frc>(sd->position, rootMoves);

    for (depth = 1; depth < MAX_PLY; depth++) {
        Score alpha = -SCORE_INFINITE, beta = SCORE_INFINITE;
        Score delta = 0, score = -SCORE_INFINITE;

        // Reset selDepth
        sd->selDepth = 0;

        searchDepth = depth;

        // Aspiration window
        if (depth > AspDepth) {
            delta = AspDelta + std::abs(bestScore)/AspScoreDiv;
            alpha = std::max(-SCORE_INFINITE, bestScore - delta);
            beta  = std::min( SCORE_INFINITE, bestScore + delta);
        }

        while (true) {
            if (alpha < -SCORE_MATE_MAX_PLY) alpha = -SCORE_INFINITE;
            if (beta > SCORE_MATE_MAX_PLY) beta = SCORE_INFINITE;
            //std::cout << "  depth=" << searchDepth << " d=" << delta << std::endl;
            score = pvSearch<Me, NodeType::Root, Frc>(alpha, beta, searchDepth, 0, false);

            if (searchAborted()) break;

            if (score <= alpha) { // Fail low
                //std::cout << "  Fail Low: a=" << alpha << " b=" << beta << " score=" << score << std::endl;
                beta = (alpha + beta) / 2;
                alpha = std::max(score - delta, -SCORE_INFINITE);
                searchDepth = depth;
            } else if (score >= beta) { // Fail high
                //std::cout << "  Fail High: a=" << alpha << " b=" << beta << " score=" << score << std::endl;
                beta = std::min(score + delta, SCORE_INFINITE);
                //searchDepth = std::max(std::max(1, depth - 4), searchDepth - 1);
                searchDepth = std::max(1, searchDepth - (std::abs(score) < 1000));
            } else {
                break;
            }

            delta += delta * AspGrowth / 1024;
        }

        if (searchAborted() && (depth > 1 || score == -SCORE_INFINITE)) break;

        bestPv = sd->node(0).pv;
        const Score prevScore = bestScore;
        bestScore = score;
        completedDepth = depth;

        // Best move stability
        if (bestPv.size() > 0) {
            if (bestPv.front() == sd->lastBestMove)
                sd->bestMoveStability = std::min(sd->bestMoveStability + 1, 8);
            else
                sd->bestMoveStability = 0;

            sd->lastBestMove = bestPv.front();
            sd->softScale = 1.3 - 0.069 * sd->bestMoveStability;

            // Best move effort
            if (depth >= 5) {
                double effort = double(sd->rootMoveNodes[moveFromTo(bestPv.front())]) / double(sd->nbNodes);
                sd->softScale *= (1.5 - effort) * 1.35;
            }

            // Score trend
            if (depth >= 5) {
                sd->softScale *= std::clamp(1.0 + 0.02 * (prevScore - score), 1.0, 1.4);
            }
        }

        onSearchProgress(SearchEvent(depth, sd->selDepth, bestPv, bestScore, sd->nbNodes, sd->getElapsed(), tt.usage()));

        if (sd->limits.maxDepth > 0 && depth >= sd->limits.maxDepth) break;

        if (sd->limits.mate > 0 && bestScore >= SCORE_MATE_MAX_PLY && SCORE_MATE - bestScore <= 2 * sd->limits.mate) break;

        // Instamove
        if (sd->useTournamentTime() && !sd->pondering && rootMoves.size() == 1) break;

        if (sd->shouldStopSoft()) break;
    }

    SearchEvent event(depth, sd->selDepth, bestPv, bestScore, sd->nbNodes, sd->getElapsed(), tt.usage());

    if (depth != completedDepth) {
        event.depth = completedDepth;
        onSearchProgress(event);
    }

    while ((sd->pondering || sd->limits.infinite) && !searchAborted())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    onSearchFinish(event);

    searching = false;
}

// Negamax search
template<Side Me, NodeType NT, bool Frc>
Score Engine::pvSearch(Score alpha, Score beta, int depth, int ply, bool cutNode) {
    constexpr bool PvNode = (NT != NodeType::NonPV);
    constexpr bool RootNode = (NT == NodeType::Root);
    constexpr NodeType QNodeType = PvNode ? NodeType::PV : NodeType::NonPV;

    // Quiescence
    if (depth <= 0) {
        return qSearch<Me, QNodeType, Frc>(alpha, beta, ply);
    }

    // Update selDepth
    if (PvNode && sd->selDepth < ply + 1) {
        sd->selDepth = ply + 1;
    }

    // Check if we should stop according to limits
    if (!RootNode && sd->shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by reaching limits) exit here
    if (!RootNode && searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    // Mate distance pruning
    if (!RootNode) {
        alpha = std::max(alpha, -SCORE_MATE + ply);
        beta  = std::min(beta, SCORE_MATE - ply - 1);

        if (alpha >= beta) return alpha;
    }

    Position &pos = sd->position;

    if (!RootNode && pos.isDraw(ply)) {
        return scoreDraw(sd->nbNodes);
    }

    if (!RootNode && alpha < SCORE_DRAW && pos.hasUpcomingRepetition(ply)) {
        alpha = scoreDraw(sd->nbNodes);
        if (alpha >= beta)
            return alpha;
    }

    if (ply >= MAX_PLY) [[unlikely]] {
        return pos.inCheck() ? scoreDraw(sd->nbNodes) : evaluate<Me>(pos);
    }

    Node& node = sd->node(ply);
    const Move excludedMove = node.excludedMove;
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = MOVE_NONE;
    bool inCheck = pos.inCheck();
    Score eval;
    bool improving = false;

    // continuation history
    PieceToHistory* contHist[CONT_HIST_PLIES] = {
        sd->node(ply-1).contHist,
        sd->node(ply-2).contHist,
        sd->node(ply-4).contHist
    };

    const ContCorrContext corrCtx = sd->contCorrContext(ply);

    if (RootNode) {
        node.pv.clear();
    }

    node.doubleExts = RootNode ? 0 : sd->node(ply-1).doubleExts;

    sd->node(ply+1).nbCutoffs = 0;

    // Query Transposition Table
    auto&&[ttHit, tte] = tt.get(pos.hash());
    Score ttScore = tte->score(ply);
    bool ttPv = PvNode || (ttHit && tte->isPv());
    Move ttMove = ttHit ? tte->move() : MOVE_NONE;
    bool ttTactical = ttHit && isValidMove(ttMove) && pos.isTactical<Frc>(ttMove);
    int ttDepth = tte->depth();
    bool ttLower = tte->isLowerBound();

    // Transposition Table cutoff
    if (!PvNode && !excludedMove && ttHit && ttDepth >= depth && tte->canCutoff(ttScore, beta)) {
        return ttScore;
    }

    // Static eval
    Score rawEval = SCORE_NONE;
    if (!inCheck) {
        if (ttHit) {
            rawEval = (tte->eval() != SCORE_NONE ? tte->eval() : evaluate<Me>(pos));
        } else {
            rawEval = evaluate<Me>(pos);
            tt.set(tte, pos.hash(), 0, ply, BOUND_NONE, MOVE_NONE, rawEval, SCORE_NONE, ttPv);
        }

        node.staticEval = eval = sd->moveHistory.correctEval<Me>(pos, rawEval, corrCtx);

        // Use score instead of eval if available.
        if (ttHit && !excludedMove && tte->canCutoff(ttScore, eval)) {
            eval = ttScore;
        }

        // Improving
        if (ply >= 2 && sd->node(ply - 2).staticEval != SCORE_NONE)
            improving = (node.staticEval > sd->node(ply - 2).staticEval);
        else if (ply >= 4 && sd->node(ply - 4).staticEval != SCORE_NONE)
            improving = (node.staticEval > sd->node(ply - 4).staticEval);
    } else {
        node.staticEval = eval = SCORE_NONE;
    }

    // Internal Iterative Reduction (IIR)
    if (depth >= IirDepth && ttMove == MOVE_NONE) {
        depth--;
    }

    // Reverse futility pruning (RFP)
    if (!PvNode && !inCheck && depth <= RfpDepth
        && eval - ((improving ? RfpMarginImproving : RfpMargin) * depth) >= beta)
    {
        return eval;
    }

    // Razoring
    if (!PvNode && !inCheck && depth <= RazorDepth
        && eval + (RazorMargin * depth) <= alpha)
    {
        Score score = qSearch<Me, QNodeType, Frc>(alpha, beta, ply);
        if (score <= alpha)
            return score;
    }

    // Null move pruning (NMP)
    if (!PvNode && !inCheck && !excludedMove && depth >= NmpDepth
        && pos.previousMove() != MOVE_NULL && pos.hasNonPawnMateriel<Me>() && eval >= beta
        && ply >= sd->nmpMinPly)
    {
        tt.prefetch(pos.getHashAfterNullMove());
        int R = NmpBase + depth * NmpDepthMult / 1024 + std::min((eval - beta) / NmpEvalDiv, NmpEvalMax);

        node.setNullMove(sd->moveHistory);
        pos.doNullMove<Me>();
        Score score = -pvSearch<~Me, NodeType::NonPV, Frc>(-beta, -beta+1, depth-R, ply+1, !cutNode);
        pos.undoNullMove<Me>();

        if (score >= beta) {
            if (score >= SCORE_MATE_MAX_PLY)
                score = beta;

            if (sd->nmpMinPly || depth < 17)
                return score;

            sd->nmpMinPly = ply + 3 * (depth - R) / 4;
            Score v = pvSearch<Me, NodeType::NonPV, Frc>(beta - 1, beta, depth - R, ply, false);
            sd->nmpMinPly = 0;

            if (v >= beta)
                return score;
        }
    }

    Score probCutBeta = beta + ProbCutMargin - ProbCutImproving*improving;
    if (!PvNode && !inCheck && depth >= ProbCutDepth && std::abs(beta) < SCORE_MATE_MAX_PLY
        && !(ttHit && ttDepth >= depth - ProbCutTtDepth && ttScore != SCORE_NONE && ttScore < probCutBeta))
    {
        MovePicker mp(pos, ttMove, &sd->moveHistory, contHist);
        Score probCutScore = -SCORE_INFINITE;

        mp.enumerate<QUIESCENCE, Me, Frc>([&](Move move, /*unused*/bool& skipQuiets) -> bool {
            if (move == excludedMove || !pos.isTactical<Frc>(move)) return true;
            if (!pos.see(move, probCutBeta - node.staticEval)) return true;

            sd->nbNodes++;

            node.setMove(sd->moveHistory, pos, move);

            pos.doMove<Me, Frc>(move);
            probCutScore = -qSearch<~Me, NodeType::NonPV, Frc>(-probCutBeta, -probCutBeta+1, ply+1);

            if (probCutScore >= probCutBeta)
                probCutScore = -pvSearch<~Me, NodeType::NonPV, Frc>(-probCutBeta, -probCutBeta+1, depth-4, ply+1, !cutNode);

            pos.undoMove<Me, Frc>(move);

            if (searchAborted()) return false;

            if (probCutScore >= probCutBeta) {
                tt.set(tte, pos.hash(), depth-3, ply, BOUND_LOWER, move, rawEval, probCutScore, ttPv);
                return false;
            }

            return true;
        }); if (searchAborted()) return -SCORE_INFINITE;

        if (probCutScore >= probCutBeta)
            return probCutScore;
    }

    // Check extension
    if (PvNode && inCheck && depth <= CheckExtDepth) {
        depth++;
    }

    if (!excludedMove)
        sd->moveHistory.clearKillers(ply+1);

    int nbMoves = 0;
    bool failedLow = !inCheck && eval <= alpha;
    MovePicker mp(pos, ttMove, &sd->moveHistory, ply, contHist);
    //MovePicker *mp = new (&node.mp) MovePicker(pos, ttMove, &sd->moveHistory, ply);
    PartialMoveList quietMoves, captureMoves;
    Score singularCutoff = SCORE_NONE;

    mp.enumerate<MAIN, Me, Frc>([&](Move move, bool& skipQuiets) -> bool {
        // Honor UCI searchmoves
        if (RootNode && sd->limits.searchMoves.size() > 0 && !sd->limits.searchMoves.contains(move))
            return true; // continue

        if (move == excludedMove)
            return true; // continue

        nbMoves++;

        bool moveIsTactical = pos.isTactical<Frc>(move);

        // Combined history, must be computed before the move is played
        MoveScore statScore = moveIsTactical ? sd->moveHistory.getCaptureHistory(pos, move)
                                             : sd->moveHistory.getStatScore<Me>(pos, move, contHist);

        int lmrDepth = std::max(0, depth - 1 - LMRTable[0][depth][nbMoves] / LMR_FACTOR);

        // Late move pruning
        if (!RootNode && bestScore > -SCORE_MATE_MAX_PLY) {
            // Move count pruning
            skipQuiets |= (nbMoves >= (LmpBase + depth*depth*(improving ? LmpImproving : LmpNotImproving)) / 1024);

            // Futility pruning
            Score futilityValue = eval + FpBase + FpDepthMargin*depth + statScore*FpHistMult/1024;
            if (!inCheck && !moveIsTactical && lmrDepth <= FpLmrDepth && futilityValue <= alpha) {
                skipQuiets = true;
                if (bestScore < futilityValue && futilityValue < SCORE_MATE_MAX_PLY)
                    bestScore = futilityValue;
                return true; // continue;
            }

            // Captures futility pruning
            Score captureFutilityValue = eval + CapFpMargin*depth + PieceValue<MG>(pos.getPieceAt(moveTo(move))) + statScore*CapFpHistMult/1024;
            if (!inCheck && moveIsTactical && depth <= CapFpDepth && captureFutilityValue <= alpha) {
                if (bestScore < captureFutilityValue)
                    bestScore = captureFutilityValue;
                return true; // continue;
            }

            // History pruning
            if (!inCheck && !moveIsTactical && depth <= HpDepth && statScore < -HpMargin * depth) {
                return true; // continue;
            }

            // SEE Pruning
            if (depth <= SeeDepth && !(Frc && moveType(move) == CASTLING && !pos.isEmpty(moveTo(move)))
                && !pos.see(move, moveIsTactical ? -SeeTacticalMargin*depth : -SeeQuietMargin*depth)) {
                return true; // continue;
            }
        }

        // Singular extensions
        int extension = 0;
        if (!RootNode && move == ttMove && !excludedMove && depth >= SeDepth + SeTtPvDepth*ttPv
            && std::abs(ttScore) < SCORE_MATE_MAX_PLY && ttLower && ttDepth >= depth - SeTtDepth)
        {
            Score singularBeta = ttScore - depth*SeMargin/1024;
            int corrAdj = std::abs(node.staticEval - rawEval) * SeCorrMult / 1024;

            node.excludedMove = ttMove;
            Score score = pvSearch<Me, NodeType::NonPV, Frc>(singularBeta-1, singularBeta, (depth-1)*SeSearchDepth/1024, ply, cutNode);
            node.excludedMove = MOVE_NONE;

            if (!PvNode && depth < SeDepthIncMax && score < singularBeta + SeDepthIncMargin)
                depth++;

            if (score < singularBeta) {
                extension = 1;

                // Double extension
                if (!PvNode && node.doubleExts <= SeDoubleMax) {
                    extension = 2;
                    node.doubleExts++;

                    if (score < singularBeta - SeTripleMargin + corrAdj)
                        extension = 3;
                }
            } else if (score >= beta && std::abs(score) < SCORE_MATE_MAX_PLY) {
                singularCutoff = score;
                return false; // break;
            } else if (ttScore >= beta) {
                extension = -SeNegTt;
            } else if (cutNode) {
                extension = -SeNegCut;
            }
        }

        const size_t nodesBefore = sd->nbNodes;

        sd->nbNodes++;

        if (PvNode)
            sd->node(ply+1).pv.clear();

        // Continuation history
        node.setMove(sd->moveHistory, pos, move);

        // Do move
        pos.doMove<Me, Frc>(move);
        sd->moveHistory.prefetchCorrection<~Me>(pos);

        Score score;
        int newDepth = depth - 1 + extension;

        // Late move reduction (LMR)
        if (depth >= 2 && nbMoves > 1) {
            int R = LMRTable[moveIsTactical][depth][nbMoves];

            R -= LmrPv * PvNode;
            R -= LmrInCheck * pos.inCheck();
            R += LmrNoTtPv * !ttPv;
            R += LmrTtTactical * ttTactical;
            R += LmrCutNode * cutNode;
            R += LmrNotImproving * !improving;
            R += LmrChildCutoffs * (sd->node(ply+1).nbCutoffs > 2);
            R -= statScore * (moveIsTactical ? LmrCaptHist : LmrQuietHist) / 1024;

            R = R <= 0 ? 0 : std::min(depth - 1, std::max(1, R / LMR_FACTOR));

            // Reduced depth, Zero window
            score = -pvSearch<~Me, NodeType::NonPV, Frc>(-alpha-1, -alpha, depth-R, ply+1, true);

            if (score > alpha && R > 1) {
                // Full depth, Zero window
                score = -pvSearch<~Me, NodeType::NonPV, Frc>(-alpha-1, -alpha, newDepth, ply+1, !cutNode);
            }

        } else if (!PvNode || nbMoves > 1) {
            // Zero window (PVS)
            score = -pvSearch<~Me, NodeType::NonPV, Frc>(-alpha-1, -alpha, newDepth, ply+1, !cutNode);
        }

        if (PvNode && (nbMoves == 1 || (score > alpha && (RootNode || score < beta)))) {
            // Full window (PVS)
            score = -pvSearch<~Me, NodeType::PV, Frc>(-beta, -alpha, newDepth, ply+1, false);
        }

        // Undo move
        pos.undoMove<Me, Frc>(move);

        if constexpr (RootNode)
            sd->rootMoveNodes[moveFromTo(move)] += sd->nbNodes - nodesBefore;

        if (searchAborted()) return false; // break

        if (score > bestScore) {
            bestScore = score;
            
            if (bestScore > alpha) {
                bestMove = move;
                alpha = bestScore;
                if (PvNode)
                    updatePv(node.pv, move, sd->node(ply+1).pv);

                if (alpha >= beta) {
                    node.nbCutoffs++;
                    sd->moveHistory.update<Me, Frc>(pos, bestMove, ply, depth + failedLow, quietMoves, captureMoves, contHist);
                    return false; // break
                }
            }
        }

        if (move != bestMove) {
            if (moveIsTactical) {
                if (captureMoves.size() < captureMoves.capacity())
                    captureMoves.push_back(move);
            } else if (quietMoves.size() < quietMoves.capacity()) {
                quietMoves.push_back(move);
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    if (singularCutoff != SCORE_NONE)
        return singularCutoff;

    // Checkmate / Stalemate detection
    if (nbMoves == 0) {
        return excludedMove ? alpha : inCheck ? -SCORE_MATE + ply : SCORE_DRAW;
    }

    // Update correction history
    if (!inCheck && !excludedMove && !(bestMove != MOVE_NONE && pos.isTactical<Frc>(bestMove)) && std::abs(bestScore) < SCORE_MATE_MAX_PLY
        && ((bestScore < node.staticEval && bestScore < beta) || (bestScore > node.staticEval && bestMove != MOVE_NONE)))
    {
        sd->moveHistory.updateCorrection<Me>(pos, bestScore, node.staticEval, depth, corrCtx);
    }

    // Update Transposition Table
    if (!excludedMove) {
        Bound ttBound = bestScore >= beta                ? BOUND_LOWER :
                        PvNode && bestMove != MOVE_NONE  ? BOUND_EXACT : BOUND_UPPER;
        tt.set(tte, pos.hash(), depth, ply, ttBound, bestMove, rawEval, bestScore, ttPv);
    }

    return bestScore;
}

// Quiescence search
template<Side Me, NodeType NT, bool Frc>
Score Engine::qSearch(Score alpha, Score beta, int ply) {
    constexpr bool PvNode = (NT != NodeType::NonPV);

    // Check if we should stop according to limits
    if (sd->shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by limits) exit here
    if (searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    Position &pos = sd->position;

    if (pos.isDraw(ply)) {
        return scoreDraw(sd->nbNodes);
    }

    if (alpha < SCORE_DRAW && pos.hasUpcomingRepetition(ply)) {
        alpha = scoreDraw(sd->nbNodes);
        if (alpha >= beta)
            return alpha;
    }

    // bestScore stays at -SCORE_INFINITE until a move is actually searched. That's what makes the mate detection trustworthy.
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = MOVE_NONE;
    //Node& node = sd->node(ply);

    if (ply >= MAX_PLY) [[unlikely]] {
        return pos.inCheck() ? scoreDraw(sd->nbNodes) : evaluate<Me>(pos);
    }

    bool inCheck = pos.inCheck();
    Score eval = SCORE_NONE;

    // Query Transposition Table
    auto&&[ttHit, tte] = tt.get(pos.hash());
    bool ttPv = PvNode || (ttHit && tte->isPv());
    int ttDepth = inCheck ? 1 : 0; // If we are in check use depth=1 because when we are in check we go through all moves
    Score ttScore = tte->score(ply);

    // Transposition Table cutoff
    if (!PvNode && ttHit && tte->depth() >= ttDepth && tte->canCutoff(ttScore, beta)) {
        return ttScore;
    }

    const ContCorrContext corrCtx = sd->contCorrContext(ply);

    // Standing Pat
    Score rawEval = SCORE_NONE;
    if (!inCheck) {
        if (ttHit) {
            rawEval = (tte->eval() != SCORE_NONE ? tte->eval() : evaluate<Me>(pos));
        } else {
            rawEval = evaluate<Me>(pos);
            tt.set(tte, pos.hash(), ttDepth, ply, BOUND_NONE, MOVE_NONE, rawEval, SCORE_NONE, ttPv);
        }

        eval = sd->moveHistory.correctEval<Me>(pos, rawEval, corrCtx);

        // Use score instead of eval if available.
        if (ttHit && tte->canCutoff(ttScore, eval)) {
            eval = ttScore;
        }

        if (eval >= beta) {
            return eval;
        }

        if (eval > alpha)
            alpha = eval;

        bestScore = eval;
    }

    Move ttMove = tte->move();
    bool useTTMove = ttHit && isValidMove(ttMove) && (inCheck || pos.isTactical<Frc>(ttMove));

    PieceToHistory* contHist[CONT_HIST_PLIES] = {
        sd->node(ply-1).contHist,
        sd->node(ply-2).contHist,
        sd->node(ply-4).contHist
    };

    MovePicker mp(pos, useTTMove ? ttMove : MOVE_NONE, &sd->moveHistory, contHist);
    //MovePicker *mp = new (&node.mp) MovePicker(pos, useTTMove ? ttMove : MOVE_NONE);

    Score futilityBase = eval + QsFpMargin;
    Square prevSq = isValidMove(pos.previousMove()) ? moveTo(pos.previousMove()) : SQ_NONE;

    mp.enumerate<QUIESCENCE, Me, Frc>([&](Move move, /*unused*/bool& skipQuiets) -> bool {
        if (bestScore > -SCORE_MATE_MAX_PLY) {
            // Futility Pruning
            if (!inCheck && moveType(move) == NORMAL && moveTo(move) != prevSq) {
                Score futilityScore = futilityBase + PieceValue<EG>(pos.getPieceAt(moveTo(move)));

                if (futilityScore <= alpha && !pos.givesCheck<Me, Frc>(move)) {
                    return true; // continue;
                }
            }

            // SEE Pruning
            if (!pos.see(move, -QsSeeMargin)) {
                return true; // continue;
            }
        }

        sd->nbNodes++;

        sd->node(ply).setMove(sd->moveHistory, pos, move);

        pos.doMove<Me, Frc>(move);
        sd->moveHistory.prefetchCorrection<~Me>(pos);
        Score score = -qSearch<~Me, NT, Frc>(-beta, -alpha, ply+1);
        pos.undoMove<Me, Frc>(move);

        if (searchAborted()) return false; // break

        if (score > bestScore) {
            bestScore = score;
            
            if (bestScore > alpha) {
                bestMove = move;
                alpha = bestScore;

                if (alpha >= beta) {
                    return false; // break
                }
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    // Checkmate detection. Only reachable when no evasion was searched at all, ie there is
    // no legal move: the pruning above cannot produce this state.
    if (inCheck && bestScore == -SCORE_INFINITE) {
        bestScore = -SCORE_MATE + ply;
    }

    // Update Transposition Table
    Bound ttBound = bestScore >= beta ? BOUND_LOWER : BOUND_UPPER;
    tt.set(tte, pos.hash(), ttDepth, ply, ttBound, bestMove, rawEval, bestScore, ttPv);

    return bestScore;
}

} /* namespace Belette */