#ifndef schema__hpp
#define schema__hpp

#include <assert.h>
#include <string.h>

#include <array>

using namespace std;

typedef uint32_t WordId;
typedef uint32_t LowerId;
typedef uint16_t Year;

enum LookupType {ByWord, ByLower};

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

struct YearIndex {
  static const Year MIN_YEAR = 1700;
  static size_t idx(Year y) {assert(y >= MIN_YEAR); return y-MIN_YEAR;}
  static Year   key(size_t i) {return i + MIN_YEAR;}
  typedef Year Idx;
};

struct WordLookup : SimpleIndex<WordId> {
  static constexpr auto fn = "WordLookup.dat";
  typedef Word Row;
};

struct ToLower : SimpleIndex<WordId> {
  static constexpr auto fn = "ToLower.dat";
  typedef LowerId Row;
};

struct LowerLookup : SimpleIndex<LowerId> {
  static constexpr auto fn = "LowerLookup.dat";
  typedef Word Row;
};

struct Freqs : SimpleIndex<> {
  static constexpr auto fn = "Freqs.dat";
  struct Row {
    WordId wid;
    Year   year;
    float  freq;
  };
};

struct FreqsPos : SimpleIndex<> {
  static constexpr auto fn = "FreqsPos.dat";
  struct Row {
    WordId wid;
    Pos    pos;
    Year   year;
    float  freq;
  };
};

struct PosTotals : SimpleIndex<> {
  static constexpr auto fn = "PosTotals.dat";
  struct Row {
    Year   year;
    Pos    pos;
    double freq;
  };
};

struct Totals : SimpleIndex<> {
  static constexpr auto fn = "Totals.dat";
  struct Row {
    Year   year;
    double freq;
  };
};

struct FreqsExclude : SimpleIndex<> {
  static constexpr auto fn = "FreqsExclude.dat";
  using Row = Freqs::Row;
};

struct FreqsFiltered : SimpleIndex<> {
  static constexpr auto fn = "FreqsFiltered.dat";
  using Row = Freqs::Row;
};

struct FreqsLower : SimpleIndex<> {
  static constexpr auto fn = "FreqsLower.dat";
  struct Row {
    LowerId lid;
    Year    year;
    float   freq;
  };
};

struct Counted : YearIndex {
  static constexpr auto fn = "Counted.dat";
  typedef double Row;
};

static constexpr auto Filtered = ByWord, Lower = ByLower;
enum YearFilter {All, Recent};

template <LookupType, YearFilter>
struct Freq;

template <YearFilter Filter>
struct Freq<Filtered,Filter> : SimpleIndex<WordId> {
  static constexpr auto fn = Filter == All ? "FreqAllFiltered.dat" : "FreqRecentFiltered.dat";
  typedef float Row;
};

template <YearFilter Filter>
struct Freq<Lower,Filter> : SimpleIndex<LowerId> {
  static constexpr auto fn = Filter == All ? "FreqAllLower.dat" : "FreqRecentLower.dat";
  typedef float Row;
};

enum SpellerDict : uint8_t {SP_NORMAL = 1, SP_LARGE = 2, SP_NONE = 9};

struct SpellerLookup : SimpleIndex<WordId> {
  static constexpr auto fn = "SpellerLookup.dat";
  typedef Word Row;
};

template <LookupType>
struct Speller;

template <>
struct Speller<ByWord> : SimpleIndex<WordId> {
  static constexpr auto fn = "Speller.dat";
  typedef SpellerDict Row;
};

template <>
struct Speller<ByLower> : SimpleIndex<LowerId> {
  static constexpr auto fn = "SpellerLower.dat";
  typedef SpellerDict Row;
};

template <LookupType>
struct Lookup;

template <>
struct Lookup<ByWord> : SpellerLookup {};

template <>
struct Lookup<ByLower> : LowerLookup {};


  // template <FreqFilter Filter>
  // struct Rank : SimpleIndex<LowerId> {
  //   static constexpr auto fn = Filter == All ? "RankAll.dat" : "RankRecent.dat";
  //   typedef Rank Row;
  // };


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
