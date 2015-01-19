#include <algorithm>

#include <iconv.h>
#include <locale.h>

#include "schema.hpp"
#include "util.hpp"
#include "table.hpp"

struct SpellerInfo {
  WordId  word_id;
  unsigned onum;
};

int main() {
  WordBuffer word_buffer;
  word_buffer.load("words.dat");
  Table<WordLookup> word_lookup_by_id;
  WordLookupByStr buffer_lookup(word_buffer, word_lookup_by_id.begin(), word_lookup_by_id.end());
  vector<SpellerInfo> tmp;
  Str line, word;
  auto fd = fopen("speller.tab","r");
  for (ssize_t res; res = getline(&line.data, &line.capacity, fd), res != -1;) {
    SpellerInfo spi;
    line.size = res;
    char * p = (char *)memchr(line.data, '\t', line.size);
    word.assign(line.data, p);
    spi.word_id = buffer_lookup.add(word);
    int res2 = sscanf(p+1, "%u\n",&spi.onum);
    assert(res2 == 1);
    tmp.push_back(spi);
  }
  TmpTable<Speller<ByWord>> speller(buffer_lookup.size(), SP_NONE);
  for (auto spi : tmp) {
    assert(speller[spi.word_id] == SP_NONE);
    speller[spi.word_id] = static_cast<SpellerDict>(spi.onum);
  }
  save_memory(Speller<ByWord>::fn(), &*speller.begin(), &*speller.end());
  word_buffer.save("words_w_speller.dat");
  buffer_lookup.save(SpellerLookup::fn());
};

