#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <math.h>

#include <aspell.h>

#include <vector>
#include <algorithm>
#include <string>

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

bool non_ascii(unsigned char c) {return c >= 127;}

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

enum AlsoReport {NEITHER, SIMILAR, ORIGINAL};

int main(int argc, char *argv[]) {
  setlocale (LC_ALL, "en_US.UTF-8");
  //sp.load<Lower,Recent>();
  WordInfoLookup lookup;
  SpellerStats sp;
  sp.load<Lower,Current>();
  SpInfo spi[2] = {{sp.normal.non_filtered.freq, sp.normal.non_filtered.rank, 0},
                   {sp.large.non_filtered.freq, sp.large.non_filtered.rank, 1}};
  
  auto level = NORMAL;
  unsigned check_limit = 1000;
  if      (argc >= 2 && strcmp(argv[1], "brief") == 0)  {level = BRIEF;}
  else if (argc >= 2 && strcmp(argv[1], "normal") == 0) {level = NORMAL;}
  else if (argc >= 2 && strcmp(argv[1], "full") == 0)   {level = FULL;}
  else if (argc >= 2) {printf("invalid argument, pos 1\n"); exit(1);}

  auto also_report = SIMILAR;
  if      (argc >= 3 && strcmp(argv[2], "similar") == 0)  {also_report = SIMILAR;}
  else if (argc >= 3 && strcmp(argv[2], "original") == 0) {also_report = ORIGINAL;}
  else if (argc >= 3 && strcmp(argv[2], "neither") == 0)  {also_report = NEITHER;}
  else if (argc >= 3) {printf("invalid argument, pos 2\n"); exit(1);}

  bool do_report = false;
  bool with_incl = true;
  bool strong = false;
  if      (argc >= 4 && strcmp(argv[3], "report-w-incl") == 0) do_report = true;
  else if (argc >= 4 && strcmp(argv[3], "report-wo-incl") == 0) {do_report = true; with_incl = false;}
  else if (argc >= 4 && strcmp(argv[3], "report-strong") == 0) {do_report = true; with_incl = false; strong = true;}
  else if (argc >= 4) {printf("invalid argument, pos 3\n"); exit(1);}
  if (do_report) check_limit = -1;
  if (do_report && level == BRIEF) {
    fprintf(stderr, "Can not crete a 'brief' report.\n");
    exit(1);
  }

  AspellSpeller * spell_checker = 0;
  vector<WordInfo *> sugs;
  if (also_report == SIMILAR) {
    AspellConfig * spell_config = new_aspell_config();
    aspell_config_replace(spell_config, "master", "./en-lower.rws");
    aspell_config_replace(spell_config, "sug-mode", "ultra");
    AspellCanHaveError * possible_err = new_aspell_speller(spell_config);
    
    if (aspell_error_number(possible_err) != 0) {
      fprintf(stderr, "%s\n", aspell_error_message(possible_err));
      return 1;
    } else {
      spell_checker = to_aspell_speller(possible_err);
    }
  }

  if (level == BRIEF) {
    setlinebuf(stdout);
  }

  char * line = NULL;
  size_t size = 0;

  bool reached_check_limit = false;

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
        printf("%s |              |      |       |\n", pad(word,20));
      } else {
        filtered.push_back(word);
        line = NULL;
      }
    } else if (!i) {
      if (level == BRIEF) {
        printf("%s |       0      |      | %-5s | %-5s\n", pad(word,20), "*", "*");
      } else {
        not_found.push_back(word);
        line = NULL;
      }
    } else {
      if (level == BRIEF) {
        printf("%s | %'12.4f | %4.1f | %-5s | %-5s\n", pad(word,20), 
               i->freq*1e6, i->newness,
               MORE_STATS(i, normal).score, MORE_STATS(i, large).score);
      } else {
        result.push_back(i);
      }
    }
  }

  std::sort(result.begin(), result.end(), [](auto a, auto b){return a < b;});

  auto write_header = [&](FILE * out) {
    auto what = (also_report == SIMILAR ? "similar words" :
                 also_report == ORIGINAL ? "original words" : "");
    if (level == NORMAL) {
      fprintf(out, "Word                 |  Adj. Freq   Newness Rank | Normal dict | Large dict\n");
      fprintf(out, "  %-19s|  (per million)            | should incl | should incl\n", what);
      fprintf(out, "---------------------|---------------------------|-------------|-------------\n");
    } else if (level == FULL) {
      fprintf(out, "Word                 |  Adj. Freq   Newness Rank |     Normal dictionary stats       |     Large dictionary stats\n");
      fprintf(out, "  %-19s|  (per million)            | F Score  Dist Coverg Positn  Incl | F Score  Dist Coverg Positn  Incl\n",what);
      fprintf(out, "---------------------|---------------------------|-----------------------------------|-----------------------------------\n");
    }
  };
  FILE * devnull = fopen("/dev/null", "w");
  unsigned checked=0;
  string repr;
  auto use_ = [](auto out){return [out](auto wi, auto normal, auto large){return out;};};
  auto use_stdout = use_(stdout);
  auto write_entry = [&](char * pos, auto outf) {
    auto i = (WordInfo *)pos;
    auto normal = MORE_STATS(i, normal);
    auto large  = MORE_STATS(i, large);
    auto out = outf(i,normal,large);

    if (!reached_check_limit && checked > check_limit) {
      reached_check_limit = true;
      fprintf(out, "*** Too many words: only showing similar words for the %u most frequent entries. ***\n\n",
              check_limit);
    }

    auto get_repr = [&repr](WordInfo * wi) {
      const char * lower = wi->word;
      auto p = (const char *)wi + wi->skip;
      auto i = (OrigWordInfo *)p;
      repr = lower;
      for (unsigned j = 0; j < repr.size(); ++j) {
        if (non_ascii(i->word[j])) break;
        repr[j] = i->word[j];
      }
      float percent_so_far = i->percent;
      if (percent_so_far < 87.5 && i->more) {
        for (;;) {
          p += i->skip;
          i = (OrigWordInfo *)p;
          for (unsigned j = 0; j < repr.size(); ++j) {
            if (non_ascii(i->word[j])) break;
            if (asc_islower(i->word[j])) repr[j] = i->word[j];
          }
          percent_so_far += i->percent;
          if (!i->more || percent_so_far >= 87.5) break;
        }
      }
    };
    get_repr(i);
    fprintf(out, "%-20s | %'12.4f ", repr.c_str(), i->freq*1e6);

    auto write_rest = [level,out](auto i, auto normal, auto large) {
      if (level == NORMAL) 
        fprintf(out, "%4.1f %7u |    %-5s    |    %-5s\n", 
                i->newness, i->rank, normal.found_or_score(), large.found_or_score());
      else
        fprintf(out, "%4.1f %7u | %c %-5s %+5.1f %6.2f %6.2f %5u | %c %-5s %+5.1f %6.2f %6.2f %6u\n",
                i->newness, i->rank,
                normal.found ? 'Y' : '-', normal.score, normal.diff, normal.rank_per, normal.total_per, i->normal_incl,
                large.found ? 'Y' : '-', large.score, large.diff, large.rank_per, large.total_per, i->large_incl);
    };
    write_rest(i, normal, large);

    pos += i->skip;

    bool more = true;
    for (OrigWordInfo * i = NULL;more && level >= NORMAL;) {
      i = (OrigWordInfo *)pos;
      auto freq_per = i->percent;
      if (also_report == ORIGINAL && freq_per > 1.0) {
        fprintf(out,"  %s | %3.0f%%", pad(i->word, 18), freq_per);
        if (level == NORMAL)
          fprintf(out,"                      |             |\n");
        else
          fprintf(out,"                      |                                   |\n");
      }
      more = i->more;
      pos += i->skip;
    }
    if (!reached_check_limit && also_report == SIMILAR && out != devnull) {
      checked++;
      const AspellWordList * suggestions = aspell_speller_suggest(spell_checker,
                                                                  i->word, strlen(i->word));
      AspellStringEnumeration * elements = aspell_word_list_elements(suggestions);
      sugs.clear();
      aspell_string_enumeration_next(elements);
      const char * sug;
      while ( (sug = aspell_string_enumeration_next(elements)) != NULL )
      {
        if (strcmp(i->word, sug)==0) continue;
        auto sz = strcspn(sug, " -");
        if (sug[sz] != '\0') continue;
        auto j = lookup(sug, sz);
        if (j)
        sugs.push_back(&*j);
      }
      sort(sugs.begin(), sugs.end(), [](auto a, auto b){return a->freq > b->freq;});

      for (auto wi : sugs) {
        get_repr(wi);
        float ratio = wi->freq/i->freq;
        if (ratio >= 0.5) {
          fprintf(out,"  %-18s |%'9.1fx    ", repr.c_str(), ratio);
          write_rest(wi, MORE_STATS(wi, normal), MORE_STATS(wi, large));
        }
      }
      fprintf(out, "\n");
    }
    return pos;
  };

  if (do_report) {

    char * pos = lookup.data.data;
    write_header(stdout);
    for (;;) {
      pos = write_entry(pos, [&](auto wi, auto normal, auto large) {
          if (!strong 
              && strlen(large.score) >= 3 
              && (with_incl || !normal.found)) 
            return stdout;
          else if (strong 
                   && !normal.found 
                   && (strlen(normal.score) >= 4 
                       || (strlen(normal.score) >= 3 && large.found))) 
            return stdout;
          else return devnull;
        });
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
        printf("%s |       0                   |    *        |    *\n", pad(str, 20));
      else
        printf("%s |       0                   |   *                               |   *\n",
               pad(str,20));
    }

    for (auto str : filtered) {
      if (level == NORMAL)
        printf("%s |   <FILTERED>              |             |\n", pad(str, 20));
      else
        printf("%s |   <FILTERED>              |                                   |\n",
               pad(str,20));
    }

  }
}
