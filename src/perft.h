#ifndef PERFT_H_INCLUDED
#define PERFT_H_INCLUDED

#include "chess.h"
#include "position.h"

namespace BabChess {

template<bool Div>
size_t perft(Position &pos, int depth);

} /* namespace BabChess */

#endif /* PERFT_H_INCLUDED */
