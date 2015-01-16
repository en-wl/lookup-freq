#include <algorithm>
#include <vector>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

template <LookupType L, YearFilter YF> 
void dump() {
  Table<Freq<L,YF> > freq;
  Table<Lookup<L> >  lookup;
  Table<Speller<L> > speller;
  WordBuffer         buffer;
  buffer.load("words_w_lower.dat");
  vector<pair<unsigned,float>> by_freq(freq.view.begin(), freq.view.end());
  sort(by_freq.begin(), by_freq.end(), [](auto a, auto b){return a.second > b.second;});
  size_t sz = 0;
  for (auto v: by_freq) {
    sz++;
    if (v.second <= 0) break;
    //printf("%-20s %-8.4g %u\n", lookup[v.first].str(buffer), v.second, speller[v.first]);
  }
  printf("%lu\n", sz);
}

int main() {
  dump<Lower,Recent>();
  dump<Filtered,Recent>();
}
