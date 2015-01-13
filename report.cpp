#include <algorithm>
#include <vector>
#include <tuple>

using namespace std;

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

template <typename T, typename Lookup> 
void dump() {
  Table<T>      freq;
  Table<Lookup> lookup;
  WordBuffer    buffer;
  buffer.load("words_w_lower.dat");
  vector<pair<unsigned,float>> by_freq(freq.view.begin(), freq.view.end());
  sort(by_freq.begin(), by_freq.end(), [](auto a, auto b){return a.second > b.second;});
  for (auto v: by_freq) {
    printf("%s %f\n", lookup[v.first].str(buffer), v.second);
  }  
}

int main() {
    dump<FreqLower<Recent>,LowerLookup>();
}
