from dataclasses import dataclass
import sys
import typing
from collections import deque
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


Coords = tuple[int, int]
"""Component order is (y, x)"""
LineRange = tuple[Coords, Coords]
"""Component order is ((y1, y2), (x1, x2))"""


@dataclass
class Mech:
    pos: Coords
    path: deque[Coords]


@dataclass
class Tokyo:
    # flooding implementation implicitly relies on it
    directions: typing.ClassVar[tuple[tuple[Coords]]] = ((-1, 0), (0, 1), (1, 0), (0, -1))

    map: list[list[Cell]]
    width: int
    height: int

    gpos: Coords
    mechs: list[Mech]

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
                if line[x] is 'G':
                    gpos = (y, x)
                    godz = GodzillaState.Current
                elif line[x] is 'M':
                    mpos.append(Mech((y, x), deque()))
                cell = Cell(kind, godz)
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

    def refresh_godz_flood(self, for_next=False):
        gpos = self.next_gpos if for_next else self.gpos
        if not gpos:
            return self.gflood

        y, x = gpos
        by, bx = self.gpos
        yr, xr = [gpos[0], gpos[0]], [gpos[1], gpos[1]]
        if not for_next or y < by or x != bx:
            for d in range(y - 1, -1, -1):
                if self.map[d][x].kind != CellKind.Residential:
                    yr[0] = d
        if not for_next or y > by or x != bx:
            for d in range(y + 1, self.height):
                if self.map[d][x].kind != CellKind.Residential:
                    yr[1] = d
        if not for_next or x < bx or y != by:
            for d in range(x - 1, -1, -1):
                if self.map[y][d].kind != CellKind.Residential:
                    xr[0] = d
        if not for_next or x > bx or y != by:
            for d in range(x + 1, self.width):
                if self.map[y][d].kind != CellKind.Residential:
                    xr[1] = d

        self.gflood = tuple(yr), tuple(xr)
        return self.gflood

    def is_in_gflood(self, p: Coords):
        gfy, gfx = self.gflood
        return gfy[0] <= p[0] <= gfy[1] and gfx[0] <= p[1] <= gfx[1]

    def index_of_mech_fire(self):
        for i in range(len(self.mechs)):
            m = self.mechs[i]
            if self.is_in_gflood(m.pos):
                return i
        return -1

    def refresh_mech_paths(self):
        for m in self.mechs:
            q = [m.pos]
            steps = {m.pos: None}

            last = None
            while q:
                c = q.pop()

                for oy, ox in self.directions:
                    y, x = c[0] + oy, c[1] + ox
                    nc = (y, x)
                    if nc in steps or self.map[y][x].kind == CellKind.Residential:
                        continue
                    steps[nc] = c
                    if self.is_in_gflood(nc):
                        last = nc
                        break
                    q.append(nc)
                else:
                    continue
                break
            del q
            if last is None or len(steps) == 1:
                m.path = deque()
                continue
            path = deque(maxlen=len(steps) - 1)
            curr = last
            while curr:
                path.appendleft(curr)
                curr = steps[curr]
            m.path = path

    def update_mechs_pos(self):
        for m in self.mechs:
            if not m.path:
                continue
            m.pos = m.path.popleft()

    def simulate(self):
        self.refresh_godz_flood()
        self.refresh_godz_next()

        while True:
            new_pos = self.update_gpos()

            if new_pos:
                if self.index_of_mech_fire() > 0:
                    return self.destroyed_count

                self.refresh_godz_next()
                self.refresh_godz_flood(for_next=True)

            self.refresh_mech_paths()
            self.update_mechs_pos()
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
