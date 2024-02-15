#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include "position.h"

namespace BabChess {

struct ThinkParams {
    int timeLeft[2] = {0};
    int increment[2] = {0};
    int movesToGo = 0;
    int maxDepth = 0;
    int maxNodes = 0;
    int maxTime = 0;
};

class Engine {
public:
    Engine() = default;

    inline Position &position() { return pos; }
    inline const Position &position() const { return pos; }

    void think(const ThinkParams &params);
    void stop();

    virtual void onThinkProgress() = 0;
    virtual void onThinkFinish() = 0;

private:
    Position pos;
    bool stopThinking = true;
};

} /* namespace BabChess */

#endif /* ENGINE_H_INCLUDED */
