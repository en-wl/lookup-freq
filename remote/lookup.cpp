#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <math.h>

#include <cmph.h>

#include <vector>
#include <algorithm>

#include "final.hpp"
#include "mmap_vector.hpp"
#include "normalize.hpp"
#include "schema.hpp"

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

int main(int argc, char *argv[]) {
  
  auto level = NORMAL;
  if      (argc == 2 && strcmp(argv[1], "brief") == 0)  level = BRIEF;
  else if (argc == 2 && strcmp(argv[1], "normal") == 0) level = NORMAL;
  else if (argc == 2 && strcmp(argv[1], "full") == 0)   level = FULL;

  if (level == BRIEF) {
    setlinebuf(stdout);
  }

  MMapVector<uint32_t> table("ngrams.tbl", MADV_RANDOM);
  MMapVector<char>     data("ngrams.dat", MADV_RANDOM);
  SpellerStats         sp;
  sp.load<Lower,Recent>();
  SpInfo spi[2] = {{sp.normal.non_filtered.freq, sp.normal.non_filtered.rank, 0},
                   {sp.large.non_filtered.freq, sp.large.non_filtered.rank, 1}};

  auto mphf_fd = fopen("ngrams.mph", "r");
  assert(mphf_fd);
  auto hash = cmph_load(mphf_fd);

  setlocale (LC_ALL, "en_US.UTF-8");
  Normalize normalize;

  char * line = NULL;
  size_t size = 0;
  char * lower = NULL;
  size_t lower_capacity = 0;

  std::vector<WordInfo *>   result;
  std::vector<const char *> filtered;
  std::vector<const char *> not_found;

  char buf[64];
  auto pad = [&buf](const char * word, unsigned width) -> const char * {
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
  
  while (getline(&line, &size, stdin) != -1) {
    char * word = line;
    while (asc_isspace(*word)) ++word;
    size_t word_size = strlen(word);
    while (word_size > 0 && asc_isspace(word[word_size-1])) --word_size;
    word[word_size] = '\0';

    if (lower_capacity < word_size + 1) {
      lower_capacity = word_size + 1;
      lower = (char *)realloc(lower, lower_capacity);
    }
    auto res = normalize(word, word_size, lower);
    if (res <= 0) {
      if (level == BRIEF) {
        printf("%s |              |       |\n", pad(word,20));
      } else {
        filtered.push_back(word);
        line = NULL;
      }
      continue;
    }

    auto id = cmph_search(hash, lower, res);
    auto pos = table[id];

    auto handle_not_found = [&](){
      if (level == BRIEF) {
        printf("%s |       0      | %-5s | %-5s\n", pad(word,20), "*", "*");
      } else {
        not_found.push_back(word);
        line = NULL;
      }
    };

    if (pos == (uint32_t)-1) {
      handle_not_found();
      continue;
    }

    auto i = (WordInfo *)&data[pos];

    if (strcmp(i->word, lower) != 0) {
      handle_not_found();
      continue;
    }

    if (level == BRIEF) {
      printf("%s | %'12.4f | %-5s | %-5s\n", pad(word,20), 
             i->freq*1e6,
             MORE_STATS(i, normal).score, MORE_STATS(i, large).score);
    } else {
      result.push_back(i);
    }
  }

  std::sort(result.begin(), result.end(), [](auto a, auto b){return a->freq > b->freq;});

  if (level == NORMAL) {
    printf("Word                 |  Freq           Rank | Normal dict | Large dict\n");
    printf("                     |  (per million)       | should incl | should incl\n");
    printf("---------------------|----------------------|-------------|-------------\n");
  } else if (level == FULL) {
    printf("Word                 |  Freq (ppm)     Rank |     Normal dictionary stats       |     Large dictionary stats\n");
    printf("---------------------|----------------------|-----------------------------------|-----------------------------------\n");
  }

  WordInfo * prev = NULL;
  for (auto i : result) {
    if (i == prev) continue;
    prev = i;
    char * pos = (char *)i;
    {
      auto normal = MORE_STATS(i, normal);
      auto large  = MORE_STATS(i, large);
      if (level == NORMAL)
        printf("%-20s | %'12.4f %7u |    %-5s    |    %-5s\n", 
               i->word, i->freq*1e6, i->rank, normal.found_or_score(), large.found_or_score());
      else
        printf("%-20s | %'12.4f %7u | %c %-5s %+5.1f %6.2f %6.2f %5u | %c %-5s %+5.1f %6.2f %6.2f %6u\n",
               i->word, i->freq*1e6, i->rank,
               normal.found ? 'Y' : '-', normal.score, normal.diff, normal.rank_per, normal.total_per, i->normal_incl,
               large.found ? 'Y' : '-', large.score, large.diff, large.rank_per, large.total_per, i->large_incl);
    }

    auto lower = i; 
    
    while (level >= NORMAL && i->more) {
      pos += i->skip;
      i = (WordInfo *)pos;
      auto freq_per = 100*i->freq/lower->freq;
      if (freq_per > 1.0) {
        printf("  %s | %3.0f%%", pad(i->word, 18), freq_per);
        if (level == NORMAL)
          puts("                 |             |");
        else
          puts("                 |                                   |");
      }
    }

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
