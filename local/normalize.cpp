#include <algorithm>

#include <locale.h>

#include "schema.hpp"
#include "util.hpp"
#include "table.hpp"
#include "normalize.hpp"

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
  Normalize normalize;

  for (auto word: word_lookup_by_id) {
    auto str = word.str(word_buffer);
    auto str_size = strlen(str);
    lower.ensure_space(str_size + 1);
    auto res = normalize(str, str_size, lower.data);
    //printf(">>%s => %s\n", str, lower.data);
    bool keep = res > 0;
    lower.size = abs(res);
    auto lower_id = keep ? lower_lookup.add(lower, buffer_lookup) : 0;
    to_lower.append_row(lower_id);
  }
  word_buffer.save("words_w_lower.dat");
  lower_lookup.save("LowerLookup.dat");
}
