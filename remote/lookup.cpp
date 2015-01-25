#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <math.h>

#include <vector>
#include <algorithm>

#include "final.hpp"
#include "mmap_vector.hpp"
#include "normalize.hpp"
#include "schema.hpp"
#include "lookup.hpp"

static inline bool asc_isspace(char c) {return c == ' ' || c == '\t' || c == '\n';}

const char * score(float diff, float per) {
  if      (diff < -4)  return "*";
  else if (diff < -1)  return "**";
  else if (diff <  2)  return "***";
  else if (per < 99.0) return "****";
  else                 return "*****";
};

enum Level {BRIEF, NORMAL, FULL};

struct SpInfo {
  static const unsigned normal = 0;
  static const unsigned large = 1;
  double cutoff_freq;
  double cutoff_rank;
  unsigned num;
};

struct MoreStats {
  double diff;
  double rank_per;
  double total_per;
  const char * score;
  const char * found;
  const char * found_or_score() const { return found ? found : score; }
  MoreStats(const WordInfo * wi, double incl, const SpInfo & sp) {
    diff = log(wi->freq/sp.cutoff_freq)/log(2);
    rank_per = 100.0*incl/wi->rank;
    total_per = 100.0*(1-incl/sp.cutoff_rank);
    score = ::score(diff, rank_per);
    found = wi->dict <= sp.num ? "incl." : NULL;
  }
};
#define MORE_STATS(wi, dict) MoreStats(wi, wi->dict##_incl, spi[SpInfo::dict])

class Pad {
  char buf[64];
public:
  const char * operator()(const char * word, unsigned width) {
    assert(width <= 60);
    auto sz = strlen(word);
    if (sz >= width) return word;
    int pad = width - mbstowcs(NULL, word, 0);
    memcpy(buf, word, sz);
    for (;pad > 0;--pad)
      buf[sz++] = ' ';
    buf[sz] = '\0';
    return buf;
  };
};

