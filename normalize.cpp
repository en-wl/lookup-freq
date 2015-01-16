#include <algorithm>

#include <iconv.h>
#include <locale.h>

#include "schema.hpp"
#include "util.hpp"
#include "table.hpp"

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
  word_buffer.load("words_w_speller.dat");
  Table<SpellerLookup> word_lookup_by_id;
  WordLookupByStr buffer_lookup(word_buffer, word_lookup_by_id.begin(), word_lookup_by_id.end());
  TableCreator<ToLower> to_lower;
  WordLookupByStr lower_lookup(word_buffer);
  Str lower;
  {
    const char * str = "<filtered>";
    lower.assign(str, str + strlen(str));
    auto lower_id = lower_lookup.add(lower);
    assert(lower_id == 0);
  }
  // we must set the locale for iconv otherwise it will translit
  // 'é' to '?' instead of 'e' as desired
  setlocale (LC_ALL, "en_US.UTF-8");
  auto conv = iconv_open("ascii//translit", "utf8");

  for (auto word: word_lookup_by_id) {
    auto str = word.str(word_buffer);
    { // iconv
      auto in = (char *)str;
      auto in_size = strlen(str);
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
    bool keep = all_of(lower.begin(), lower.end(), [](char c){return asc_islower(c);});
    //bool keep = lower.size > 2 
    //  && asc_islower(lower.front()) && asc_islower(lower.back()) 
    //  && all_of(lower.begin() + 1, lower.end() - 1, [](char c){return asc_islower(c) || c == '\'';});
    auto lower_id = keep ? lower_lookup.add(lower, buffer_lookup) : 0;
    to_lower.append_row(lower_id);
  }
  word_buffer.save("words_w_lower.dat");
  lower_lookup.save("LowerLookup.dat");
}
