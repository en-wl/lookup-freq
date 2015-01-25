#include <algorithm>
#include <vector>
#include <tuple>
#include <unordered_set>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

template <LookupType L, YearFilter YF> 
void stats(SpellerStats && sp) {
  Table<Freq<L,YF> >  freq;
  printf("%lu %lu %f\n", freq.len, freq.size_, freq.view[0]);
  Table<Lookup<L> >   lookup;
  Table<Speller<L> >  speller;
  WordBuffer          buffer;
  MutTable<Stats<L,YF> > stats(lookup.size());
  for (auto & v : stats) v = {-1,-1,-1};
  printf("%lu %lu %f\n", freq.len, freq.size_, freq.view[0]);
  buffer.load("words_w_lower.dat");
  vector<pair<unsigned,float>> by_freq(freq.view.begin(), freq.view.end());
  sort(by_freq.begin(), by_freq.end(), [](auto a, auto b){return a.second > b.second;});
  float prev_freq = 0;
  StatsRow prev = {0,0,0};
  StatsRow next = prev;
  for (auto v: by_freq) {
    if (v.second <= 0) break;
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
  sp.normal.in_corpus.rank = next.normal_incl;
  sp.large.in_corpus.rank  = next.large_incl;

  sp.normal.in_corpus.freq = by_freq[sp.normal.in_corpus.rank-1].second;
  sp.normal.non_filtered.freq = by_freq[sp.normal.non_filtered.rank-1].second;
  sp.normal.total.freq = by_freq[sp.normal.total.rank-1].second;
  sp.large.in_corpus.freq = by_freq[sp.large.in_corpus.rank-1].second;
  sp.large.non_filtered.freq = by_freq[sp.large.non_filtered.rank-1].second;
  sp.large.total.freq = by_freq[sp.large.total.rank-1].second;

  sp.save<L,YF>();

  printf("TOTAL: %u\n", next.rank-1);
}

struct SpellerCount {
  unsigned lower = 0;
  unsigned filtered = 0;
  unsigned total = 0;
  SpellerCount(SpellerDict dict) {
    Table<Speller<Filtered>> speller;
    Table<ToLower>           to_lower;
    unordered_set <LowerId>  collect;
    for (auto v : speller.view) {
      if (v.second <= dict) {
        total++;
        auto lid = to_lower[v.first];
        if (lid != 0) {
          filtered++;
          collect.insert(lid);
        }
      }
    }
    lower = collect.size();
    printf("?? %u %u %u %u\n", dict, lower, filtered, total);
  }
};

int main() {
  SpellerCount normal(SP_NORMAL), large(SP_LARGE);
  stats<Lower,Adjusted>(SpellerStats{{normal.lower,normal.total},{large.lower,large.total}});
  stats<Lower,Current>(SpellerStats{{normal.lower,normal.total},{large.lower,large.total}});
  stats<Filtered,Current>(SpellerStats{{normal.filtered,normal.total},{large.filtered,large.total}});
  stats<Lower,Recent>(SpellerStats{{normal.lower,normal.total},{large.lower,large.total}});
  stats<Filtered,Recent>(SpellerStats{{normal.filtered,normal.total},{large.filtered,large.total}});
  stats<Lower,All>(SpellerStats{{normal.lower,normal.total},{large.lower,large.total}});
  stats<Filtered,All>(SpellerStats{{normal.filtered,normal.total},{large.filtered,large.total}});
}
