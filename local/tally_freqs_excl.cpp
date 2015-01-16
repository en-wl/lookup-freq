#include <algorithm>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"

int main() {
  auto key    = [](auto row){return make_tuple(row.wid,row.year);};
  auto merger = [](auto & x, const auto & y) {x.freq += y.freq;};
  {
    Table<FreqsPos> freqs;
    auto tmp = merge_table_creator<FreqsExclude>(key, merger);
    for (auto row : freqs) {
      if (row.pos.exclude()) {
        tmp.append_row({row.wid,row.year,row.freq});
      }
    }
    assert(tmp.sorted);
  }
  //{
  //  MutTable<FreqsExclude> tmp;
  //  merge(tmp, key, merger);
  //}
}
