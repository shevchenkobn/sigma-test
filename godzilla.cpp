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


enum class CellKind : uint8_t {
  Empty = 0,
  Residential,
  Destroyed
};

enum class CellType : const char {
  Empty = '.',
  Residential = 'R',
  Destroyed = 'X',
  Godzilla = 'G',
  Mech = 'M'
};

// TODO: readonly bi-directional map (cannot use boost)
using CellKindNameMap = std::unordered_map<CellKind, CellType>;
CellKindNameMap CellKindName;
CellKindNameMap* initCellKindName(CellKindNameMap* m) {
  m->insert({ CellKind::Empty, CellType::Empty });
  m->insert({ CellKind::Residential, CellType::Residential });
  m->insert({ CellKind::Destroyed, CellType::Destroyed });
  return m;
}
const CellKind getCellKindOrEmpty(const char ch) {
  for (auto p : CellKindName) {
    if (ch == (char)p.second) {
      return p.first;
    }
  }
  return CellKind::Empty;
}

enum class GodzillaStatus : uint8_t {
  Untouched = 0,
  Current,
  Visited
};

const int CoordsMax = 1e6;

// TODO: is immutable only possible through private members & public getter functions? If const, assignment is broken 
struct Coords {
  static const Coords none;

  int y = INT_MIN;
  int x = INT_MIN;

