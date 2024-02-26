# BabChess

Another UCI-compatible chess engine written in C++. 

For now it only supports CPUs with AVX2 and BMI2 instructions. Support for other architectures might be added in the future.

See [Release page](https://github.com/vincentbab/babchess-engine/releases) for precompiled binaries

You can play against the engine on lichess: https://lichess.org/@/BabChess-Engine
<br>
The Bot is hosted on a machine with a Core i7 4785T. It will accept challenges up to 15min+10s, rated and casual.

## Compiling

Tested on Windows and Linux with GCC >= 12. Might also work with CLang

```sh
make release
```
Executable will be in `./build/Release/bin/`

## UCI Options

### Debug Log File
Log every input and output of the engine to the specified file

## Internals

### Board & Move generation
 - Bitboard
 - Zobrist hashing
 - Fast legal move generation inspired by [Gigantua](https://github.com/Gigantua/Gigantua)
```
perft 7, start position, Core i7 12700k

Nodes: 3195901860
NPS: 925007774
Time: 3455ms
```

### Search
 - Iterative deepening
 - Negamax
 - Quiescence

### Evaluation
 - Tapered
 - Material
 - PSQT ([PeSTO](https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function))

## Credits

Resources and other engines that inspired me:
 - [Chess Programming Wiki](https://www.chessprogramming.org/)
 - [Gigantua](https://github.com/Gigantua/Gigantua)
 - [Stockfish](https://stockfishchess.org/)
 - [Ethereal](https://github.com/AndyGrant/Ethereal)
