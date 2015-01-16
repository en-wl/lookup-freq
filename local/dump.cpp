#include <tuple>

#include "schema.hpp"
#include "table.hpp"

int main() {
  //Table<Totals> totals;
  //for (auto row : totals) {
  //  printf("%u %f\n", row.year, row.freq);
  //}

  Table<FreqsExclude> freqs;
  //madvise(freqs.data, freqs.len, MADV_SEQUENTIAL);
  for (auto row : freqs) {
    printf("%u %u %f\n", row.wid, row.year, row.freq);
  }

  // Table<Freqs> freqs_w_pos;
  // auto res = join(freqs, freqs_w_pos, [](auto a){return make_tuple(a.wid, a.year);});
  // while (!res.at_end()) {
  //   auto row = res.next();
  // }
}
