#include <iostream>
#include <map>
#include <stack>
#include <queue>
#include <vector>
#include <functional>
#include <limits>
#include <climits>
#include <memory>

#ifdef _DEBUG
#include <unordered_set>
#endif

#define EACH_I(array, i) int i = 0; i < sizeof(array) / sizeof(array[0]); i += 1


using namespace std;

enum class CellKind : uint8_t {
  Empty = 0,
  Residential,
  Destroyed
};

// TODO: char readonly enum
const char CellTypeEmpty = '.'; 
const char CellTypeResidential = 'R';
const char CellTypeDestroyed = 'X';
const char CellTypeGodzilla = 'G';
const char CellTypeMech = 'M';

// TODO: readonly bi-directional map (cannot use boost)
using CellKindNameMap = unordered_map<CellKind, const char>;
CellKindNameMap CellKindName;
CellKindNameMap* initCellKindName(CellKindNameMap* m) {
  m->insert({ CellKind::Empty, CellTypeEmpty });
  m->insert({ CellKind::Residential, CellTypeResidential });
  m->insert({ CellKind::Destroyed, CellTypeDestroyed });
  return m;
}
const CellKind getCellKindOrEmpty(const char ch) {
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

  Cell() : kind(CellKind::Empty), godz(GodzillaState::Untouched) {}
  Cell(CellKind kind, GodzillaState godz) : kind(kind), godz(godz) {}
  Cell(const Cell& other) : kind(other.kind), godz(other.godz) {}
};

const int CoordsMax = 1e6;

// TODO: is immutable only possible through private members & public getter functions? If const, assignment is broken 
struct Coords {
  static const Coords none;

  int y;
  int x;

  Coords() : y(INT_MIN), x(INT_MIN) {}
  Coords(int y, int x) : y(y), x(x) {}
  Coords(const Coords& other) : y(other.y), x(other.x) {}

  Coords operator+(const Coords& other) const {
    return Coords(y + other.y, x + other.x);
  }

  bool operator==(const Coords& other) const {
    return y == other.y && x == other.x;
  }

  operator bool() const {
    return y != INT_MIN || x != INT_MIN;
  }

  #ifdef _DEBUG
  operator string() const {
    return "(" + to_string(y) + ", " + to_string(x) + ")";
  }
  #endif
};
const Coords Coords::none = Coords();

template<>
struct std::hash<Coords> {
  size_t operator()(const Coords& p) const {
    return p.y * CoordsMax + p.x;
  }
};

struct Mech {
  Coords pos;
  stack<Coords> path;
  Coords goal;

  Mech(const Coords& pos) : pos(pos), path(stack<Coords>()), goal(pos) {}
  Mech(const Mech& other) : pos(other.pos), path(other.path), goal(other.goal) {}
};

struct Flood : Coords {
  pair<int, int> xLimits;
  pair<int, int> yLimits;

  Flood(int y, pair<int, int> xLimits, int x, pair<int, int> yLimits) : Coords(y, x), xLimits(xLimits), yLimits(yLimits) {}
  Flood(const Flood& other) : Coords(other), xLimits(other.xLimits), yLimits(other.yLimits) {}
};

/**
 * A lot of public members for easier manipulation.
 */
class Tokyo {
  public:
    static const Coords directions[];

    /**
     * Vector is not used because width & height are present.
     */
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
    int destroyedCount = 0;

