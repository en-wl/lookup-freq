#include <tuple>

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"

int main() {
  Table<LowerLookup> lookup;
  WordBuffer         buffer;
  buffer.load("words_w_lower.dat");
  for (auto str : lookup) {
    printf("%s\n", str.str(buffer));
  }
}
