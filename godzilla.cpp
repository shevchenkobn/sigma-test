#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>
#include <limits>
#include <climits>
#include <memory>

#ifdef _DEBUG
#include <unordered_set>
#endif


enum class CellKind : const uint8_t {
  Empty = 0,
  Residential,
  Destroyed
};

enum class CellName : const char {
  Empty = '.',
  Residential = 'R',
  Destroyed = 'X',
  Godzilla = 'G',
  Mech = 'M'
};

class CellKindName {
  public:
    static const std::unordered_map<CellKind, CellName> toName;

    static inline CellName name(CellKind kind) {
      return toName.at(kind);
    }

    static inline CellKind kindOrEmpty(const char name) {
      for (auto p : toName) {
        if (name == (char)p.second) {
          return p.first;
        }
      }
      return CellKind::Empty;
    }
};
const std::unordered_map<CellKind, CellName> CellKindName::toName = {
  {CellKind::Empty, CellName::Empty},
  {CellKind::Residential, CellName::Residential},
  {CellKind::Destroyed, CellName::Destroyed},
};

enum class GodzillaStatus : const uint8_t {
  Untouched = 0,
  Current,
  Visited
};

/**
 * Mutable.
 */
struct Coords {
  private:
    int16_t y_ = INT16_MIN;
    int16_t x_ = INT16_MIN;

  public:
    /**
     * According to problem definition, the x & y cannot be larger than 1000.
     */
    static const short maxXY;
    static const Coords none;

    Coords(int16_t y, int16_t x) : y_(y), x_(x) {}

    Coords() = default;
    Coords(const Coords& other) = default;
    Coords& operator=(const Coords& other) = default;
    bool operator==(const Coords& other) const = default;
    bool operator!=(const Coords& other) const = default;

    int16_t x() const {
      return x_;
    }

    int16_t x(const int16_t x) {
      return x_ = x;
    }

    int16_t y() const {
      return y_;
    }

    int16_t y(const int16_t y) {
      return y_ = y;
    }

    Coords operator+(const Coords& other) const {
      return Coords(y_ + other.y_, x_ + other.x_);
    }

    operator bool() const {
      return !this->operator==(Coords::none);
    }

    #ifdef _DEBUG
    operator std::string() const {
      return "(" + std::to_string(y_) + ", " + std::to_string(x_) + ")";
    }
    #endif
};
const int16_t Coords::maxXY = 1001;
const Coords Coords::none = Coords();

template<>
struct std::hash<Coords> {
  size_t operator()(const Coords& p) const {
    return p.y() * Coords::maxXY + p.x();
  }
};

struct MechState {
  private:
    int8_t mechI_ = -1;
    int32_t turnN_ = -1;
    Coords from_ = Coords::none;

    MechState(int8_t mechI, int32_t turnN, const Coords& from) : mechI_(mechI), turnN_(turnN), from_(from) {}
  
  public:
    static const MechState none;

    int8_t mechI() const {
      return mechI_;
    }

    // TODO: custom compiler warning that `m.turnN() < smth || m.turnN() == smth` should be used only after `isEmpty()`.
    int32_t turnN() const {
      return turnN_;
    }

    Coords from() const {
      return from_;
    }

    MechState(int8_t mechI) : MechState(mechI, 0, Coords::none) {}
    MechState(const MechState& other, const Coords& from) : mechI_(other.mechI_), turnN_(other.turnN_ + 1), from_(from) {}

    MechState() = default;
    MechState(const MechState& other) = default;
    MechState& operator=(const MechState& other) = default;
    bool operator==(const MechState& other) = delete;
    bool operator!=(const MechState& other) = delete;

    bool isEmpty() const {
      return mechI_ < 0 && turnN_ < 0;
    }

    operator bool() const {
      return !isEmpty();
    }
};
const MechState MechState::none = MechState(-1, -1, Coords::none);

struct Cell {
  private:
    CellKind kind_ = CellKind::Empty;
    GodzillaStatus godz_ = GodzillaStatus::Untouched;
    MechState mech_ = MechState::none;

