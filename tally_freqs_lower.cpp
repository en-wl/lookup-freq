#include <algorithm>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

int main() {
  {
    Table<Freqs>             freq;
    Table<FreqsExclude>      excl;
    Table<ToLower>           to_lower;
    TableCreator<FreqsLower> res;
    MutTable<Counted>        counted(500);
    auto log = fopen("tally_freqs_lower.log", "w");
    WordBuffer word_buffer;
    word_buffer.load("words.dat");
    auto freq_excl = join(freq, excl, [](auto row){return make_tuple(row.wid, row.year);});
    while (!freq_excl.at_end()) {
      auto row = freq_excl.next();
      if (!row.a) {
        fprintf(log, "Skipping %s %u %u\n", word_buffer.begin + row.b->wid, row.b->wid, row.b->year);
        continue;
      }
      assert(row.a);
      auto excl = row.b ? row.b->freq : 0.0;
      //printf("%u %u %f %f\n", row.a->wid, row.a->year, row.a->freq, excl);
      auto freq = row.a->freq - excl;
      auto lid = to_lower[row.a->wid];
      if (lid != 0 && row.a->year >= 1700 && freq > 0.0)  {
        res.append_row({lid, row.a->year, freq});
        counted.view[row.a->year] += freq;
      }
    }
    fclose(log);
  } {
    printf("Sorting and Merging...\n");
    MutTable<FreqsLower> res;
    merge(res, 
          [](auto row){return make_tuple(row.lid, row.year);},
          [](auto & x, const auto & y){x.freq += y.freq;});
  }
}
