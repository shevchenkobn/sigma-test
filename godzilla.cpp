#include <iostream>
#include <map>
#include <memory>

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

enum class Godzilla : uint8_t {
  Untouched = 0,
  Current,
  Visited
};

int main() {
  CellKindNameMap CellKindName;
  initCellKindName(&CellKindName);
  cout << CellKindName[CellKind::Empty] << endl;
}

