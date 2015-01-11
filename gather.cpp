#include <algorithm>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"

struct YearDenom : YearIndex {
  typedef double Row;
};

constexpr const char fn[] = "freqs_lower.dat";
int main() {
  Table<FreqsLower<fn> > freqs;
  TmpTable<YearDenom> denom_all;
  TmpTable<YearDenom> denom_recent;
  {
    Table<Counted> counted;
    double all = 0, recent = 0;
    for (Year year = counted.min(); year != counted.max(); ++year) {
      if (counted.row(year) > 0) {
        all += 1;
        if (year >= 1980)
          recent += 1;
      }
    }
    denom_all.reserve(counted.size());
    denom_recent.reserve(counted.size());
    for (double v : counted) {
      denom_all.push_back(v*all);
      denom_recent.push_back(v*recent);
    }
  }
  {
    Table<LowerLookup> lower_lookup;
    MutTable<FreqAll>    freq_all(lower_lookup.size());
    MutTable<FreqRecent> freq_recent(lower_lookup.size());
    for (auto v : freqs) {
      freq_all.row(v.lid) += v.freq/denom_all.row(v.year);
      if (v.year >= 1980)
        freq_recent.row(v.lid) += v.freq/denom_recent.row(v.year);
    }
  }
}