    /**
     * Return value requires delete.
     * Input stream is not being closed in the function.
     * FIXME: return std::unique_ptr<Tokyo> without consequent `delete` requirement.
     */
    static unique_ptr<Tokyo> decode(istream& input) {
      int width, height;
      input >> width >> height;

      auto map = new Cell*[height];
      Coords gPos = Coords::none;
      auto mechs = new vector<Mech>();
      for (int y = 0; y < height; y += 1) {
        auto row = new Cell[width];
        for (int x = 0; x < width; x += 1) {
          char c;
          input >> c;
          auto kind = getCellKindOrEmpty(c);
          auto godz = GodzillaState::Untouched;
          switch (c) {
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
          auto cell = Cell(kind, godz);
          row[x] = cell;
        }
        map[y] = row;
        input.ignore(numeric_limits<streamsize>::max(), '\n');
      }
      return unique_ptr<Tokyo>(new Tokyo(map, width, height, gPos, mechs));
    }

    bool isPosValid(Coords p) {
      return isPosValid(p.y, p.x);
    }

    bool isPosValid(int y, int x) {
      return 0 <= y && y < height && 0 <= x && x < width;
    }

    /**
     * Memory is cleared in destructor.
     */
    Coords refreshGodzNext() {
      nextGPos = getGodzNext();
      return nextGPos;
    }

    Coords getGodzNext();

    Coords updateGPos() {
      if (!nextGPos) {
        return Coords::none;
      }
      Cell& cell = map[gPos.y][gPos.x];
      Cell& nextCell = map[nextGPos.y][nextGPos.x];
      if (nextCell.kind == CellKind::Residential) {
        nextCell.kind = CellKind::Destroyed;
        destroyedCount += 1;
      }
      cell.godz = GodzillaState::Visited;
      nextCell.godz = GodzillaState::Current;
      gPos = nextGPos;
      return nextGPos;
    }

    /**
     * If residential cell is found at the boundary, it is included into the flood.
     */
    Flood refreshGodzFlood(bool force = false) {
      Coords b(gFlood.y, gFlood.x);
      gFlood.y = gPos.y;
      gFlood.x = gPos.x;
      if (force || gPos.y < b.y || gPos.x != b.x) {
        for (int d = gPos.y - 1; d >= 0; d -= 1) {
          gFlood.yLimits.first = d;
          if (map[d][gPos.x].kind == CellKind::Residential) {
            break;
          }
        }
      }
      if (force || gPos.y > b.y || gPos.x != b.x) {
        for (int d = gPos.y + 1; d < height; d += 1) {
          gFlood.yLimits.second = d;
          if (map[d][gPos.x].kind == CellKind::Residential) {
            break;
          }
        }
      }
      if (force || gPos.x < b.x || gPos.y != b.y) {
        for (int d = gPos.x - 1; d >= 0; d -= 1) {
          gFlood.xLimits.first = d;
          if (map[gPos.y][d].kind == CellKind::Residential) {
            break;
          }
        }
      }
      if (force || gPos.x > b.x || gPos.y != b.y) {
        for (int d = gPos.x + 1; d < width; d += 1) {
          gFlood.xLimits.second = d;
          if (map[gPos.y][d].kind == CellKind::Residential) {
            break;
          }
        }
      }
      return gFlood;
    }

    bool isInGFlood(Coords p) {
      auto f = gFlood; // WTF: f has garbage
      return f.x == p.x && f.yLimits.first <= p.y && p.y <= f.yLimits.second
        || f.y == p.y && f.xLimits.first <= p.x && p.x <= f.xLimits.second;
    }

    int indexOfMechFire() {
      for (int i = 0; i < mechs->size(); i += 1) {
        auto m = mechs->at(i);
        if (isInGFlood(m.pos)) {
          return i;
        }
      }
      return -1;
    }

    void refreshMechPaths();

    void updateMechsPos() {
      for (Mech& m : *mechs) {
        if (m.path.empty()) {
          continue;
        }
        m.pos = m.path.top();
        m.path.pop();
      }
    }

    #ifdef _DEBUG
    void _printMap() {
      unordered_set<Coords> mechsPos;
      for (auto m : *mechs) {
        mechsPos.insert(m.pos);
      }
      for (int y = 0; y < height; y += 1) {
        for (int x = 0; x < width; x += 1) {
          Coords p(y, x);
          if (p == gPos) {
            cout << CellTypeGodzilla;
          } else if (mechsPos.find(p) != mechsPos.end()) {
            cout << CellTypeMech;
          } else {
            cout << CellKindName[map[y][x].kind];
          }
        }
        cout << endl;
      }
    }
    #endif

    int simulate() {
      refreshGodzFlood(true);
      refreshGodzNext();

      while (true) {
        auto newPos = updateGPos();

        if (newPos) {
          refreshGodzFlood();
          if (indexOfMechFire() >= 0) {
            return destroyedCount;
          }

          refreshGodzNext();
        }

        refreshMechPaths();
        updateMechsPos();

        #ifdef _DEBUG
        cout << endl;
        _printMap();
        #endif

        if (indexOfMechFire() >= 0) {
          return destroyedCount;
        }
      }
      return destroyedCount;
    }

    ~Tokyo() {
      if (map && height > 0) {
        for (int y = 0; y < height; y += 1) {
          auto row = map[y];
          if (row && width > 0) {
            delete[] row;
          }
        }
        delete[] map;
      }
      if (mechs) {
        delete mechs;
      }
    }

  private:
    Tokyo(Cell** map, int width, int height, Coords gPos, vector<Mech>* mechs)
      : map(map), width(width), height(height),
        gPos(gPos), mechs(mechs),
        gFlood(gPos.y, { gPos.x, gPos.x }, gPos.x, { gPos.y, gPos.y }),
        nextGPos(Coords::none), destroyedCount(0) {}
};
const Coords Tokyo::directions[] = { Coords(-1, 0), Coords(0, 1), Coords(1, 0), Coords(0, -1) };

Coords Tokyo::getGodzNext() {
  auto firstUntouched = Coords::none;
  Coords o = Coords::none;
  for (EACH_I(directions, i)) {
    auto p = gPos + directions[i];
    if (!isPosValid(p)) {
      continue;
    }
    auto cell = map[p.y][p.x];
    if (cell.godz == GodzillaState::Untouched) {
      if (!firstUntouched) {
        firstUntouched = p;
      }
      if (cell.kind == CellKind::Residential) {
        return p;
      }
    }
  }
  return firstUntouched;
}

void Tokyo::refreshMechPaths() {
  for (Mech& m : *mechs) {
    if (isInGFlood(m.goal)) {
      continue;
    }
    queue<Coords> q({ m.pos });
    auto steps = unordered_map<Coords, Coords>({ {m.pos, Coords::none} });

    
    Coords last = Coords::none;
    while (!q.empty()) {
      auto c = q.front();

      for (EACH_I(directions, i)) { // WTF: `steps` after this line gets additional element & the elements change on their own
        auto nc = c + directions[i];
        if (!isPosValid(nc) || steps.find(nc) != steps.end() || map[nc.y][nc.x].kind == CellKind::Residential) {
          continue;
        }
        steps[nc] = c;
        if (isInGFlood(nc)) {
          last = nc;
          goto rmp1;
        }
        q.push(nc);
      }
      q.pop();
    }
    rmp1: queue<Coords>().swap(q); // clear the queue

    if (!last || steps.size() == 1) {
      m.path = stack<Coords>();
      continue;
    }
    auto path = stack<Coords>();
    auto curr = last;
    while (curr) {
      auto prev = steps[curr];
      if (prev) {
        path.push(curr);
        curr = prev;
      } else {
        break;
      }
    }
    m.path = path;
    m.goal = last;
  }
}

int main() {
  initCellKindName(&CellKindName);
  
  istream& ins = cin;
  ostream& outs = cout;

  int t;
  ins >> t;
  for (int _ = 0; _ < t; _ += 1) {
    auto tokyo = Tokyo::decode(ins);
    int destroyedCount = tokyo->simulate();
    outs << destroyedCount << endl;
  }
}
