#ifndef PERFT_H_INCLUDED
#define PERFT_H_INCLUDED

#include "chess.h"
#include "position.h"

namespace Belette {

template<bool Div>
size_t perft(Position &pos, int depth);

void perft(Position &pos, int depth);

} /* namespace Belette */

#endif /* PERFT_H_INCLUDED */
