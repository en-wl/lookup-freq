#include <aspell.h>

#include "lookup.hpp"

static inline bool asc_isspace(char c) {return c == ' ' || c == '\t' || c == '\n';}

int main(int argc, char *argv[]) {
  setlocale (LC_ALL, "en_US.UTF-8");
  WordInfoLookup lookup;

  char * line = NULL;
  size_t size = 0;

  AspellConfig * spell_config = new_aspell_config();
  aspell_config_replace(spell_config, "master", "./en-lower.rws");
  aspell_config_replace(spell_config, "sug-mode", "ultra");
  AspellCanHaveError * possible_err = new_aspell_speller(spell_config);
  AspellSpeller * spell_checker = 0;
  if (aspell_error_number(possible_err) != 0) {
    fprintf(stderr, "%s\n", aspell_error_message(possible_err));
    return 1;
  } else {
    spell_checker = to_aspell_speller(possible_err);
  }

  vector<WordInfo *> sugs;
  while (getline(&line, &size, stdin) != -1) {
    char * word = line;
    while (asc_isspace(*word)) ++word;
    size_t word_size = strlen(word);
    while (word_size > 0 && asc_isspace(word[word_size-1])) --word_size;
    word[word_size] = '\0';

    auto i = lookup(word, word_size);

    if (!i) {
      printf("%s:\n", word);
      continue;
    }

    printf("%-20s (%'12.4f)\n", i->word, i->freq*1e6);

    const AspellWordList * suggestions = aspell_speller_suggest(spell_checker,
                                                                i->word, strlen(i->word));
    AspellStringEnumeration * elements = aspell_word_list_elements(suggestions);
    sugs.clear();
    aspell_string_enumeration_next(elements);
    const char * sug;
    while ( (sug = aspell_string_enumeration_next(elements)) != NULL )
    {
      if (strcmp(word, sug)==0) continue;
      auto sz = strcspn(sug, " -");
      if (sug[sz] != '\0') continue;
      auto j = lookup(sug, sz);
      if (j)
        sugs.push_back(&*j);
    }
    sort(sugs.begin(), sugs.end(), [](auto a, auto b){return a->freq > b->freq;});
    for (auto wi : sugs) {
      float ratio = wi->freq/i->freq;
      if (ratio >= 0.5) {
        printf("  %-19s %'12.1fx\n", wi->word, ratio);
      }
    }
    delete_aspell_string_enumeration(elements);    
  }

  return 0;
}
