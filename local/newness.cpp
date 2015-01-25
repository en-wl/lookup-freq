#include <algorithm>
#include <tuple>
#include <vector>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

struct Newness {
  LowerId id;
  float factor;
};

int main() {
  WordBuffer words;
  words.load("words_w_lower.dat");
  Table<LowerLookup> lookup;
  Table<Freq<Lower,Recent>> recent;
  Table<Freq<Lower,Current>> current;
  vector<Newness> newness;
  newness.reserve(recent.size());
  for (unsigned i = 0; i < recent.size(); ++i) {
    if (current[i] > 1e-8 || recent[i] > 1e-8)
    //printf("%-20s %.12f %.12f\n", lookup[i].str(words), current[i], recent[i]);
      newness.push_back({i, current[i]/recent[i]});
  }
  sort(newness.begin(), newness.end(), [](auto a, auto b) {return a.factor > b.factor;});
  for (auto v : newness) {
    printf("%-20s %f\n", lookup[v.id].str(words), v.factor);
  }
}
