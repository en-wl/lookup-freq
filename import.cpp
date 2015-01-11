#include <tuple>
#include <utility>
#include <algorithm>
#include <unordered_map>

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "schema.hpp"
#include "util.hpp"
#include "table.hpp"

using namespace std;

static Pos separate_pos(const Str & word_pre, Str & word);

int main() {
  Str line, word_pre, word;
  Pos pos;
  WordId word_id = -1;
  unsigned year;
  unsigned long freq;
  unsigned books;

  WordBuffer word_buffer;
  WordLookup word_lookup{word_buffer};

  TableCreator<Freqs>      freqs;
  TableCreator<FreqsPos>   freqs_pos;
  TableCreator<PosTotals>  pos_totals;
  TableCreator<Totals>     totals;

  while (!feof(stdin)) {
    line.size = getline(&line.data, &line.capacity, stdin);
    char * p = (char *)memchr(line.data, '\t', line.size);
    if (!word_pre.eq(line.data, p)) {
      word_pre.assign(line.data, p);
      pos = separate_pos(word_pre, word);
      if (word.size >= 2 && word.front() == '_' && word.back() == '_' && !pos.defined()) {
        // special case: total for all words of a given pos and year
        word.back() = '\0';
        pos = Pos(word.data + 1);
        word.back() = '_';
        if (pos.defined()) word_id = -1;
      } else if (word.eq(" ", 1)) {
        // special case: total for all words in a given year
        word_id = -2;
      } else {
        word_id = word_lookup.add(word);
      }
      //printf("%s %u %u\n", word.data, word_id, pos.id);
    }
    int res = sscanf(p+1, "%u\t%lu\t%u\n",&year,&freq,&books);
    if (res != 3)                 printf("Bad line: %s", line.data);
    else if (word_id == WordId(-2)) totals.append_row({year, freq});
    else if (word_id == WordId(-1)) pos_totals.append_row({year, pos, freq});
    else if (pos.defined())         freqs_pos.append_row({word_id, pos, year, freq});
    else                            freqs.append_row({word_id, year, freq});
  }
  
  word_buffer.save("words.dat");
  word_lookup.save("word_lookup.dat");
}

static Pos separate_pos(const Str & word_pre, Str & word) {
  auto begin = max(word_pre.begin(), word_pre.end() - 5);
  auto p = word_pre.end() - 1;
  while (p > begin && *p != '_') --p;
  if (*p == '_') {
    Pos pos(p+1);
    if (pos.defined()) {
      word.assign(word_pre.data, p);
      return pos;
    } else {
      word = word_pre;
      return Pos();
    }
  } else {
    word = word_pre;
    return Pos();
  }
}


