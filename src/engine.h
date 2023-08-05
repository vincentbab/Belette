#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include "position.h"

namespace BabChess {

class Engine {
public:
    Engine() = default;

    inline Position &position() { return pos; }
    inline const Position &position() const { return pos; }

private:
    Position pos;
};

} /* namespace BabChess */

#endif /* ENGINE_H_INCLUDED */
