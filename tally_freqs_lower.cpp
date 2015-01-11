#include <algorithm>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"

constexpr const char fn_pre[] = "freqs_lower_pre.dat";
constexpr const char fn_excl[] = "freqs_lower_exclude.dat";
constexpr const char fn[] = "freqs_lower.dat";
int main() {
  Table<FreqsLower<fn_pre> >    pre;
  Table<FreqsLower<fn_excl> >   excl;
  TableCreator<FreqsLower<fn> > res;
  MutTable<Counted>             counted(500);
  auto pre_excl = join(pre, excl, [](auto row){return make_tuple(row.lid, row.year);});
  while (!pre_excl.at_end()) {
    auto row = pre_excl.next();
    assert(row.a);
    auto excl = row.b ? row.b->freq : 0.0;
    auto freq = row.a->freq - excl;
    if (row.a->year >= 1700 && freq > 0.0)  {
      res.append_row({row.a->lid, row.a->year, freq});
      counted.row(row.a->year) += freq;
    }
  }
}
