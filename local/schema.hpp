#ifndef schema__hpp
#define schema__hpp

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <math.h>

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

#define FILE_NAME(f) static const char * fn() {return f;}

struct WordLookup : SimpleIndex<WordId> {
  FILE_NAME("WordLookup.dat");
  typedef Word Row;
};

struct ToLower : SimpleIndex<WordId> {
  FILE_NAME("ToLower.dat");
  typedef LowerId Row;
};

struct FromLower : SimpleIndex<> {
  FILE_NAME("FromLower.dat");
  struct Row {
    LowerId lid;
    WordId  wid;
  };
};

struct LowerLookup : SimpleIndex<LowerId> {
  FILE_NAME("LowerLookup.dat");
  typedef Word Row;
};

struct Freqs : SimpleIndex<> {
  FILE_NAME("Freqs.dat");
  struct Row {
    WordId wid;
    Year   year;
    float  freq;
  };
};

struct FreqsPos : SimpleIndex<> {
  FILE_NAME("FreqsPos.dat");
  struct Row {
    WordId wid;
    Pos    pos;
    Year   year;
    float  freq;
  };
};

struct PosTotals : SimpleIndex<> {
  FILE_NAME("PosTotals.dat");
  struct Row {
    Year   year;
    Pos    pos;
    double freq;
  };
};

struct Totals : SimpleIndex<> {
  FILE_NAME("Totals.dat");
  struct Row {
    Year   year;
    double freq;
  };
};

struct FreqsExclude : SimpleIndex<> {
  FILE_NAME("FreqsExclude.dat");
  using Row = Freqs::Row;
};

struct FreqsFiltered : SimpleIndex<> {
  FILE_NAME("FreqsFiltered.dat");
  using Row = Freqs::Row;
};

struct FreqsLower : SimpleIndex<> {
  FILE_NAME("FreqsLower.dat");
  struct Row {
    LowerId lid;
    Year    year;
    float   freq;
  };
};

struct Counted : YearIndex {
  FILE_NAME("Counted.dat");
  typedef double Row;
};

static constexpr auto Filtered = ByWord, Lower = ByLower;
enum YearFilter {All, Recent, Current, Adjusted};

template <LookupType, YearFilter>
struct Freq;

template <YearFilter Filter>
struct Freq<Filtered,Filter> : SimpleIndex<WordId> {
  FILE_NAME(Filter == All ? "FreqAllFiltered.dat" : 
            Filter == Recent ? "FreqRecentFiltered.dat" : 
            Filter == Current ? "FreqCurrentFiltered.dat" :
            NULL);
  typedef float Row;
};

template <YearFilter Filter>
struct Freq<Lower,Filter> : SimpleIndex<LowerId> {
  FILE_NAME(Filter == All ? "FreqAllLower.dat" :  
            Filter == Recent ? "FreqRecentLower.dat" :
            Filter == Current ? "FreqCurrentLower.dat" :
            Filter == Adjusted ? "FreqAdjustedLower.dat" :
            NULL);
  typedef float Row;
};

enum SpellerDict : uint8_t {SP_NORMAL = 1, SP_LARGE = 2, SP_NONE = 9};

struct SpellerLookup : SimpleIndex<WordId> {
  FILE_NAME("SpellerLookup.dat");
  typedef Word Row;
};

template <LookupType>
struct Speller;

template <>
struct Speller<ByWord> : SimpleIndex<WordId> {
  FILE_NAME("Speller.dat");
  typedef SpellerDict Row;
};

template <>
struct Speller<ByLower> : SimpleIndex<LowerId> {
  FILE_NAME("SpellerLower.dat");
  typedef SpellerDict Row;
};

template <LookupType>
struct Lookup;

template <>
struct Lookup<ByWord> : SpellerLookup {};

template <>
struct Lookup<ByLower> : LowerLookup {};

template <LookupType, YearFilter>
struct Stats;

struct StatsRow {
  uint32_t rank; 
  uint32_t normal_incl; 
  uint32_t large_incl;
};

template<>
struct Stats<Filtered,Current> : SimpleIndex<WordId> {
  FILE_NAME("StatsCurrentFiltered.dat");
  typedef StatsRow Row;
};
template<>
struct Stats<Filtered,Recent> : SimpleIndex<WordId> {
  FILE_NAME("StatsRecentFiltered.dat");
  typedef StatsRow Row;
};
template<>
struct Stats<Filtered,All> : SimpleIndex<WordId> {
  FILE_NAME("StatsAllFiltered.dat");
  typedef StatsRow Row;
};
template<>
struct Stats<Lower,Current> : SimpleIndex<LowerId> {
  FILE_NAME("StatsCurrentLower.dat");
  typedef StatsRow Row;
};
template<>
struct Stats<Lower,Recent> : SimpleIndex<LowerId> {
  FILE_NAME("StatsRecentLower.dat");
  typedef StatsRow Row;
};
template<>
struct Stats<Lower,All> : SimpleIndex<LowerId> {
  FILE_NAME("StatsAllLower.dat");
  typedef StatsRow Row;
};
template<>
struct Stats<Lower,Adjusted> : SimpleIndex<LowerId> {
  FILE_NAME("StatsAdjustedLower.dat");
  typedef StatsRow Row;
};

struct RankFreq {
  unsigned rank = -1;
  float    freq = NAN;
  RankFreq() {}
  RankFreq(unsigned r) : rank(r) {}
};

struct DictionaryStats {
  RankFreq in_corpus;
  RankFreq non_filtered;
  RankFreq total;
  DictionaryStats() {}
  DictionaryStats(unsigned nf, unsigned t) 
    : non_filtered{nf}, total{t} {}
};

struct SpellerStats {
  DictionaryStats normal,large;
  template<LookupType L, YearFilter F>
  static std::string fn() {
    string res;
    res += "Speller";
    res += Stats<L,F>::fn();
    return res;
  }
  template<LookupType L, YearFilter F>
  void save() {
    auto fd = open(fn<L,F>().c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0666);
    write(fd, this, sizeof(SpellerStats));
    close(fd);
  }
  template<LookupType L, YearFilter F>
  void load() {
    auto fd = open(fn<L,F>().c_str(), O_RDONLY);
    assert(fd != -1);
    auto res = read(fd, this, sizeof(SpellerStats));
    assert(res == sizeof(SpellerStats));
    close(fd);
  }
};

  // template <FreqFilter Filter>
  // struct Rank : SimpleIndex<LowerId> {
  //   FILE_NAME(Filter == All ? "RankAll.dat" : "RankRecent.dat";
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
