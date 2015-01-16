#include <algorithm>
#include <vector>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

template <LookupType L, YearFilter YF> 
void stats() {
  Table<Freq<L,YF> >  freq;
  Table<Lookup<L> >   lookup;
  Table<Speller<L> >  speller;
  WordBuffer          buffer;
  MutTable<Stats<L,YF> > stats(lookup.size());
  buffer.load("words_w_lower.dat");
  vector<pair<unsigned,float>> by_freq(freq.view.begin(), freq.view.end());
  sort(by_freq.begin(), by_freq.end(), [](auto a, auto b){return a.second > b.second;});
  float prev_freq = 0;
  StatsRow prev = {0,0,0};
  StatsRow next = prev;
  for (auto v: by_freq) {
    StatsRow cur = prev;
    ++cur.rank, ++next.rank;
    if (speller[v.first] <= 1) ++cur.normal_incl, ++next.normal_incl;
    if (speller[v.first] <= 2) ++cur.large_incl, ++next.large_incl;
    if (prev_freq != v.second) {prev = cur = next;}
    prev_freq = v.second;
    stats[v.first] = cur;
    //printf("%-20s %-8.4g %u %u %u\n", lookup[v.first].str(buffer), v.second, 
    //       cur.rank, cur.normal_incl, cur.large_incl);
  }
  //printf("TOTAL: %u\n", next.rank-1);
}

int main() {
  stats<Lower,Recent>();
  stats<Filtered,Recent>();
  stats<Lower,All>();
  stats<Filtered,All>();
}
