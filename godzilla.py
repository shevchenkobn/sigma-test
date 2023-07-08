from dataclasses import dataclass
import sys
import typing
from enum import Enum


class CellKind(Enum):
    Residential = 'R'
    Empty = '.'
    # Godzilla = 'G'
    Destroyed = 'X'
    # Mech = 'M'


class GodzillaState(Enum):
    Current = 2
    Visited = 1
    Untouched = 0


@dataclass
class Cell:
    kind: CellKind
    godz: GodzillaState
    mech_count: int = 0


Coords = tuple[int, int]
"""Component order is (y, x)"""
LineRange = tuple[Coords, Coords]
"""Component order is ((y1, y2), (x1, x2))"""


@dataclass
class Tokyo:
    # flooding implementation implicitly relies on it
    directions: typing.ClassVar[tuple[tuple[Coords]]] = ((-1, 0), (0, 1), (1, 0), (0, -1))

    map: list[list[Cell]]
    width: int
    height: int

    gpos: Coords
    mpos: list[Coords]

    gflood: LineRange
    next_gpos: typing.Union[Coords, None] = None
    destroyed_count: int = 0

    @classmethod
    def decode(cls, width: int, height: int, get_next_line: typing.Callable[[], str]):
        m = []
        gpos = None
        mpos = []
        for y in range(height):
            line = get_next_line()
            row = []
            for x in range(width):
                kind = CellKind('.' if line[x] == 'R' else line[x])
                godz = GodzillaState.Untouched
                mech_count = 0
                if line[x] is 'G':
                    gpos = (y, x)
                    godz = GodzillaState.Current
                elif line[x] is 'M':
                    mpos.append((y, x))
                    mech_count += 1
                cell = Cell(kind, godz, mech_count)
                row.append(cell)
            m.append(row)
        return cls(m, width, height, gpos, mpos, ((gpos[0], gpos[0]), (gpos[1], gpos[1])))

    def refresh_godz_next(self) -> typing.Union[Coords, None]:
        self.next_gpos = self.get_godz_next()
        return self.next_gpos

    def get_godz_next(self) -> typing.Union[Coords, None]:
        y, x = self.gpos
        first_untouched = None
        for (oy, ox) in self.directions:
            ny, nx = y + oy, x + ox
            if ny < 0 or nx < 0:
                continue
            cell = self.map[ny][nx]
            if cell.godz == GodzillaState.Untouched:
                if cell.kind == CellKind.Residential:
                    return ny, nx
                if not first_untouched:
                    first_untouched = ny, nx
        return first_untouched

    def update_gpos(self):
        if not self.next_gpos:
            return None
        cell = self.map[self.gpos[0]][self.gpos[1]]
        next_cell = self.map[self.next_gpos[0]][self.next_gpos[1]]
        if next_cell.kind == CellKind.Residential:
            next_cell.kind = CellKind.Destroyed
            self.destroyed_count += 1
        cell.godz = GodzillaState.Visited
        next_cell.godz = GodzillaState.Current
        self.gpos = self.next_gpos
        return self.gpos

    def refresh_godz_flood(self, next=False):
        gpos = self.next_gpos if next else self.gpos
        if not gpos:
            return self.gflood

        y, x = gpos
        by, bx = self.gpos
        yr, xr = [gpos[0], gpos[0]], [gpos[1], gpos[1]]
        if not next or y < by or x != bx:
            for d in range(y - 1, -1, -1):
                if self.map[d][x].kind != CellKind.Residential:
                    yr[0] = d
        if not next or y > by or x != bx:
            for d in range(y + 1, self.height):
                if self.map[d][x].kind != CellKind.Residential:
                    yr[1] = d
        if not next or x < bx or y != by:
            for d in range(x - 1, -1, -1):
                if self.map[y][d].kind != CellKind.Residential:
                    xr[0] = d
        if not next or x > bx or y != by:
            for d in range(x + 1, self.width):
                if self.map[y][d].kind != CellKind.Residential:
                    xr[1] = d

        self.gflood = tuple(yr), tuple(xr)
        return self.gflood

    def index_of_mech_fire(self):
        gfy, gfx = self.gflood
        for i in range(len(self.mpos)):
            my, mx = self.mpos[i]
            if gfy[0] <= my <= gfy[1] and gfx[0] <= mx <= gfx[1]:
                return i
        return -1

    def simulate(self):
        self.refresh_godz_flood()
        self.refresh_godz_next()

        while True:
            new_pos = self.update_gpos()

            if new_pos:
                if self.index_of_mech_fire() > 0:
                    return self.destroyed_count

                self.refresh_godz_next()
                self.refresh_godz_flood(next=True)

            #  TODO: move mechs
            if self.index_of_mech_fire() > 0:
                return self.destroyed_count


def main():
    inf = sys.stdin
    outf = sys.stdout

    sim_count = int(inf.readline())
    for _ in range(sim_count):
        width, height = (int(d) for d in inf.readline().split(' '))
        tokyo = Tokyo.decode(width, height, lambda: inf.readline())
        destroyed_count = tokyo.simulate()
        print(destroyed_count, file=outf)

    inf.close()
    outf.close()


main()
