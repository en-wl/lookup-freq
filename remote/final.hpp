#ifndef final__hpp
#define final__hpp

struct WordInfo {
  float freq;
  float newness;
  struct {
    unsigned rank : 28;
    unsigned dict : 2; // 0 = normal, 1 = large, 2 = reserved, 3 = none
    unsigned normal_incl : 24;
    unsigned skip : 8; // number of bytes to skip to get to next entry 
    unsigned large_incl  : 24;
    unsigned word_len : 6;
    unsigned more : 1; // if there are more words within the same category that follow
  } ;
  char word[]; // note, maxium word size = 60 
};

struct OrigWordInfo {
  float percent;
  struct {
    unsigned skip : 7;
    unsigned more : 1;
  } __attribute__((packed));
  char  word[];
} __attribute__((packed));

#endif
