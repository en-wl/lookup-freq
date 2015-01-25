#include <vector>
#include <algorithm>

#include <cmph.h>

#include "schema.hpp"
#include "table.hpp"
#include "util.hpp"
#include "final.hpp"

template <LookupType L>
struct Info;

template <>
struct Info<Lower> {
  WordBuffer & buffer;
  Table<Lookup<Lower> >         lookup;
  Table<Freq<Lower,Adjusted> >  adjusted;
  Table<Speller<Lower> >        speller;
  Table<Freq<Lower,Recent> >    recent;
  Table<Freq<Lower,Current> >   current;
  Table<Stats<Lower,Adjusted> > stats;
};
template <>
struct Info<Filtered> {
  WordBuffer & buffer;
  Table<Lookup<Filtered> >      lookup;
  Table<Freq<Filtered,Recent> > freq;
};

size_t calaculate_length(Info<Lower> & info);
size_t calaculate_length(Info<Filtered> & info);
WordInfo * populate_entry(unsigned idx, Info<Lower> & info, void * loc);
OrigWordInfo * populate_entry(unsigned idx, float base_freq, Info<Filtered> & info, void * loc);

struct ByFreq {
  LowerId  lid;
  float    freq;
};

int main() {
  WordBuffer     buffer;
  buffer.load("words_w_lower.dat");
  Info<Lower>    info_lower{buffer};
  Info<Filtered> info_filtered{buffer};
  Table<FromLower> from_lower;
  vector<const char *> keys;
  vector<ByFreq>       by_freq;

  printf("gathering keys...\n");

  for (auto v : info_lower.adjusted.view) {
    if (v.second <= 0.0) continue;
    //printf("%g\n", v.second);
    keys.push_back(info_lower.lookup[v.first].str(buffer));
    by_freq.push_back({v.first,v.second});
  }

  printf("number of keys = %lu (vs %lu)\n", keys.size(), info_lower.adjusted.size());

  printf("computing perfect hash\n");

  auto source = cmph_io_vector_adapter((char **)&*keys.begin(), keys.size());
  auto mphf_fd = fopen("ngrams.mph", "w");
  auto config = cmph_config_new(source);
  cmph_config_set_algo(config, CMPH_BDZ_PH);
  cmph_config_set_mphf_fd(config, mphf_fd);
  auto hash = cmph_new(config);
  cmph_config_destroy(config);
  cmph_dump(hash, mphf_fd); 
  cmph_destroy(hash);	
  fclose(mphf_fd);

  mphf_fd = fopen("ngrams.mph", "r");
  assert(mphf_fd);
  hash = cmph_load(mphf_fd);
  printf("hash table size = %u\n", cmph_size(hash));

  printf("indexing from_lower\n");

  assert(from_lower.size() == info_filtered.freq.size());
  vector<unsigned> from_lower_idx(info_lower.lookup.size(), 0);
  {
    unsigned prev = -1;
    for (unsigned i = 0; i != from_lower.size(); ++i) {
      auto lid = from_lower[i].lid;
      if (lid != prev) from_lower_idx[lid] = i;
      prev = lid;
    }
  }

  printf("sorting based on freq\n");

  sort(by_freq.begin(), by_freq.end(), [](auto a, auto b){return a.freq > b.freq;});

  //vector<ToHash> to_hash;
  //to_hash.reserve(keys.size());
  //for (auto v : info_lower.freq.view) {
  //  if (v.second <= 0.0) continue;
  //  auto key = info_lower.lookup[v.first].str(buffer);
  //  unsigned id = cmph_search(hash, key, (cmph_uint32)strlen(key));
  //  to_hash.push_back({v.first,id});
  //}

  printf("computing request len...\n");

  size_t result_len = calaculate_length(info_lower) + calaculate_length(info_filtered);
  printf("result_len = %lu\n", result_len);

  printf("creating data file\n");

  MutMMapVector<uint32_t> hash_table("ngrams.tbl", cmph_size(hash));
  for (auto & v : hash_table) v = -1;

  auto fd = open("ngrams.dat", O_CREAT | O_TRUNC | O_RDWR| O_NOATIME, 00666);
  ftruncate(fd, result_len);
  char * data = (char *)mmap(NULL, result_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  char * data_end = data + result_len;
  assert(data);

  unsigned lower_size = 0, fil_size = 0;
  vector<WordId> others;
  auto p = data;
  for (auto v : by_freq) {
    auto key = info_lower.lookup[v.lid].str(buffer);
    unsigned hash_id = cmph_search(hash, key, (cmph_uint32)strlen(key));
    hash_table[hash_id] = p - data;
    assert (p < data_end);
    auto wi = populate_entry(v.lid, info_lower, p);
    p += wi->skip;
    lower_size += wi->skip;
    {
      others.clear();
      for (auto q = from_lower.begin() + from_lower_idx[v.lid];
           q != from_lower.end() && q->lid == v.lid;
           ++q)
        if (info_filtered.freq[q->wid] > 0.0)
          others.push_back(q->wid);
      sort(others.begin(), others.end(), [&](auto a, auto b){return info_filtered.freq[a] > info_filtered.freq[b];});
    }
    wi->more = 1;
    OrigWordInfo * prev = NULL;
    for (auto wid : others) {
      if (prev) prev->more = 1;
      assert(p < data_end);
      auto owi = populate_entry(wid, info_lower.recent[v.lid], info_filtered, p);
      p += owi->skip;
      fil_size += owi->skip;
      prev = owi;
    }
  }
  if (p != data_end) {
    fprintf(stderr, "NOT AT END: %p != %p, diff %ld\n", p, data_end, data_end-p);
    abort();
  }

  printf("Done.\n");
}

static inline size_t round_up(size_t wlen) {
  auto remainder = wlen % sizeof(uint32_t);
  return remainder == 0 ? wlen : wlen + sizeof(uint32_t) - remainder;
}

size_t calaculate_length(Info<Lower> & info) {
  size_t sz = 0, len = 0;
  for (auto v : info.adjusted.view) {
    if (v.second > 0.0) {
      sz++;
      auto word = info.lookup[v.first].str(info.buffer);
      len += sizeof(WordInfo) + round_up(strlen(word) + 1);
    }
  }
  printf("entries = %lu, len = %lu\n", sz, len);
  return len;
}

size_t calaculate_length(Info<Filtered> & info) {
  size_t sz = 0, len = 0;
  for (auto v : info.freq.view) {
    if (v.second > 0.0) {
      sz++;
      auto word = info.lookup[v.first].str(info.buffer);
      len += round_up(sizeof(OrigWordInfo) + strlen(word) + 1);
    }
  }
  printf("entries = %lu, len = %lu\n", sz, len);
  return len;
}

#define set_check(field,val) {wi->field = (val); if (wi->field != (val)) {fprintf(stderr,"word: %s(%lu) wi->%s != val %lu %lu", word, strlen(word), #field, (unsigned long)wi->field, (unsigned long)(val)); abort();}}

WordInfo * populate_entry(unsigned idx, Info<Lower> & info, void * loc) {
  auto word = info.lookup[idx].str(info.buffer);
  //printf("populate: %s\n", word);
  auto wi = static_cast<WordInfo *>(loc);
  wi->freq = info.adjusted[idx];
  wi->newness = info.current[idx]/info.recent[idx];
  switch (info.speller[idx]) {
    case SP_NORMAL: wi->dict = 0; break;
    case SP_LARGE: wi->dict = 1; break;
    case SP_NONE: wi->dict = 3; break;
  }
  set_check(rank, info.stats[idx].rank);
  set_check(normal_incl, info.stats[idx].normal_incl);
  set_check(large_incl, info.stats[idx].large_incl);
  set_check(word_len, strlen(word));
  set_check(skip, round_up(sizeof(WordInfo) + strlen(word)+1));
  wi->more = 0;
  strcpy(wi->word, word);
  return wi;
}

OrigWordInfo * populate_entry(unsigned idx, float base_freq, Info<Filtered> & info, void * loc) {
  auto word = info.lookup[idx].str(info.buffer);
  //printf("populate: %s\n", word);
  auto wi = static_cast<OrigWordInfo *>(loc);
  wi->percent = 100*info.freq[idx]/base_freq;
  set_check(skip, round_up(sizeof(OrigWordInfo) + strlen(word)+1));
  wi->more = 0;
  strcpy(wi->word, word);
  return wi;
}

#undef set_check
