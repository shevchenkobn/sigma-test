#include <iostream>
#include <map>
#include <stack>
#include <vector>
#include <string>
#include <functional>


using namespace std;

enum class CellKind : uint8_t {
  Empty = 0,
  Residential,
  Destroyed
};

// FIXME: char readonly enum
const char CellTypeEmpty = '.'; 
const char CellTypeResidential = 'R';
const char CellTypeDestroyed = 'X';
const char CellTypeGodzilla = 'G';
const char CellTypeMech = 'M';

// FIXME: readonly bi-directional map (cannot use boost)
using CellKindNameMap = map<CellKind, const char>;
CellKindNameMap CellKindName;
CellKindNameMap* initCellKindName(CellKindNameMap* m) {
  m->insert({ CellKind::Empty, CellTypeEmpty });
  m->insert({ CellKind::Residential, CellTypeDestroyed });
  m->insert({ CellKind::Destroyed, CellTypeDestroyed });
  return m;
}
const CellKind inline getCellKindOrEmpty(const char ch) {
  for (auto p : CellKindName) {
    if (p.second == ch) {
      return p.first;
    }
  }
  return CellKind::Empty;
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

  Coords() : y(0), x(0) {}
  Coords(int y, int x) : y(y), x(x) {}
};

struct Mech {
  Coords pos;
  stack<Coords> path;
  Coords goal;

  Mech(Coords pos) : pos(pos), path(stack<Coords>()), goal(pos) {}
};

struct Flood : Coords {
  pair<int, int> xLimits;
  pair<int, int> yLimits;

  Flood(int y, pair<int, int> xLimits, int x, pair<int, int> yLimits) : Coords(y, x), xLimits(xLimits), yLimits(yLimits) {}
};

/**
 * A lot of public members for easier manipulation.
 */
class Tokyo {
  public:
    static const Coords directions[];

    // FIXME: more elegant way to have an array?
    Cell** map;
    const int width;
    const int height;

    Coords gPos;
    /**
     * Readonly vector of mutable objects.
     * TODO: make readonly without [std::span](https://en.cppreference.com/w/cpp/container/span) or custom class.
     */
    vector<Mech>* mechs;
    Flood gFlood;
    Coords nextGPos;
    int destroyedCount;

    /**
     * Requires delete.
     * FIXME: return object without consequent `delete` requirement.
     * FIXME: is it the best to include readonly string_view as apposed to char*.
     */
    static Tokyo* decode(int width, int height, function<string_view()> getNextLine) {
      auto map = new Cell**[height];
      Coords gPos;
      auto mechs = new vector<Mech>();
      for (int y = 0; y < height; y += 1) {
        auto line = getNextLine();
        auto row = new Cell*[width];
        for (int x = 0; x < width; x += 1) {
          auto kind = getCellKindOrEmpty(line[x]);
          auto godz = GodzillaState::Untouched;
          switch (line[x]) {
            case CellTypeGodzilla: {
              gPos = Coords(y, x);
              godz = GodzillaState::Current;
              break;
            }
            case CellTypeMech: {
              mechs->push_back(Mech(Coords(y, x)));
              break;
            }
          }
          auto cell = new Cell(kind, godz);
          row[x] = cell;
        }
        map[y] = row;
      }
      return new Tokyo(*map, width, height, gPos, mechs);
    }

    int simulate() {
      // TODO: implement
      return destroyedCount;
    }

    ~Tokyo() {
      if (map) {
        for (int y = 0; y < height; y += 1) {
          auto row = map[y];
          if (row) {
            // for (int x = 0; x < width; x += 1) {
            //   if (row[x]) {
            //     row[x].godz = GodzillaState::Current;
            //     delete row[x];
            //   }
            // }
            delete[] row;
          }
          delete[] map;
        }
      }
      if (mechs) {
        delete mechs;
      }
    }

  private:
    Tokyo(Cell** map, int width, int height, Coords gPos, vector<Mech>* mechs) : map(map), width(width), height(height), gPos(gPos), mechs(mechs), gFlood(gPos.y, { gPos.x, gPos.x }, gPos.x, { gPos.y, gPos.y }) {}
};
const Coords Tokyo::directions[] = { Coords(-1, 0), Coords(0, 1), Coords(1, 0), Coords(0, -1) };

int main() {
  initCellKindName(&CellKindName);
  
  istream& ins = cin;
  ostream& outs = cout;

  int t;
  ins >> t;
  for (int _ = 0; _ < t; _ += 1) {
    int width, height;
    ins >> width >> height;
    
    auto tokyo = Tokyo::decode(width, height, [&ins]() {
      string line;
      getline(ins, line);
      return (string_view)line;
    });
    int destroyedCount = tokyo->simulate();
    outs << destroyedCount << endl;
    delete tokyo;
  }
}