  public:
    CellKind kind() const {
      return kind_;
    }

    CellKind kind(CellKind kind) {
      return kind_ = kind;
    }

    GodzillaStatus godz() const {
      return godz_;
    }

    GodzillaStatus godz(GodzillaStatus godz) {
      return godz_ = godz;
    }

    MechState mech() const {
      return mech_;
    }

    MechState mech(const MechState& mech) {
      return mech_ = mech;
    }

    Cell(CellKind kind, GodzillaStatus godz, const MechState& mech = MechState::none) : kind_(kind), godz_(godz), mech_(mech) {}

    Cell() = default;
    Cell(const Cell& other) = default;
    Cell& operator=(const Cell& other) = default;
    bool operator==(const Cell& other) = delete;
    bool operator!=(const Cell& other) = delete;
};

struct Mech {
  private:
    Coords pos_;

  public:
    Coords pos() const {
      return pos_;
    }

    Mech(const Coords& pos) : pos_(pos) {}

    Mech() = delete;
    Mech(const Mech& other) = default;
    Mech& operator=(const Mech& mech) = default;
    bool operator==(const Mech& other) = delete;
    bool operator!=(const Mech& other) = delete;
};

struct Flood : Coords {
  private:
    std::pair<int16_t, int16_t> xLimits_;
    std::pair<int16_t, int16_t> yLimits_;

  public:
    std::pair<int16_t, int16_t> xLimits() const {
      return xLimits_;
    }

    std::pair<int16_t, int16_t>& l_xLimits() {
      return xLimits_;
    }

    // std::pair<int16_t, int16_t> xLimits(const std::pair<int16_t, int16_t>& xLimits) {
    //   return xLimits_ = xLimits;
    // }

    std::pair<int16_t, int16_t> yLimits() const {
      return yLimits_;
    }

    std::pair<int16_t, int16_t>& l_yLimits() {
      return yLimits_;
    }
    
    // std::pair<int16_t, int16_t> yLimits(const std::pair<int16_t, int16_t>& yLimits) {
    //   return yLimits_ = yLimits;
    // }

    Flood(int y, const std::pair<int, int>& xLimits, int x, const std::pair<int, int>& yLimits) : Coords(y, x), xLimits_(xLimits), yLimits_(yLimits) {}

    Flood() = delete;
    Flood(const Flood& other) = default;
    Flood& operator=(const Flood& other) = default;
    bool operator==(const Flood& other) = delete;
    bool operator!=(const Flood& other) = delete;
};

class Tokyo {
  std::vector<std::vector<Cell>> map;
  const int16_t width;
  const int16_t height;

  Coords gPos;
  std::vector<Mech> mechs;
  Flood gFlood;
  uint32_t currTurn = 0;
  std::queue<Coords> mechQueue = {};
  Coords nextGPos = Coords::none;
  uint32_t destroyedCount = 0;

  Tokyo(const std::vector<std::vector<Cell>>& map, int16_t width, int16_t height, const Coords& gPos, const std::vector<Mech>& mechs)
    : map(map), width(width), height(height),
      gPos(gPos), mechs(mechs),
      gFlood(gPos.y(), { gPos.x(), gPos.x() }, gPos.x(), { gPos.y(), gPos.y() }) {}
  
  public:
    static const std::array<Coords, 4> directions;

