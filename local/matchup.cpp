#include <tuple>
#include <unordered_map>
#include <string>
#include <algorithm>

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

struct Mapping {
  char data[256] = {};
  Mapping() {
    auto set = [this](const char * from, char to) {
      for (;*from; ++from) {data[(unsigned char)*from] = to;}
    };
    set("aeiouy", '*');
    set("bfpv", '1');
    set("cgjkqs", '2');
    set("dt", '3');
    set("l", '4');
    set("mn", '5');
    set("r", '6');
  }
  char operator[](char c) const {
    return data[(unsigned char)c];
  }
};
static Mapping mapping;

static inline bool is_vowel(char c) {
  return mapping[c] == '*';
}

bool very_similar(const char * a, const char * b) {
  if (*a != *b) return false;
  ++a, ++b;
  while (*a != '\0' && *b != '\0' && *a == *b) ++a, ++b;
  if (*a == '\0' && *b == '\0') return true;

  // check for special case at e at end
  if (*a == 'e' && a[1] == '\0' && *b == '\0') return true;
  if (*b == 'e' && b[1] == '\0' && *a == '\0') return true;

  // Notw: at this point both a and b are not at the beginning so
  // a[-1] is safe

  // check its a vowel that differs
  if (is_vowel(*a) && is_vowel(*b)) {
    if (strcmp(a+1,b+1) == 0) return true;
  }

  // check for double letter
  if (a[-1] == *a) {
    if (strcmp(a+1,b) == 0) return true;
  }
  if (b[-1] == *b) {
    if (strcmp(a,b+1) == 0) return true;
  }

  // check if there is an extra vowel
  if (is_vowel(*a) && (is_vowel(a[-1]) || is_vowel(a[1]))) {
    if (strcmp(a+1,b) == 0) return true;
  }
  if (is_vowel(*b) && (is_vowel(b[-1]) || is_vowel(b[1]))) {
    if (strcmp(a,b+1) == 0) return true;
  }
  
  return false;
}

// uses a modified soundex algorithm
void get_key(const char * p, string & out) {
  //printf(">in>%s\n", p);
  out.clear();
  if (*p == '\0') {return;}
  out += *p++;
  auto append = [&out](char c) {
    if (out.back() != c)
      out += c;
  };
  while (*p) {
    if (*p == 'e' && p[1] == '\0') {
      ++p;
    } else if ((*p == 'y' || *p == 'w' || *p == 'h') && is_vowel(p[-1]) && is_vowel(p[1])) {
      append(*p); ++p;
    } else {
      char c = mapping[*p];
      if (c)
        append(c);
      ++p;
    }
  }
  //printf(">out>%s\n", out.c_str());
}

struct ByFreq {
  LowerId  lid;
  float    freq;
};

int main() {
  Table<LowerLookup> lookup;
  WordBuffer         buffer;
  buffer.load("words_w_lower.dat");
  WordLookupByStr by_str(buffer, lookup.begin(), lookup.end());
  Table<Freq<Lower,Recent> > freq;

  unordered_map<string,vector<LowerId> > collect;

  std::unordered_multimap<LowerId, LowerId> similar;
  vector<ByFreq> by_freq;

  auto insert = [&](LowerId a, LowerId b) {
    if (freq[a] < freq[b]) {
      if (similar.count(a) == 0)
        by_freq.push_back({a,freq[a]});
      similar.insert({a, b});
    }
  };

  string key;
  for (auto v : lookup.view) {
    const char * word = v.second.str(buffer);
    assert(strlen(word) <= 60);
    get_key(word, key);
    collect[key].push_back(v.first);
    //printf("%s %s\n", word, key.c_str());
  }

  for (auto v : collect) {
    //printf("%s:\n", v.first.c_str());
    for (auto i = v.second.begin(); i < v.second.end(); ++i) {
      for (auto j = i + 1; j < v.second.end(); ++j) {
        if (very_similar(lookup[*i].str(buffer), lookup[*j].str(buffer))) {
          insert(*i, *j);
          insert(*j, *i);
        }
      }
    }
  }
  char word[64];
  for (auto v : lookup.view) {
    const char * orig = v.second.str(buffer);
    assert(strlen(orig) <= 60);
    strcpy(word, orig);
    if (word[0] == '\0' || word[1] == '\0') continue;
    char * p = word + 1;
    while (p[1] != '\0') {
      if (p[0] != p[1]) {
        swap(p[0],p[1]);
        auto itr = by_str.find(word);
        if (itr != by_str.end()) {
          insert(v.first, itr->second);
          //printf("%s %s\n", orig, word);
        }
        swap(p[0],p[1]);
      }
      ++p;
    }
  }

  sort(by_freq.begin(), by_freq.end(), [](auto a, auto b){return a.freq > b.freq;});

  Table<Speller<ByLower> > speller;

  for (auto v : by_freq) {
    auto range = similar.equal_range(v.lid);
    for (auto i = range.first; i != range.second; ++i) {
      auto ratio = freq[i->second]/freq[i->first];
      if (ratio > 8 && speller[i->first] <= SP_NORMAL) 
        printf("%'12.4f %6.1f %s %s\n", 1e6*freq[i->first], ratio,
               lookup[i->first].str(buffer), lookup[i->second].str(buffer));
    }
  }
}
