from dataclasses import dataclass
import sys
import typing
from enum import Enum


class CellKind(Enum):
    Residential = 'R'
    Empty = '.'
    Godzilla = 'G'
    Destroyed = 'X'
    Mech = 'M'


@dataclass
class Cell:
    kind: CellKind
    mech: int
    godz: int


def decode_map(width: int, height: int, get_next_line: typing.Callable[[], str]):
    m = []
    gpos = (0, 0)
    mpos = []
    for i in range(height):
        line = get_next_line()
        row = []
        for j in range(width):
            kind = CellKind(line[j])
            if kind is CellKind.Godzilla:
                gpos = (i, j)
            elif kind is CellKind.Mech:
                mpos.append((i, j))
            cell = Cell(kind, 0, 0)
            row.append(cell)
        m.append(row)
    return m, gpos, mpos


def simulate(width: int, height: int, m: list[list[Cell]], gpos, mpos):
    return -1


def main():
    inf = sys.stdin
    outf = sys.stdout

    sim_count = int(inf.readline())
    for _ in range(sim_count):
        width, height = (int(d) for d in inf.readline().split(' '))
        m, gpos, mpos = decode_map(width, height, lambda: inf.readline())
        destroyed_count = simulate(width, height, m, gpos, mpos)
        print(destroyed_count, file=outf)

    inf.close()
    outf.close()


main()
