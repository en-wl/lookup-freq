#include <algorithm>
#include <iconv.h>

static inline int asc_tolower(int c)
{
  return ('A' <= c && c <= 'Z') ? c + 0x20 : c;
}
static inline int asc_islower(int c)
{
  return ('a' <= c && c <= 'z');
}

struct Normalize {
  iconv_t conv;
  Normalize() {conv = iconv_open("ascii//translit", "utf8");};
  int operator()(const char * str, size_t str_size, char * res) {
    if (str_size == 0) {*res = '\0'; return 0;}
    auto res_size = [&](){
      auto in = (char *)str;
      auto in_size = str_size;
      auto out = res;
      auto out_size = str_size + 1;
      iconv(conv, &in, &in_size, &out, &out_size);
      if (in_size != 0) {res[0] = '?'; out = res + 1;}
      *out = '\0';
      return out - res;
    }();
    for (unsigned i = 0; i < res_size; ++i)
      res[i] = asc_tolower(res[i]);
    bool keep = res_size <= 60 && std::all_of(res, res+res_size, [](char c){return asc_islower(c);});
    //bool keep = lower.size > 2 
    //  && asc_islower(lower.front()) && asc_islower(lower.back()) 
    //  && all_of(lower.begin() + 1, lower.end() - 1, [](char c){return asc_islower(c) || c == '\'';});
    return keep ? res_size : -res_size;
  }
};