  Coords() {}
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
  operator std::string() const {
    return "(" + std::to_string(y) + ", " + std::to_string(x) + ")";
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

struct MechState {
  static const MechState none;

  int mechI = -1;
  int turnN = -1;
  Coords from = Coords::none;

  MechState() {}
  MechState(int mechI, int turnN, Coords from) : mechI(mechI), turnN(turnN), from(from) {}
};
const MechState MechState::none = MechState();

struct Cell {
  CellKind kind = CellKind::Empty;
  GodzillaStatus godz = GodzillaStatus::Untouched;

  Cell() {}
  Cell(CellKind kind, GodzillaStatus godz) : kind(kind), godz(godz) {}
  Cell(const Cell& other) : kind(other.kind), godz(other.godz) {}
};

struct Mech {
  Coords pos;
  std::stack<Coords> path = std::stack<Coords>();
  Coords goal = Coords::none;

  Mech(const Coords& pos) : pos(pos), goal(pos) {}
  Mech(const Mech& other) : pos(other.pos), path(other.path), goal(other.goal) {}
};

struct Flood : Coords {
  std::pair<int, int> xLimits;
  std::pair<int, int> yLimits;

  Flood(int y, std::pair<int, int> xLimits, int x, std::pair<int, int> yLimits) : Coords(y, x), xLimits(xLimits), yLimits(yLimits) {}
  Flood(const Flood& other) : Coords(other), xLimits(other.xLimits), yLimits(other.yLimits) {}
};

/**
 * A lot of public members for easier manipulation.
 */
class Tokyo {
  public:
    static const std::vector<Coords> directions;

    std::vector<std::vector<Cell>> map;
    const int width;
    const int height;

    Coords gPos;
    /**
     * Readonly vector of mutable objects.
     * TODO: make readonly without [std::span](https://en.cppreference.com/w/cpp/container/span) or custom class.
     */
    std::vector<Mech> mechs;
    Flood gFlood;
    Coords nextGPos = Coords::none;
    int destroyedCount = 0;

    /**
     * Return value requires delete.
     * Input stream is not being closed in the function.
     * FIXME: return std::unique_ptr<Tokyo> without consequent `delete` requirement.
     */
    static std::unique_ptr<Tokyo> decode(std::istream& input) {
      int width, height;
      input >> width >> height;

      auto map = std::vector<std::vector<Cell>>(height, std::vector<Cell>(width)); // TODO: test it
      Coords gPos = Coords::none;
      auto mechs = std::vector<Mech>();
      for (int y = 0; y < height; y += 1) {
        auto& row = map[y];
        for (int x = 0; x < width; x += 1) {
          char c;
          input >> c;
          auto kind = getCellKindOrEmpty(c);
          auto godz = GodzillaStatus::Untouched;
          switch ((CellType)c) {
            case CellType::Godzilla: {
              gPos = Coords(y, x);
              godz = GodzillaStatus::Current;
              break;
            }
            case CellType::Mech: {
              mechs.push_back(Mech(Coords(y, x)));
              break;
            }
          }
          auto cell = Cell(kind, godz);
          row[x] = cell;
        }
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      return std::unique_ptr<Tokyo>(new Tokyo(map, width, height, gPos, mechs));
    }

    bool isPosValid(Coords p) const {
      return isPosValid(p.y, p.x);
    }

    bool isPosValid(int y, int x) const {
      return 0 <= y && y < height && 0 <= x && x < width;
    }

    /**
     * Memory is cleared in destructor.
     */
    Coords refreshGodzNext() {
      nextGPos = getGodzNext();
      return nextGPos;
    }

    Coords getGodzNext() {
      auto firstUntouched = Coords::none;
      Coords o = Coords::none;
      for (const auto d : directions) {
        auto p = gPos + d;
        if (!isPosValid(p)) {
          continue;
        }
        auto cell = map[p.y][p.x];
        if (cell.godz == GodzillaStatus::Untouched) {
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

    Coords updateGPos() {
      if (!nextGPos) {
        return Coords::none;
      }
      auto& cell = map[gPos.y][gPos.x];
      auto& nextCell = map[nextGPos.y][nextGPos.x];
      if (nextCell.kind == CellKind::Residential) {
        nextCell.kind = CellKind::Destroyed;
        destroyedCount += 1;
      }
      cell.godz = GodzillaStatus::Visited;
      nextCell.godz = GodzillaStatus::Current;
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

    bool isInGFlood(const Coords& p) const {
      const auto& f = gFlood;
      return f.x == p.x && f.yLimits.first <= p.y && p.y <= f.yLimits.second
        || f.y == p.y && f.xLimits.first <= p.x && p.x <= f.xLimits.second;
    }

    int indexOfMechFire() {
      for (int i = 0; i < mechs.size(); i += 1) {
        const auto& m = mechs[i];
        if (isInGFlood(m.pos)) {
          return i;
        }
      }
      return -1;
    }

    void refreshMechPaths() {
      for (auto& m : mechs) {
        if (isInGFlood(m.goal)) {
          continue;
        }
        std::queue<Coords> q({ m.pos });
        auto steps = std::unordered_map<Coords, Coords>({ {m.pos, Coords::none} });

        
        Coords last = Coords::none;
        while (!q.empty()) {
          auto c = q.front();

          for (const auto d : directions) {
            auto nc = c + d;
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
        rmp1: std::queue<Coords>().swap(q); // clear the queue

        if (!last || steps.size() == 1) {
          m.path = std::stack<Coords>();
          continue;
        }
        auto path = std::stack<Coords>();
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

    void refreshMechsState() {
      std::queue<Coords> q;
      auto steps = std::unordered_map<Coords, MechState>();
      for (int i = 0; i < mechs.size(); i += 1) {
        auto m = mechs[i];
        q.push(m.pos);
        steps[m.pos] = MechState(i, 0, Coords::none);
      }

      while (!q.empty()) {
        auto c = q.front();

        for (const auto d : directions) {
          auto nc = c + d;
          if (!isPosValid(nc)) {
            continue;
          }
          auto prevStep = steps[c];
          if (steps.find(nc) != steps.end()) {
            steps[nc] = MechState(prevStep.mechI, prevStep.turnN + 1, c);
          } else {
            auto& step = steps[nc];
            if (step.turnN < prevStep.turnN + 1) {

            }
          }
          // steps[nc] = MechState(prevStep[i])
        }
      }
    }

    void updateMechsPos() {
      for (auto& m : mechs) {
        if (m.path.empty()) {
          continue;
        }
        m.pos = m.path.top();
        m.path.pop();
      }
    }

    #ifdef _DEBUG
    void _printMap() {
      std::unordered_set<Coords> mechsPos;
      for (auto m : mechs) {
        mechsPos.insert(m.pos);
      }
      for (int y = 0; y < height; y += 1) {
        for (int x = 0; x < width; x += 1) {
          Coords p(y, x);
          if (p == gPos) {
            std::cout << (char)CellType::Godzilla;
          } else if (mechsPos.find(p) != mechsPos.end()) {
            std::cout << (char)CellType::Mech;
          } else {
            std::cout << (char)CellKindName[map[y][x].kind];
          }
        }
        std::cout << std::endl;
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
        std::cout << std::endl;
        _printMap();
        #endif

        if (indexOfMechFire() >= 0) {
          return destroyedCount;
        }
      }
      return destroyedCount;
    }

  private:
    Tokyo(std::vector<std::vector<Cell>> map, int width, int height, Coords gPos, std::vector<Mech> mechs)
      : map(map), width(width), height(height),
        gPos(gPos), mechs(mechs),
        gFlood(gPos.y, { gPos.x, gPos.x }, gPos.x, { gPos.y, gPos.y }) {}
};
const std::vector<Coords> Tokyo::directions = { Coords(-1, 0), Coords(0, 1), Coords(1, 0), Coords(0, -1) };

int main() {
  initCellKindName(&CellKindName);
  
  auto& ins = std::cin;
  auto& outs = std::cout;

  int t;
  ins >> t;
  for (int _ = 0; _ < t; _ += 1) {
    auto tokyo = Tokyo::decode(ins);
    int destroyedCount = tokyo->simulate();
    outs << destroyedCount << std::endl;
  }
}
