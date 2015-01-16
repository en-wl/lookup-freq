#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

template <typename T>
struct MMapVector {
  T * data;
  size_t size_;

  typedef const T value_type;
  typedef const T * iterator;
  typedef const T * const_iterator;
  size_t size() const {return size_;}
  const T & at(size_t i) const {assert(i <= size_); return data[i];}
  const T & operator[] (int i) const {return data[i];}
  const T * begin() const {return data;}
  const T * end() const {return data + size_;}

  int fd;
  size_t len;
  MMapVector(const char * fn, int advice = MADV_SEQUENTIAL) {
    fd = open(fn, O_RDONLY | O_NOATIME);
    struct stat st;
    fstat(fd, &st);
    len = st.st_size;
    size_ = len / sizeof(T);
    data = (T *)mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
    madvise(data, len, advice);
    assert(data);
  }
  ~MMapVector() {
    munmap((void *)data, len);
    close(fd);
  }
};

template <typename T>
struct MutMMapVector {
  T * data;
  size_t size_;

  typedef T value_type;
  typedef T * iterator;
  typedef const T * const_iterator;
  size_t size() const {return size_;}
  T & at(size_t i) {assert(i <= size_); return data[i];}
  T & operator[] (int i) {return data[i];}
  T * begin() {return data;}
  T * end() {return data + size_;}

  int fd;
  size_t len;
  MutMMapVector(const char * fn) {
    fd = open(fn, O_RDWR | O_NOATIME);
    struct stat st;
    fstat(fd, &st);
    len = st.st_size;
    size_ = len / sizeof(T);
    data = (T *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(data);
  }
  MutMMapVector(const char * fn, unsigned sz) {
    fd = open(fn, O_CREAT | O_TRUNC | O_RDWR| O_NOATIME, 00666);
    len = sz * sizeof(T);
    size_ = sz;
    ftruncate(fd, len);
    data = (T *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(data);
  }
  void resize(size_t sz) {
    munmap((void *)data, len);
    len = sz * sizeof(T);
    size_ = sz;
    auto res = ftruncate(fd, len);
    if (res == -1) perror("");
    data = (T *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(data);
  }
  ~MutMMapVector() {
    munmap((void *)data, len);
    close(fd);
  }
};
