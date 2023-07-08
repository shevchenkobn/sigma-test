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


Coords = typing.Tuple[int, int]
"""Component order is (y, x)"""
LineRange = typing.List[typing.List[int]]
"""Component order is [[y, x1, x2], [x, y1, y2]]"""


@dataclass
class Mech:
    pos: Coords
    path: deque  # deque[Coords]


@dataclass
class Tokyo:
    # flooding implementation implicitly relies on it
    directions: typing.ClassVar[typing.Tuple[typing.Tuple[Coords]]] = ((-1, 0), (0, 1), (1, 0), (0, -1))

    map: typing.List[typing.List[Cell]]
    width: int
    height: int

    gpos: Coords
    mechs: typing.List[Mech]

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
                kind = CellKind('.' if line[x] != 'R' else line[x])
                godz = GodzillaState.Untouched
                if line[x] == 'G':
                    gpos = (y, x)
                    godz = GodzillaState.Current
                elif line[x] == 'M':
                    mpos.append(Mech((y, x), deque()))
                cell = Cell(kind, godz)
                row.append(cell)
            m.append(row)
        return cls(m, width, height, gpos, mpos, [[gpos[0], gpos[1], gpos[1]], [gpos[1], gpos[0], gpos[0]]])

    def refresh_godz_next(self) -> typing.Union[Coords, None]:
        self.next_gpos = self.get_godz_next()
        return self.next_gpos

    def is_pos_valid(self, y: int, x: int):
        return 0 <= y < self.height and 0 <= x < self.width

    def get_godz_next(self) -> typing.Union[Coords, None]:
        y, x = self.gpos
        first_untouched = None
        for (oy, ox) in self.directions:
            ny, nx = y + oy, x + ox
            if not self.is_pos_valid(ny, nx):
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

    def refresh_godz_flood(self, force=False):
        # gpos = self.next_gpos if for_next else self.gpos
        # if not gpos:
        #     return self.gflood
        yr, xr = self.gflood

        y, x = self.gpos
        by, bx = yr[0], xr[0]
        yr[0], xr[0] = y, x
        if force or y < by or x != bx:
            for d in range(y - 1, -1, -1):
                yr[1] = d
                if self.map[d][x].kind is CellKind.Residential:
                    break
        if force or y > by or x != bx:
            for d in range(y + 1, self.height):
                yr[2] = d
                if self.map[d][x].kind is CellKind.Residential:
                    break
        if force or x < bx or y != by:
            for d in range(x - 1, -1, -1):
                xr[1] = d
                if self.map[y][d].kind is CellKind.Residential:
                    break
        if force or x > bx or y != by:
            for d in range(x + 1, self.width):
                xr[2] = d
                if self.map[y][d].kind is CellKind.Residential:
                    break

        return self.gflood

    def is_in_gflood(self, p: Coords):
        gfy, gfx = self.gflood
        return gfy[0] == p[0] and gfy[1] <= p[1] <= gfy[2] or gfx[0] == p[1] and gfx[1] <= p[0] <= gfx[2]

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
                    if not self.is_pos_valid(y, x) or nc in steps or self.map[y][x].kind == CellKind.Residential:
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
                prev = steps[curr]
                if prev:
                    path.appendleft(curr)
                    curr = prev
                else:
                    break
            m.path = path

    def update_mechs_pos(self):
        for m in self.mechs:
            if not m.path:
                continue
            m.pos = m.path.popleft()

    def _print_map(self):
        char_map = [[c.kind.value for c in r] for r in self.map]
        for m in self.mechs:
            char_map[m.pos[0]][m.pos[1]] = 'M'
        char_map[self.gpos[0]][self.gpos[1]] = 'G'
        for r in char_map:
            print(''.join(r))

    def simulate(self):
        self.refresh_godz_flood(force=True)
        self.refresh_godz_next()

        while True:
            new_pos = self.update_gpos()

            if new_pos:
                if self.index_of_mech_fire() >= 0:
                    return self.destroyed_count

                self.refresh_godz_flood()
                self.refresh_godz_next()

            self.refresh_mech_paths()
            self.update_mechs_pos()
            if self.index_of_mech_fire() >= 0:
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