    /**
     * Input stream is not being closed in the function. No more characters are read than the specified size.
     */
    static std::unique_ptr<Tokyo> decode(std::istream& input) {
      int16_t width, height;
      input >> width >> height;

      auto map = std::vector<std::vector<Cell>>(height);
      auto gPos = Coords::none;
      auto mechs = std::vector<Mech>();
      for (int16_t y = 0; y < height; y += 1) {
        auto row = std::vector<Cell>(width);
        for (int16_t x = 0; x < width; x += 1) {
          char c;
          input >> c;
          auto kind = CellKindName::kindOrEmpty(c);
          auto godz = GodzillaStatus::Untouched;
          switch ((CellName)c) {
            case CellName::Godzilla: {
              gPos = Coords(y, x);
              godz = GodzillaStatus::Current;
              break;
            }
            case CellName::Mech: {
              mechs.emplace_back(Mech(Coords(y, x)));
              break;
            }
          }
          auto cell = Cell(kind, godz);
          row[x] = cell;
        }
        map[y] = row;
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      
      return std::unique_ptr<Tokyo>(new Tokyo(map, width, height, gPos, mechs));
    }

    uint32_t simulate() {
      refreshGodzFlood(true);
      refreshGodzNext();

      tryInitMechsQueue();
      
      currTurn = 1;
      while (nextGPos) {
        auto gPosIsResidential = nextGPos && map[nextGPos.y()][nextGPos.x()].kind() == CellKind::Residential;

        const auto newPos = updateGPos();
        refreshGodzFlood();

        if (gPosIsResidential) {
          tryAddSurrounding(newPos);
        }
        refreshMechsState(currTurn);
        #ifdef _DEBUG
        _printMechMap();
        #endif

        if (indexOfMechFire(currTurn) >= 0) {
          break;
        }
        refreshGodzNext();
        
        currTurn += 1;
      }
      return destroyedCount;
    }

    protected:
      bool isPosValid(const Coords& p) const {
        return isPosValid(p.y(), p.x());
      }

      bool isPosValid(int16_t y, int16_t x) const {
        return 0 <= y && y < height && 0 <= x && x < width;
      }

      Coords refreshGodzNext() {
        nextGPos = getGodzNext();
        return nextGPos;
      }

      Coords getGodzNext() const {
        auto firstUntouched = Coords::none;
        for (const auto& d : directions) {
          auto p = gPos + d;
          if (!isPosValid(p)) {
            continue;
          }
          auto cell = map[p.y()][p.x()];
          if (cell.godz() == GodzillaStatus::Untouched) {
            if (!firstUntouched) {
              firstUntouched = p;
            }
            if (cell.kind() == CellKind::Residential) {
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
        auto& cell = map[gPos.y()][gPos.x()];
        auto& nextCell = map[nextGPos.y()][nextGPos.x()];
        if (nextCell.kind() == CellKind::Residential) {
          nextCell.kind(CellKind::Destroyed);
          destroyedCount += 1;
        }
        cell.godz(GodzillaStatus::Visited);
        nextCell.godz(GodzillaStatus::Current);
        gPos = nextGPos;
        return nextGPos;
      }

      /**
       * If residential cell is found at the boundary, it is included into the flood.
       */
      Flood refreshGodzFlood(bool force = false) {
        Coords prevGPos(gFlood.y(), gFlood.x());
        gFlood.y(gPos.y());
        gFlood.x(gPos.x());
        if (force || gPos.y() < prevGPos.y() || gPos.x() != prevGPos.x()) {
          for (int d = gPos.y() - 1; d >= 0; d -= 1) {
            gFlood.l_yLimits().first = d;
            if (map[d][gPos.x()].kind() == CellKind::Residential) {
              break;
            }
          }
        }
        if (force || gPos.y() > prevGPos.y() || gPos.x() != prevGPos.x()) {
          for (int d = gPos.y() + 1; d < height; d += 1) {
            gFlood.l_yLimits().second = d;
            if (map[d][gPos.x()].kind() == CellKind::Residential) {
              break;
            }
          }
        }
        if (force || gPos.x() < prevGPos.x() || gPos.y() != prevGPos.y()) {
          for (int d = gPos.x() - 1; d >= 0; d -= 1) {
            gFlood.l_xLimits().first = d;
            if (map[gPos.y()][d].kind() == CellKind::Residential) {
              break;
            }
          }
        }
        if (force || gPos.x() > prevGPos.x() || gPos.y() != prevGPos.y()) {
          for (int d = gPos.x() + 1; d < width; d += 1) {
            gFlood.l_xLimits().second = d;
            if (map[gPos.y()][d].kind() == CellKind::Residential) {
              break;
            }
          }
        }
        return gFlood;
      }

      bool isInGFlood(const Coords& p) const {
        const auto& f = gFlood;
        return (f.x() == p.x() && f.yLimits().first <= p.y() && p.y() <= f.yLimits().second)
          || (f.y() == p.y() && f.xLimits().first <= p.x() && p.x() <= f.xLimits().second);
      }

      int indexOfMechFire(int turnN) const {
        for (int y = gFlood.yLimits().first; y <= gFlood.yLimits().second; y += 1) {
          const auto& state = map[y][gFlood.x()].mech();
          if (!state.isEmpty() && state.turnN() <= turnN) {
            return state.mechI();
          }
        }
        for (int x = gFlood.xLimits().first; x <= gFlood.xLimits().second; x += 1) {
          const auto& state = map[gFlood.y()][x].mech();
          if (!state.isEmpty() && state.turnN() <= turnN) {
            return state.mechI();
          }
        }
        return -1;
      }

      bool tryInitMechsQueue() {
        if (!mechQueue.empty()) {
          return false;
        }
        for (int i = 0; i < mechs.size(); i += 1) {
          const auto& m = mechs[i];
          mechQueue.push(m.pos());
          map[m.pos().y()][m.pos().x()].mech(MechState(i));
        }
        return true;
      }

      void tryAddSurrounding(const Coords& p) {
        auto& cell = map[p.y()][p.x()];
        if (cell.kind() == CellKind::Residential) {
          return;
        }
        for (const auto& d : directions) {
          const auto& next = p + d;
          if (!isPosValid(next)) {
            continue;
          }
          const auto& prev = map[next.y()][next.x()].mech();
          if (!prev.isEmpty()) {
            mechQueue.push(p);
            cell.mech(MechState(prev, p));
            return;
          }
        }
      }

      void refreshMechsState(int maxTurn) {
        while (!mechQueue.empty()) {
          const auto& curr = mechQueue.front();
          const auto& prevCell = map[curr.y()][curr.x()];
          if (prevCell.mech().turnN() >= maxTurn) {
            break;
          }

          for (const auto& d : directions) {
            const auto next = curr + d;
            auto& currCell = map[next.y()][next.x()];
            if (!isPosValid(next) || currCell.kind() == CellKind::Residential) {
              continue;
            }
            auto state = MechState(prevCell.mech(), curr);
            if (currCell.mech().isEmpty()) {
              currCell.mech(state);
              mechQueue.push(next);
            } else if (currCell.mech().turnN() > state.turnN()) {
              currCell.mech(state);
            }
          }

          mechQueue.pop();
        }
      }

      #ifdef _DEBUG
      void _printMap(bool topMargin = true) const {
        if (topMargin) {
          std::cout << std::endl;
        }

        std::unordered_set<Coords> mechsPos;
        for (auto m : mechs) {
          mechsPos.emplace(m.pos());
        }
        for (int y = 0; y < height; y += 1) {
          for (int x = 0; x < width; x += 1) {
            Coords p(y, x);
            if (p == gPos) {
              std::cout << (char)CellName::Godzilla;
            } else if (mechsPos.find(p) != mechsPos.end()) {
              std::cout << (char)CellName::Mech;
            } else {
              std::cout << (char)CellKindName::name(map[y][x].kind());
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
            const auto& m = map[y][x].mech();
            if (m.turnN() >= 0) {
              std::cout << m.turnN() % 10;
            } else {
              if (Coords(y, x) == gPos) {
                std::cout << (char)CellName::Godzilla;
              } else {
                std::cout << (char)CellKindName::name(map[y][x].kind());
              }
            }
          }
          std::cout << std::endl;
        }
      }
      #endif
};
const std::array<Coords, 4> Tokyo::directions = { Coords(-1, 0), Coords(0, 1), Coords(1, 0), Coords(0, -1) };

int main() {  
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