int main(int argc, char *argv[]) {
  setlocale (LC_ALL, "en_US.UTF-8");
  //sp.load<Lower,Recent>();
  WordInfoLookup lookup;
  SpellerStats sp;
  sp.load<Lower,Current>();
  SpInfo spi[2] = {{sp.normal.non_filtered.freq, sp.normal.non_filtered.rank, 0},
                   {sp.large.non_filtered.freq, sp.large.non_filtered.rank, 1}};
  
  auto level = NORMAL;
  if      (argc >= 2 && strcmp(argv[1], "brief") == 0)  level = BRIEF;
  else if (argc >= 2 && strcmp(argv[1], "normal") == 0) level = NORMAL;
  else if (argc >= 2 && strcmp(argv[1], "full") == 0)   level = FULL;

  bool do_report = false;
  bool with_incl = true;
  if (argc >= 3 && strcmp(argv[2], "report-w-incl") == 0) do_report = true;
  if (argc >= 3 && strcmp(argv[2], "report-wo-incl") == 0) {do_report = true; with_incl = false;}
  if (do_report && level == BRIEF) {
    fprintf(stderr, "Can not crete a 'brief' report.\n");
    exit(1);
  }

  if (level == BRIEF) {
    setlinebuf(stdout);
  }

  char * line = NULL;
  size_t size = 0;

  Pad pad;

  std::vector<WordInfo *>   result;
  std::vector<const char *> filtered;
  std::vector<const char *> not_found;

  while (!do_report && getline(&line, &size, stdin) != -1) {
    char * word = line;
    while (asc_isspace(*word)) ++word;
    size_t word_size = strlen(word);
    while (word_size > 0 && asc_isspace(word[word_size-1])) --word_size;
    word[word_size] = '\0';

    WordInfo * i = lookup(word, word_size);

    if (!i && lookup.filtered) {
      if (level == BRIEF) {
        printf("%s |              |       |\n", pad(word,20));
      } else {
        filtered.push_back(word);
        line = NULL;
      }
    } else if (!i) {
      if (level == BRIEF) {
        printf("%s |       0      | %-5s | %-5s\n", pad(word,20), "*", "*");
      } else {
        not_found.push_back(word);
        line = NULL;
      }
    } else {
      if (level == BRIEF) {
        printf("%s | %'12.4f | %-5s | %-5s\n", pad(word,20), 
               i->freq*1e6,
               MORE_STATS(i, normal).score, MORE_STATS(i, large).score);
      } else {
        result.push_back(i);
      }
    }
  }

  std::sort(result.begin(), result.end(), [](auto a, auto b){return a < b;});

  auto write_header = [&](FILE * out) {
    if (level == NORMAL) {
      fprintf(out, "Word                 |  Freq           Rank | Normal dict | Large dict\n");
      fprintf(out, "                     |  (per million)       | should incl | should incl\n");
      fprintf(out, "---------------------|----------------------|-------------|-------------\n");
    } else if (level == FULL) {
      fprintf(out, "Word                 |  Freq           Rank |     Normal dictionary stats       |     Large dictionary stats\n");
      fprintf(out, "                     |  (per million)       | F Score  Dist Coverg Positn  Incl | F Score  Dist Coverg Positn  Incl\n");
      fprintf(out, "---------------------|----------------------|-----------------------------------|-----------------------------------\n");
    }
  };
  auto use_stdout = [](auto wi, auto normal, auto large){return stdout;};
  auto write_entry = [&](char * pos, auto outf) {
    auto i = (WordInfo *)pos;
    auto normal = MORE_STATS(i, normal);
    auto large  = MORE_STATS(i, large);
    auto out = outf(i,normal,large);
    if (level == NORMAL)
      fprintf(out, "%-20s | %'12.4f %7u |    %-5s    |    %-5s\n", 
              i->word, i->freq*1e6, i->rank, normal.found_or_score(), large.found_or_score());
    else
      fprintf(out, "%-20s | %'12.4f %7u | %c %-5s %+5.1f %6.2f %6.2f %5u | %c %-5s %+5.1f %6.2f %6.2f %6u\n",
              i->word, i->freq*1e6, i->rank,
              normal.found ? 'Y' : '-', normal.score, normal.diff, normal.rank_per, normal.total_per, i->normal_incl,
              large.found ? 'Y' : '-', large.score, large.diff, large.rank_per, large.total_per, i->large_incl);

    auto lower = i; 
    
    while (level >= NORMAL && i->more) {
      pos += i->skip;
      i = (WordInfo *)pos;
      auto freq_per = 100*i->freq/lower->freq;
      if (freq_per > 1.0) {
        fprintf(out,"  %s | %3.0f%%", pad(i->word, 18), freq_per);
        if (level == NORMAL)
          fprintf(out,"                 |             |\n");
        else
          fprintf(out,"                 |                                   |\n");
      }
    }
    return pos;
  };

  if (do_report) {

    char * pos = lookup.data.data;
    FILE * devnull = fopen("/dev/null", "w");
    write_header(stdout);
    for (;;) {
      pos = write_entry(pos, [&](auto wi, auto normal, auto large){
          if (strlen(large.score) >= 3 && (with_incl || !normal.found)) return stdout;
          else return devnull;
        });
      pos += reinterpret_cast<WordInfo *>(pos)->skip;
      if (pos == lookup.data.end()) break;
    }

  } else {

    write_header(stdout);

    WordInfo * prev = NULL;
    for (auto i : result) {
      if (i == prev) continue;
      prev = i;
      char * pos = (char *)i;
      write_entry(pos, use_stdout);
    }

    for (auto str : not_found) {
      if (level == NORMAL)
        printf("%s |       0              |    *        |    *\n", pad(str, 20));
      else
        printf("%s |       0              |   *                               |   *\n",
               pad(str,20));
    }

    for (auto str : filtered) {
      if (level == NORMAL)
        printf("%s |   <FILTERED>         |             |\n", pad(str, 20));
      else
        printf("%s |   <FILTERED>         |                                   |\n",
               pad(str,20));
    }

  }
}
