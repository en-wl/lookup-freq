#include <algorithm>

#include <iconv.h>
#include <locale.h>

#include "schema.hpp"
#include "util.hpp"
#include "table.hpp"

int main() {
  Table<Speller<ByWord>> speller;
  Table<ToLower> to_lower;
  Table<LowerLookup> lower_lookup;
  TmpTable<Speller<ByLower>> speller_lower(lower_lookup.size(), SP_NONE);

  for (auto row : speller.view) {
    auto lid = to_lower[row.first];
    speller_lower[lid] = min(speller_lower[lid],row.second);
  }

  save_memory(Speller<Lower>::fn, &*speller_lower.begin(), &*speller_lower.end());
}
