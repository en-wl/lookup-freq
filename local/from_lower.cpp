#include <algorithm>

#include "schema.hpp"
#include "util.hpp"
#include "table.hpp"

int main() {
  {
    Table<ToLower> to_lower;
    TableCreator<FromLower> from_lower;
    
    for (auto row : to_lower.view) {
      from_lower.append_row({row.second, row.first});
    }
  }
  {
    MutTable<FromLower> from_lower;
    std::sort(from_lower.begin(), from_lower.end(), [](auto a, auto b){return a.lid < b.lid;});
  }
}
