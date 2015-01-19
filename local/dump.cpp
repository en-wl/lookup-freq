#include <tuple>

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

int main() {
  //Table<Totals> totals;
  //for (auto row : totals) {
  //  printf("%u %f\n", row.year, row.freq);
  //}

  //Table<FreqsExclude> freqs;
  //madvise(freqs.data, freqs.len, MADV_SEQUENTIAL);
  //for (auto row : freqs) {
  //  printf("%u %u %f\n", row.wid, row.year, row.freq);
  //}

  // Table<Freqs> freqs_w_pos;
  // auto res = join(freqs, freqs_w_pos, [](auto a){return make_tuple(a.wid, a.year);});
  // while (!res.at_end()) {
  //   auto row = res.next();
  // }

  Table<LowerLookup> lookup;
  WordBuffer         buffer;
  buffer.load("words_w_lower.dat");
  for (auto str : lookup) {
    printf("%s\n", str.str(buffer));
  }
}
