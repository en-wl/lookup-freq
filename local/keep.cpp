#include <algorithm>

#include <iconv.h>
#include <locale.h>

#include "ngrams.hpp"
#include "util.hpp"
#include "mmap_vector.hpp"

static inline int asc_tolower(int c)
{
  return ('A' <= c && c <= 'Z') ? c + 0x20 : c;
}
static inline int asc_islower(int c)
{
  return ('a' <= c && c <= 'z');
}

int main() {
  WordBuffer word_buffer;
  word_buffer.load("words.dat");
  WordLookup word_lookup(word_buffer);
  MMapVector<u32> offsets("word_lookup.dat");
  MMapVector<u32> lower("word_lower.dat");
  Str lower;
  // we must set the locale for iconv otherwise it will translit
  // 'é' to '?' instead of 'e' as desired
  setlocale (LC_ALL, "en_US.UTF-8");
  auto conv = iconv_open("ascii//translit", "utf8");
  auto lower_map = fopen("word_lower.dat", "w");
  auto keep_map = fopen("word_keep.dat", "w");

  for (auto offset: offsets) {
    auto & word_info = *word_lookup.find(word_buffer.begin + offset);
    { // iconv
      auto in = (char *)word_info.first;
      auto in_size = strlen(word_info.first);
      lower.size = 0;
      while (in_size != 0) {
        auto out = lower.data + lower.size;
        auto out_size = lower.capacity - 1 - lower.size;
        iconv(conv, &in, &in_size, &out, &out_size);
        lower.size = out - lower.data;
        *out = '\0';
        if (in_size != 0) lower.ensure_space(lower.capacity * 2);
      }
    }
    { // to_lower
      for (unsigned i = 0; i < lower.size; ++i)
        lower[i] = asc_tolower(lower[i]);
    }
    auto lower_p = word_lookup.find(lower.data);
    if (lower_p == word_lookup.end()) {
      // if not found insert lowercase word
      lower_p = word_lookup.insert({word_buffer.append(lower), word_lookup.size()}).first;
    }
    // store mapping to lower case word
    fwrite(&lower_p->second, sizeof(lower_p->second), 1, lower_map);
  }
  // store identity mapping for newly inserted lowercase words
  for (u32 i = offsets.size; i != word_lookup.size(); ++i)
    fwrite(&i, sizeof(i), 1, lower_map);
  word_buffer.save("words.dat");
  word_lookup.save("word_lookup.dat");
}
