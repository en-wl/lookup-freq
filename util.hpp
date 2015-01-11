#ifndef util__hpp
#define util__hpp

#include <assert.h>
#include <unordered_map>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "schema.hpp"

struct Str {
  char * data;
  size_t size = 0;
  size_t capacity = 8;
  Str() {data = (char *)malloc(capacity); data[0] = '\0';}
  Str(const Str &) = delete;
  char & operator[](int i) {return data[i];}
  const char * begin() const {return data;}
  char & front() {return data[0];}
  char & back()  {return data[size-1];}
  const char * end() const {return data + size;}
  void ensure_space(size_t needed) {
    if (capacity < needed) {data = (char *)realloc(data, needed); capacity = needed;}
  }
  void assign(const char * b, const char * e) {
    size = e - b;
    if (capacity < size + 1) {data = (char *)realloc(data, size + 1); capacity = size + 1;}
    memcpy(data, b, size);
    data[size] = '\0';
  }
  void operator=(const Str & other) {
    size = other.size;
    if (capacity < size + 1) {data = (char *)realloc(data, size + 1); capacity = size + 1;}
    memcpy(data, other.data, size + 1);
  }
  bool eq(const char * d, unsigned sz) {
    return size == sz && memcmp(data, d, sz) == 0;
  }
  bool eq(const char * b, const char * e) {return eq(b,e-b);}
};

struct WordBuffer {
  char * begin;
  char * end;
  operator const char * () const {return begin;}
  size_t size() const {return end - begin;}
  size_t capacity = 128*1024*1014;
  WordBuffer() {begin = end = (char *)malloc(capacity);}
  void load(const char * fn) {
    auto fd = open(fn, O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    end = begin + st.st_size;
    assert(size() <= capacity);
    for (auto p = begin; p < end;)
      p += read(fd, p, end-p);
    close(fd);
  }
  void save(const char * fn) {
    auto fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC,00666);
    for (auto p = begin; p < end;)
      p += write(fd, p, end - p);
    close(fd);
  }
  char * append(Str & word) {
    assert(size() + word.size + 1 <= capacity);
    auto loc = end;
    memcpy(end, word.data, word.size + 1);
    end += word.size + 1;
    return loc;
  }
};

struct StrHash {
  unsigned long operator()(const char * __s) const {
    unsigned long __h = 0; 
    for ( ; *__s; ++__s)
      __h = 5*__h + *__s;
    return size_t(__h);
  }
};

struct StrEq {
  bool operator() (const char * a, const char * b) const {return strcmp(a,b)==0;}
};

struct WordLookup : std::unordered_map<const char *, WordId, StrHash, StrEq> {
  WordBuffer & word_buffer;
  WordLookup(WordBuffer & b, const Word * i = NULL, const Word * e = NULL) : word_buffer(b) {
    reserve(e-i);
    for (; i != e; ++i)
      insert({i->str(word_buffer), size()});
  }
  void save(const char * fn) {
    auto data_len = sizeof(WordId) * size();
    auto data = (WordId *)malloc(data_len);
    for (auto p : *this) {
      data[p.second] = p.first - word_buffer.begin;
    }
    auto fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC,00666);
    for (size_t offset = 0; offset < data_len;) 
      offset += write(fd, data + offset, data_len - offset);
    close(fd);
    free(data);
  }
  unsigned add(Str & word) {
    auto found = find(word.data);
    if (found == end()) {
      found = insert({word_buffer.append(word), size()}).first;
    }
    return found->second;
  }
  unsigned add(Str & word, WordLookup & buffer_lookup) {
    auto found = find(word.data);
    if (found == end()) {
      auto p = buffer_lookup.find(word.data);
      const char * str = p == buffer_lookup.end() ? word_buffer.append(word) : p->first;
      found = insert({str, size()}).first;
    }
    return found->second;
  }
};

#endif
