#ifndef schema__hpp
#define schema__hpp

#include <assert.h>
#include <string.h>

#include <array>

using namespace std;

typedef uint32_t WordId;
typedef uint32_t LowerId;
typedef uint16_t Year;

struct Pos {
  uint16_t id;
  bool defined() {return id != (uint16_t)-1;}
  Pos() : id(-1) {}
  explicit Pos(uint16_t id) : id(id) {}
  explicit Pos(const char * str) : id(from_str(str)) {}
  const char * str() const {return to_str[id];}
  static const uint16_t POS_X = 9;
  bool exclude() const {return id >= POS_X;}
private:
  uint16_t from_str(const char * pos) const {
    for (unsigned i = 0; i != to_str.size(); ++i) {
      if (strcmp(to_str[i],pos)==0) return i;
    }
    return -1;
  }
  static const array<const char[8],12> to_str;
};
inline bool operator==(Pos a, Pos b) {return a.id == b.id;}

struct Word {
  uint32_t offset;
  const char * str(const char * buffer) const {return buffer + offset;}
};

template <typename T = size_t>
struct SimpleIndex {
  typedef T Idx;
  static size_t idx(Idx i) {return i;}
  static Idx    key(size_t t) {return static_cast<Idx>(t);}
};

struct WordLookupById : SimpleIndex<WordId> {
  static constexpr const char * fn = "word_lookup.dat";
  typedef Word Row;
};

struct ToLower : SimpleIndex<WordId> {
  static constexpr const char * fn = "word_lower.dat";
  typedef LowerId Row;
};

struct LowerLookup : SimpleIndex<LowerId> {
  static constexpr const char * fn = "lower_lookup.dat";
  typedef Word Row;
};

struct Freqs : SimpleIndex<> {
  static constexpr const char * fn = "freq.dat";
  struct Row {
    WordId wid;
    Year   year;
    float  freq;
  };
};

struct FreqsPos : SimpleIndex<> {
  static constexpr const char * fn = "freq_w_pos.dat";
  struct Row {
    WordId wid;
    Pos    pos;
    Year   year;
    float  freq;
  };
};

struct PosTotals : SimpleIndex<> {
  static constexpr const char * fn = "pos_totals.dat";
  struct Row {
    Year   year;
    Pos    pos;
    double freq;
  };
};

struct Totals : SimpleIndex<> {
  static constexpr const char * fn = "totals.dat";
  struct Row {
    Year   year;
    double freq;
  };
};

template <const char * FN>
struct FreqsLower : SimpleIndex<> {
  static constexpr const char * fn = FN;
  struct Row {
    LowerId lid;
    Year    year;
    float   freq;
  };
};

struct YearIndex {
  static const Year MIN_YEAR = 1700;
  static size_t idx(Year y) {assert(y >= MIN_YEAR); return y-MIN_YEAR;}
  static Year   key(size_t i) {return i + MIN_YEAR;}
  typedef Year Idx;
};

struct Counted : YearIndex {
  static constexpr const char * fn = "counted.dat";
  typedef double Row;
};

struct FreqAll : SimpleIndex<LowerId> {
  static constexpr const char * fn = "freq_all.dat";
  typedef float Row;
};

struct FreqRecent : SimpleIndex<LowerId> {
  static constexpr const char * fn = "freq_recent.dat";
  typedef float Row;
};


//static inline Year year_group(Year year) {
//  if (year >= 1980) return floor(float(year)/10)*10;
//  if (year >= 1900) return floor(float(year)/20)*20;
//  else              return floor(float(year)/50)*50;
//}

// struct YearGroup {
//   Year start;
//   Year stop;
// };

// struct Gather {
//   static constexpr const char * fn = "gather.dat";
//   struct Row {
//     LowerId lid;
//     struct SubTable {
      
//       struct Row {
//         float freq;
//         float non_word;
//       };
//     };
//     Data year_group[YEAR_GROUPS.size()]
//   }
// };

#endif
