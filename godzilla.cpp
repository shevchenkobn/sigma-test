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

// FIXME: better create a class with 2 `const unordered_map&`
using CellKindNameMap = std::unordered_map<CellKind, CellType>;
CellKindNameMap CellKindName;
const CellKindNameMap& initCellKindName(CellKindNameMap& m) {
  m[CellKind::Empty] = CellType::Empty;
  m[CellKind::Residential] = CellType::Residential;
  m[CellKind::Destroyed] = CellType::Destroyed;
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

// FIXME: add getters as `const type& get() const { ... }`
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
    return this->operator==(Coords::none);
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

  bool isUninit() const {
    return mechI < 0 && turnN < 0;
  }

  // operator bool() const {
  //   return !isUninit();
  // }

  MechState() {}
  MechState(int mechI, int turnN, const Coords& from) : mechI(mechI), turnN(turnN), from(from) {}
  MechState(const MechState& other) : mechI(other.mechI), turnN(other.turnN), from(other.from) {}
};
const MechState MechState::none = MechState();

struct Cell {
  CellKind kind = CellKind::Empty;
  GodzillaStatus godz = GodzillaStatus::Untouched;
  MechState mech = MechState::none;

  Cell() {}
  Cell(CellKind kind, GodzillaStatus godz, const MechState& mech = MechState::none) : kind(kind), godz(godz), mech(mech) {}
  Cell(const Cell& other) : kind(other.kind), godz(other.godz), mech(mech) {}
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

  Flood(int y, const std::pair<int, int>& xLimits, int x, const std::pair<int, int>& yLimits) : Coords(y, x), xLimits(xLimits), yLimits(yLimits) {}
  Flood(const Flood& other) : Coords(other), xLimits(other.xLimits), yLimits(other.yLimits) {}
};

class Tokyo {
  std::vector<std::vector<Cell>> map;
  const int width;
  const int height;

  Coords gPos;
  /**
   * Readonly vector of mutable objects.
   */
  std::vector<Mech> mechs;
  // std::queue<Coords> mechQueue;
  Flood gFlood;
  Coords nextGPos = Coords::none;
  int destroyedCount = 0;

  Tokyo(const std::vector<std::vector<Cell>>& map, int width, int height, const Coords& gPos, const std::vector<Mech>& mechs)
    : map(map), width(width), height(height),
      gPos(gPos), mechs(mechs),
      gFlood(gPos.y, { gPos.x, gPos.x }, gPos.x, { gPos.y, gPos.y }) {}
  
  public:
    static const std::vector<Coords> directions;

    /**
     * Input stream is not being closed in the function.
     */
    static std::unique_ptr<Tokyo> decode(std::istream& input) {
      int width, height;
      input >> width >> height;

      auto map = std::vector<std::vector<Cell>>(height);
      Coords gPos = Coords::none;
      auto mechs = std::vector<Mech>();
      for (int y = 0; y < height; y += 1) {
        map[y] = std::vector<Cell>(width);
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
          map[y][x] = cell;
        }
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      return std::unique_ptr<Tokyo>(new Tokyo(map, width, height, gPos, mechs));
    }

    bool isPosValid(const Coords& p) const {
      return isPosValid(p.y, p.x);
    }

    bool isPosValid(int y, int x) const {
      return 0 <= y && y < height && 0 <= x && x < width;
    }

    Coords refreshGodzNext() {
      nextGPos = getGodzNext();
      return nextGPos;
    }

    Coords getGodzNext() const {
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

    int indexOfMechFire() const {
      for (int i = 0; i < mechs.size(); i += 1) {
        const auto& m = mechs[i];
        if (isInGFlood(m.pos)) {
          return i;
        }
      }
      return -1;
    }

    int indexOfMechFire(int turnN) const {
      for (int y = gFlood.yLimits.first; y <= gFlood.yLimits.second; y += 1) {
        const auto& state = map[y][gFlood.x].mech;
        if (state.turnN <= turnN) {
          return state.mechI;
        }
      }
      for (int x = gFlood.xLimits.first; x <= gFlood.xLimits.second; x += 1) {
        const auto& state = map[gFlood.y][x].mech;
        if (state.turnN <= turnN) {
          return state.mechI;
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

          for (const auto& d : directions) {
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
      for (int i = 0; i < mechs.size(); i += 1) {
        const auto& m = mechs[i];
        q.push(m.pos);
        map[m.pos.y][m.pos.x].mech = MechState(i, 0, Coords::none);
      }

      while (!q.empty()) {
        const auto& c = q.front();

        for (const auto d : directions) {
          const auto nc = c + d;
          if (!isPosValid(nc)) {
            continue;
          }
          const auto& prevCell = map[c.y][c.x];
          auto& currCell = map[nc.y][nc.x];
          auto state = MechState(prevCell.mech.mechI, prevCell.mech.turnN + 1, c);
          if (currCell.mech.isUninit()) {
            currCell.mech = state;
            q.push(nc);
          } else if (currCell.mech.turnN > state.turnN) {
            currCell.mech = state;
          }
        }

        q.pop();
      }

      #ifdef _DEBUG
      _printMechMap();
      #endif

      // for (auto& [pos, state] : steps) {
      //   map[pos.y][pos.x].mech = state;
      // }
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
    void _printMap(bool topMargin = true) const {
      if (topMargin) {
        std::cout << std::endl;
      }

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

    void _printMechMap(bool topMargin = true) const {
      if (topMargin) {
        std::cout << std::endl;
      }

      for (int y = 0; y < height; y += 1) {
        for (int x = 0; x < width; x += 1) {
          std::cout << map[y][x].mech.turnN;
        }
        std::cout << std::endl;
      }
    }
    #endif

    int predict() {
      refreshGodzFlood(true);
      refreshGodzNext();

      refreshMechsState();
      
      int i = 1;
      while (nextGPos) {
        const auto newPos = updateGPos();
        refreshGodzFlood();
        #ifdef _DEBUG
        _printMap();
        #endif

        if (indexOfMechFire(i) >= 0) {
          break;
        }
        refreshGodzNext();
        
        i += 1;
      }
      return destroyedCount;
    }

    int simulate() {
      refreshGodzFlood(true);
      refreshGodzNext();

      while (true) {
        const auto newPos = updateGPos();

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
};
const std::vector<Coords> Tokyo::directions = { Coords(-1, 0), Coords(0, 1), Coords(1, 0), Coords(0, -1) };

int main() {
  initCellKindName(CellKindName);
  
  auto& ins = std::cin;
  auto& outs = std::cout;
  
  int t;
  ins >> t;
  for (int _ = 0; _ < t; _ += 1) {
    auto tokyo = Tokyo::decode(ins);
    int destroyedCount = tokyo->predict();
    outs << destroyedCount << std::endl;
  }
}
