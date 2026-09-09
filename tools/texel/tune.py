import argparse
import json
import math
import os
import re
import subprocess
import sys
import numpy as np
import scipy.sparse as sp
from scipy.optimize import minimize, minimize_scalar

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from extract import parse_chunk

NF = 384
PIECES = ['Pawn', 'Knight', 'Bishop', 'Rook', 'Queen', 'King']
VALUE_NAMES = ['Pawn', 'Knight', 'Bishop', 'Rook', 'Queen']
LN10 = math.log(10)
PSQT_RE = r'constexpr Score PSQT\[NB_PIECE_TYPE\]\[NB_PHASE\]\[NB_SQUARE\] = \{(.*?)\n\};'


def read_header(path):
    src = open(path, newline='').read()
    tempo = int(re.search(r'constexpr Score Tempo = (-?\d+);', src).group(1))
    pv = {}

    for name in VALUE_NAMES:
        pv[name] = tuple(int(re.search(rf'constexpr Score {name}Value{ph} = (-?\d+);', src).group(1)) for ph in ('Mg', 'Eg'))

    block = re.search(PSQT_RE, src, re.S).group(1)
    nums = np.array([int(x) for x in re.findall(r'-?\d+', block)], np.float64)
    assert nums.size == 6 * 2 * 64, nums.size

    psqt = nums.reshape(6, 2, 64)
    wmg = psqt[:, 0, :].copy()
    weg = psqt[:, 1, :].copy()

    for i, name in enumerate(VALUE_NAMES):
        wmg[i] += pv[name][0]
        weg[i] += pv[name][1]
        
    return src, pv, np.concatenate([wmg.ravel(), weg.ravel(), [tempo]])


def load(path):
    z = np.load(path)
    X = sp.csr_matrix((z['data'].astype(np.float32), z['indices'].astype(np.int32), z['indptr']), shape=(len(z['phase']), NF))
    X.sum_duplicates()

    return X, z['phase'].astype(np.float32), z['stm'].astype(np.float32), z['result'].astype(np.float32)


class Model:
    def __init__(self, X, phase, stm, result):
        self.X = X
        self.XT = X.T.tocsr()
        self.pm = phase / np.float32(24.0)
        self.pe = np.float32(1.0) - self.pm
        self.stm = stm
        self.r = result
        self.n = len(result)

    def evals(self, theta):
        t = theta.astype(np.float32)

        return self.pm * (self.X @ t[:NF]) + self.pe * (self.X @ t[NF:2 * NF]) + self.stm * t[2 * NF]

    def mse(self, theta, K):
        p = 1.0 / (1.0 + 10.0 ** (-K * self.evals(theta) / 400.0))
        d = (p - self.r).astype(np.float64)

        return float(np.dot(d, d) / self.n)

    def fun(self, theta, K):
        e = self.evals(theta)
        p = 1.0 / (1.0 + 10.0 ** (np.float32(-K / 400.0) * e))
        d = p - self.r
        d64 = d.astype(np.float64)
        loss = float(np.dot(d64, d64) / self.n)
        g = (d * p * (1.0 - p) * np.float32(2.0 * K * LN10 / 400.0 / self.n)).astype(np.float32)
        grad = np.concatenate([self.XT @ (g * self.pm), self.XT @ (g * self.pe), [np.dot(g, self.stm)]])

        return loss, grad.astype(np.float64)


def fit_k(model, theta):
    res = minimize_scalar(lambda k: model.mse(theta, k), bounds=(0.3, 5.0), method='bounded', options={'xatol': 1e-4})

    return res.x


def verify(engine, book, theta, count):
    with open(book) as f:
        lines = [next(f) for _ in range(count)]

    counts, idx, val, phase, stm, _ = parse_chunk(lines)
    indptr = np.zeros(len(counts) + 1, np.int64)
    np.cumsum(counts, out=indptr[1:])
    X = sp.csr_matrix((val.astype(np.float64), idx.astype(np.int32), indptr), shape=(len(counts), NF))
    X.sum_duplicates()
    mg = X @ theta[:NF]
    eg = X @ theta[NF:2 * NF]
    ph = phase.astype(np.float64)
    white = np.fix((mg * ph + eg * (24 - ph)) / 24) + stm * theta[2 * NF]
    expected = (white * stm).astype(np.int64)

    cmds = ''.join(f'position fen {l.rsplit(" ", 1)[0]}\neval\n' for l in lines) + 'quit\n'
    out = subprocess.run([os.path.abspath(engine)], input=cmds, capture_output=True, text=True).stdout
    got = np.array([int(m) for m in re.findall(r'Static eval: (-?\d+)', out)], np.int64)
    assert len(got) == len(expected), (len(got), len(expected))
    diff = np.abs(got - expected)
    print(f'verify: {len(got)} positions, max diff {diff.max()}, mismatches {(diff > 0).sum()}')

    return diff.max() == 0


