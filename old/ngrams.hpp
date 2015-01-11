#ifndef ngrams__hpp
#define ngrams__hpp

#include <string.h>

#include <array>
using std::array;

typedef unsigned short u16;
typedef unsigned       u32;
typedef unsigned long  u64;

struct Word {
  u32 id;
  const char * str(const u32 * offsets, const char * buffer) const {return buffer + offsets[id];}
};

typedef short Pos;
typedef u16 Year;

struct FreqKey {
  Word word;
  Year year;
};

struct FreqPosKey {
  Word word;
  Pos  pos;
  Year year;
};

template <typename Key>
struct FreqRow {
  Key key;
  float freq;
  //float books;
};


static const array<const array<const char,8>,13> poses {
  "", "NOUN", "VERB", "ADJ", "ADV", "PRON", 
    "DET", "ADP", "CONJ", "PRT", 
    "X", "NUM",  "."
};
static inline int lookup_pos(const char * pos) {
  for (int i = 0; i != poses.size(); ++i) {
    if (strcmp(poses[i].data(),pos)==0) return i;
  }
  return -1;
}

#endif

