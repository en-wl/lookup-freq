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

#include "ngrams.hpp"
#include "util.hpp"

using namespace std;

static Pos separate_pos(const Str & word_pre, Str & word);

int main() {
  Str line, word_pre, word;
  Pos pos = -1;
  Word word_id = -1;
  unsigned year;
  unsigned long freq;
  unsigned books;

  WordBuffer word_buffer;
  WordLookup word_lookup{word_buffer};

  auto totals    = fopen("year_totals.dat", "w");
  auto freqs     = fopen("freq.dat", "w");
  auto freqs_pos = fopen("freq_w_pos.dat", "w");

  while (!feof(stdin)) {
    line.size = getline(&line.data, &line.capacity, stdin);
    char * p = (char *)memchr(line.data, '\t', line.size);
    if (!word_pre.eq(line.data, p)) {
      word_pre.assign(line.data, p);
      pos = separate_pos(word_pre, word);
      if (word.size >= 2 && word.front() == '_' && word.back() == '_' && pos == -1) {
        // special case: total for all words of a given pos and year
        word.back() = '\0';
        pos = lookup_pos(word.data + 1);
        word.back() = '_';
        if (pos != -1) word_id = -1;
      } else if (word.eq(" ", 1)) {
        // special case: total for all words in a given year
        word_id = -2;
      } else {
        word_id = word_lookup.add(word);
      }
    }
    int res = sscanf(p+1, "%u\t%lu\t%u\n",&year,&freq,&books);
    if (res != 3)                 {printf("Bad line: %s", line.data);}
    else if (word_id == (u32)-2)  {FreqRow<Year> d{year,freq}; fwrite(&d,sizeof(d),1,totals);}
    else if (word_id == (u32)-1)  {/* ignore for now */;}
    else if (pos == -1)           {FreqRow<FreqKey> d{{word_id, year}, freq}; fwrite(&d,sizeof(d),1,freqs);}
    else                          {FreqRow<FreqPosKey> d{{word_id, pos, year}, freq}; fwrite(&d,sizeof(d),1,freqs_pos);}
  }
  
  fclose(totals);
  fclose(freqs);
  fclose(freqs_pos);

  word_buffer.save("words.dat");
  {
  }
  
}

static Pos separate_pos(const Str & word_pre, Str & word) {
  auto begin = max(word_pre.begin(), word_pre.end() - 5);
  auto p = word_pre.end() - 1;
  while (p > begin && *p != '_') --p;
  if (*p == '_') {
    const char * pos = p + 1;
    int pos_id = lookup_pos(pos);
    if (pos_id == -1) {
      word = word_pre;
      return -1;
    } else {
      word.assign(word_pre.data, p);
      return pos_id;
    }
  } else {
    word = word_pre;
    return -1;
  }
}


