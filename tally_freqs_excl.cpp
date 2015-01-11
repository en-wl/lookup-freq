#include <algorithm>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"

constexpr const char fn[] = "freqs_lower_exclude.dat";
int main() {
  {
    printf("creating\n");
    Table<FreqsPos> freqs;
    Table<ToLower> to_lower;
    TableCreator<FreqsLower<fn> > tmp;
    for (auto row : freqs) {
      LowerId lid = to_lower[row.wid];
      if (lid > 0 && row.pos.exclude())
        tmp.append_row({lid,row.year,row.freq});
    }
  }
  {
    printf("sorting and merging");
    MutTable<FreqsLower<fn> > tmp;
    merge(tmp,
          [](auto row){return tie(row.lid, row.year);},
          [](auto & x, const auto & y){x.freq += y.freq;});
  }
}
