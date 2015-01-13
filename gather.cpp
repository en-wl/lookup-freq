#include <algorithm>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"

struct YearDenom : YearIndex {
  typedef double Row;
};

int main() {
  Table<FreqsLower> freqs;
  TmpTable<YearDenom> denom_all;
  TmpTable<YearDenom> denom_recent;
  {
    Table<Counted> counted;
    double all = 0, recent = 0;
    for (auto v : counted.view) {
      if (v.second > 0) {
        all += 1;
        if (v.first >= 1980)
          recent += 1;
      }
    }
    printf("All: %f  Recent: %f\n", all, recent);
    denom_all.reserve(counted.size());
    denom_recent.reserve(counted.size());
    for (double v : counted) {
      denom_all.push_back(v*all);
      denom_recent.push_back(v*recent);
    }
  }
  {
    Table<LowerLookup>     lower_lookup;
    MutTable<Freq<All>>    freq_all(lower_lookup.size());
    MutTable<Freq<Recent>> freq_recent(lower_lookup.size());
    for (auto v : freqs) {
      freq_all.view[v.lid] += v.freq/denom_all.view[v.year];
      if (v.year >= 1980)
        freq_recent.view[v.lid] += v.freq/denom_recent.view[v.year];
    }
  }
}
