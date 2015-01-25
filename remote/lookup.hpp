#include <cmph.h>

#include "mmap_vector.hpp"
#include "normalize.hpp"
#include "schema.hpp"
#include "final.hpp"

struct WordInfoLookup {
  MMapVector<uint32_t> table;
  MMapVector<char>     data;

  Normalize normalize;

  FILE * mphf_fd;
  cmph_t * hash;

  WordInfoLookup() : table("ngrams.tbl", MADV_RANDOM), data("ngrams.dat", MADV_RANDOM) {
    mphf_fd = fopen("ngrams.mph", "r");
    assert(mphf_fd);
    hash = cmph_load(mphf_fd);
  }

  char * lower = NULL;
  size_t lower_capacity = 0;
  bool filtered;

  WordInfo * operator()(const char * word, size_t word_size) {
    filtered = false;

    if (lower_capacity < word_size + 1) {
      lower_capacity = word_size + 1;
      lower = (char *)realloc(lower, lower_capacity);
    }
    auto res = normalize(word, word_size, lower);
    if (res <= 0) {
      filtered = true;
      return NULL;
    }

    auto id = cmph_search(hash, lower, res);
    auto pos = table[id];

    if (pos == (uint32_t)-1) {
      return NULL;
    }

    auto i = (WordInfo *)&data[pos];

    if (strcmp(i->word, lower) != 0) {
      return NULL;
    }

    return i;
  }
};

