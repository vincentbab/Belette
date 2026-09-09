import argparse
import multiprocessing as mp
import numpy as np

PIECE = {'P': 0, 'N': 1, 'B': 2, 'R': 3, 'Q': 4, 'K': 5}
PHASE_W = {0: 0, 1: 1, 2: 1, 3: 2, 4: 4, 5: 0}
RESULT = {'[1.0]': 1.0, '[0.5]': 0.5, '[0.0]': 0.0}


def parse_chunk(lines):
    counts, idx, val, phase, stm, res = [], [], [], [], [], []

    for line in lines:
        parts = line.split()

        if len(parts) < 2 or parts[-1] not in RESULT:
            continue

        board, side = parts[0], parts[1]
        sq = 56
        n = 0
        ph = 0

        for c in board:
            if c == '/':
                sq -= 16
            elif c.isdigit():
                sq += ord(c) - 48
            else:
                pt = PIECE[c.upper()]
                ph += PHASE_W[pt]

                if c.isupper():
                    idx.append(pt * 64 + sq)
                    val.append(1)
                else:
                    idx.append(pt * 64 + (sq ^ 56))
                    val.append(-1)
                n += 1
                sq += 1

        counts.append(n)
        phase.append(ph)
        stm.append(1 if side == 'w' else -1)
        res.append(RESULT[parts[-1]])

    return (np.array(counts, np.int32), np.array(idx, np.int16), np.array(val, np.int8),
            np.array(phase, np.int8), np.array(stm, np.int8), np.array(res, np.float32))


def chunks(path, size):
    buf = []

    with open(path) as f:
        for line in f:
            buf.append(line)
            if len(buf) >= size:
                yield buf
                buf = []
    if buf:
        yield buf


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('book')
    ap.add_argument('out')
    ap.add_argument('--workers', type=int, default=max(1, mp.cpu_count() - 2))
    args = ap.parse_args()

    with mp.Pool(args.workers) as pool:
        results = list(pool.imap(parse_chunk, chunks(args.book, 50000), chunksize=1))

    counts = np.concatenate([r[0] for r in results])
    indptr = np.zeros(len(counts) + 1, np.int64)
    np.cumsum(counts, out=indptr[1:])
    np.savez(args.out,
            indptr=indptr,
            indices=np.concatenate([r[1] for r in results]),
            data=np.concatenate([r[2] for r in results]),
            phase=np.concatenate([r[3] for r in results]),
            stm=np.concatenate([r[4] for r in results]),
            result=np.concatenate([r[5] for r in results]))
    print(f'{len(counts)} positions, {indptr[-1]} features')


if __name__ == '__main__':
    main()
