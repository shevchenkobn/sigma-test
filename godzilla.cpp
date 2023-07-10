#include <iostream>
#include <map>
#include <stack>
#include <vector>
#include <tuple>
#include <memory>
#include <string>

using namespace std;

enum class CellKind : uint8_t {
  Empty = 0,
  Residential,
  Destroyed
};

const char CellTypeEmpty = '.';
const char CellTypeResidential = 'R';
const char CellTypeDestroyed = 'X';
const char CellTypeGodzilla = 'G';
const char CellTypeMech = 'M';

typedef map<CellKind, const char> CellKindNameMap;

CellKindNameMap* initCellKindName(CellKindNameMap* m) {
  m->insert({ CellKind::Empty, CellTypeEmpty });
  m->insert({ CellKind::Residential, CellTypeDestroyed });
  m->insert({ CellKind::Destroyed, CellTypeDestroyed });
  return m;
}

enum class GodzillaState : uint8_t {
  Untouched = 0,
  Current,
  Visited
};

struct Cell {
  CellKind kind;
  GodzillaState godz;

  Cell(CellKind kind, GodzillaState godz) : kind(kind), godz(godz) {}
};

struct Coords {
  int y;
  int x;

  Coords(int y, int x) : y(y), x(x) {}
};

struct Mech {
  Coords pos;
  stack<Coords> path;
  Coords goal;

  Mech(Coords pos) : pos(pos), path(stack<Coords>()), goal(pos) {}
};

class Tokyo {
  public:
    static const Coords directions[];

    const vector<vector<Cell>>* map;
    const int width;
    const int height;

    const Coords gpos;
    const vector<Mech>* mechs;
    const tuple<int, int, int> gflood[2];
    const Coords nextGpos;
    const int destroyedCount;

    /**
     * Requires delete.
     */
    Tokyo* decode(int width, int height, string (*getNextLine)()) {
      map = new vector<vector<Cell>>();
      

      return nullptr;
    }
};
const Coords Tokyo::directions[] = { Coords(-1, 0), Coords(0, 1), Coords(1, 0), Coords(0, -1) };

int main() {
  CellKindNameMap CellKindName;
  initCellKindName(&CellKindName);
  cout << CellKindName[CellKind::Empty] << endl;
}