def split_tables(theta, pv):
    wmg = np.rint(theta[:NF]).astype(int).reshape(6, 64)
    weg = np.rint(theta[NF:2 * NF]).astype(int).reshape(6, 64)
    tempo = int(round(theta[2 * NF]))
    psqt = np.zeros((6, 2, 64), int)

    for i, name in enumerate(VALUE_NAMES):
        psqt[i, 0] = wmg[i] - pv[name][0]
        psqt[i, 1] = weg[i] - pv[name][1]
    psqt[5, 0] = wmg[5]
    psqt[5, 1] = weg[5]
    psqt[0, :, :8] = 0
    psqt[0, :, 56:] = 0

    return tempo, psqt


def format_psqt(psqt):
    out = ['constexpr Score PSQT[NB_PIECE_TYPE][NB_PHASE][NB_SQUARE] = {', '    {},']
    for i, name in enumerate(PIECES):
        out.append(f'    // {name}')
        out.append('    {')

        for ph in range(2):
            out.append('        {')

            for rank in range(8):
                row = psqt[i, ph, rank * 8:rank * 8 + 8]
                out.append('            ' + ', '.join(f'{v:4d}' for v in row) + ',')
            out.append('        },' if ph == 0 else '        }')
        out.append('    },' if i < 5 else '    }')
    out.append('};')

    return '\n'.join(out)


def write_header(path, src, tempo, psqt):
    src = re.sub(r'constexpr Score Tempo = -?\d+;', f'constexpr Score Tempo = {tempo};', src)
    src = re.sub(PSQT_RE, lambda m: format_psqt(psqt), src, flags=re.S)
    crlf = '\r\n' in src
    src = src.replace('\r\n', '\n')

    open(path, 'w', newline='').write(src.replace('\n', '\r\n') if crlf else src)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('features')
    ap.add_argument('--header', default='src/evaluate.h')
    ap.add_argument('--verify', help='engine binary to check feature extraction against')
    ap.add_argument('--book', help='book file used with --verify')
    ap.add_argument('--val', type=float, default=0.1)
    ap.add_argument('--maxiter', type=int, default=400)
    ap.add_argument('--K', type=float)
    ap.add_argument('--no-anchor', action='store_true', help='do not rescale to the initial pawn eg mean')
    ap.add_argument('--write', action='store_true', help='rewrite the header with tuned values')
    ap.add_argument('--out', default='tuned.json')
    args = ap.parse_args()

    src, pv, theta0 = read_header(args.header)

    if args.verify:
        if not verify(args.verify, args.book, theta0, 3000):
            sys.exit('feature extraction does not match engine eval')

    X, phase, stm, result = load(args.features)
    n = X.shape[0]
    nval = int(n * args.val)
    ntrain = n - nval
    train = Model(X[:ntrain], phase[:ntrain], stm[:ntrain], result[:ntrain])
    val = Model(X[ntrain:], phase[ntrain:], stm[ntrain:], result[ntrain:])
    print(f'{ntrain} train, {nval} val, {X.nnz} nnz', flush=True)

    K = args.K or fit_k(train, theta0)
    print(f'K = {K:.4f}')
    print(f'initial mse: train {train.mse(theta0, K):.6f}  val {val.mse(theta0, K):.6f}', flush=True)

    it = [0]

    def cb(xk):
        it[0] += 1

        if it[0] % 20 == 0:
            print(f'iter {it[0]}: train {train.mse(xk, K):.6f}  val {val.mse(xk, K):.6f}', flush=True)

    res = minimize(train.fun, theta0, args=(K,), jac=True, method='L-BFGS-B', callback=cb,
                   options={'maxiter': args.maxiter, 'maxfun': args.maxiter * 2, 'ftol': 1e-12, 'gtol': 1e-9})
    theta = res.x
    print(f'{res.message} after {res.nit} iterations')
    print(f'final mse: train {train.mse(theta, K):.6f}  val {val.mse(theta, K):.6f}')

    pawn_eg0 = theta0[NF + 8:NF + 56].mean()
    pawn_eg = theta[NF + 8:NF + 56].mean()
    scale = 1.0 if args.no_anchor else pawn_eg0 / pawn_eg
    print(f'pawn eg mean {pawn_eg0:.2f} -> {pawn_eg:.2f}, scale {scale:.4f}, effective K {K / scale:.4f}')
    
    theta *= scale
    print(f'anchored mse: train {train.mse(theta, K / scale):.6f}  val {val.mse(theta, K / scale):.6f}')

    tempo, psqt = split_tables(theta, pv)
    print(f'Tempo {tempo}')

    for i, name in enumerate(PIECES):
        print(f'{name} mean mg {theta[i * 64:(i + 1) * 64].mean():.1f} eg {theta[NF + i * 64:NF + (i + 1) * 64].mean():.1f}')

    json.dump({'K': K, 'scale': scale, 'theta': theta.tolist(), 'tempo': tempo, 'psqt': psqt.tolist()}, open(args.out, 'w'))

    if args.write:
        write_header(args.header, src, tempo, psqt)
        print(f'wrote {args.header}')


if __name__ == '__main__':
    main()
