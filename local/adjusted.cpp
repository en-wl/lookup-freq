#include <algorithm>
#include <vector>
#include <tuple>
#include <unordered_set>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

int main() {
  Table<Freq<Lower,Recent> >          recent;
  Table<Freq<Lower,Current> >         current;
  TableCreator<Freq<Lower,Adjusted> > adjusted;
  for (unsigned i = 0; i < recent.size(); ++i) {
    adjusted.append_row(max(recent[i],current[i]));
  }
}
