
#include <assert.h>

#include "schema.hpp"

const array<const char[8],12> Pos::to_str {
  "NOUN", "VERB", "ADJ", "ADV", "PRON", 
    "DET", "ADP", "CONJ", "PRT", 
    "X", "NUM",  "."};


void sanity() __attribute__((constructor));
void sanity() 
{
  assert(Pos(Pos::POS_X) == Pos("X"));
}
