#ifndef table__hpp
#define table__hpp

#include <vector>
#include <iterator>

#include <type_traits>
template <typename T>
using no_reference = typename std::remove_reference<T>::type;

#include "mmap_vector.hpp"

template <typename T> 
struct TableCreator {
  FILE * fd = NULL;
  TableCreator() {
    fd = fopen(T::fn, "w");
  };
  TableCreator(const TableCreator &) = delete;
  TableCreator(TableCreator && other) : fd(other.fd) {other.fd=NULL;}
  void operator=(const TableCreator &) = delete;
  void append_row(const typename T::Row & row) {
    fwrite(&row, sizeof(typename T::Row), 1, fd);
  }
  ~TableCreator() {
    if (fd)
      fclose(fd);
  }
};

template <typename T, class Key, class Merge>
struct MergeTableCreator {
  MergeTableCreator(Key k, Merge m) 
    : key(k), merge(m) {}
  MergeTableCreator(MergeTableCreator && other) = default;
  TableCreator<T> table;
  Key key;
  Merge merge;
  bool first = true;
  bool sorted = true;
  typename T::Row prev_row;
  void append_row(const typename T::Row & row) {
    if (first) {
      prev_row = row;
      first = false;
    } else if (key(prev_row) != key(row)) {
      table.append_row(prev_row);
      if (key(prev_row) > key(row)) {
        sorted = false;
      }
      prev_row = row;
    } else {
      merge(prev_row, row);
    }
  }
  ~MergeTableCreator() {
    if (table.fd && !first)
      table.append_row(prev_row);
  }
};
template <typename T, class Key, class Merge>
MergeTableCreator<T,Key,Merge>
merge_table_creator(Key key, Merge merge) {return {key, merge};}

template <typename T, typename I, typename V> 
struct MapViewItr : public iterator<forward_iterator_tag,V> 
{
  I start, i;
  MapViewItr(I i) : start(i), i(i) {}
  V operator*() const {return {T::key(0) + (i - start),*i};}
  auto operator++() {++i; return *this;}
  auto operator++(int) {auto tmp = *this; ++i; return *this;}
};
template <typename T, typename I, typename V>
inline bool operator==(MapViewItr<T,I,V> x, MapViewItr<T,I,V> y) {return x.i == y.i;}
template <typename T, typename I, typename V>
inline bool operator!=(MapViewItr<T,I,V> x, MapViewItr<T,I,V> y) {return x.i != y.i;}

template <typename T, typename Vec>
struct MapView {
  Vec & data;
  MapView(Vec & d) : data(d) {}
  typedef typename T::Idx            key_type;
  typedef typename Vec::value_type   mapped_type;
  typedef pair<key_type,mapped_type> value_type;
  typedef MapViewItr<T, typename Vec::iterator,       value_type> iterator;
  typedef MapViewItr<T, typename Vec::const_iterator, value_type> const_iterator;
  iterator begin() {return {data.begin()};}
  iterator end()   {return {data.end()};}
  const_iterator begin() const {return {data.begin()};}
  const_iterator end()   const {return {data.end()};}
  auto & operator[] (key_type i) {return data.at(T::idx(i));}
  const auto & operator[] (key_type i) const {return data.at(T::idx(i));}
  static auto min() {return T::key(0);}
  auto max() const {return T::key(this->size()-1);}
};

template <typename T, typename Vec>
struct TableLike : Vec {
  typedef MapView<T,Vec> View;
  MapView<T,Vec> view = *this;
  template<typename... Args>
  TableLike(Args&&... args) : Vec(forward<Args>(args)...) {}
};

template <typename T>
using TmpTable = TableLike<T, vector<typename T::Row>>;

template <typename T>
struct Table : TableLike<T, MMapVector<typename T::Row> > {
  Table() : TableLike<T, MMapVector<typename T::Row> >(T::fn) {}
};

template <typename T>
struct MutTable : TableLike<T, MutMMapVector<typename T::Row> > {
  MutTable() : TableLike<T, MutMMapVector<typename T::Row> >(T::fn) {}
  MutTable(size_t sz) : TableLike<T, MutMMapVector<typename T::Row> >(T::fn, sz) {}
};

template <typename T, class Key, class Merge>
void merge(MutTable<T> & tbl, Key key, Merge merge) {
  sort(tbl.begin(), tbl.end(), [key](auto a, auto b){return key(a) < key(b);});
  auto p = tbl.begin();
  for (auto i = tbl.begin() + 1; i < tbl.end(); ++i) {
    if (key(*p) == key(*i)) {
      merge(*p,*i);
    } else {
      ++p;
      *p = *i;
    }
  }
  ++p;
  printf("Merge: Old Size: %lu => New Size: %lu\n", tbl.size(), p - tbl.begin());
  tbl.resize(p - tbl.begin());
}

template <class Key, class A, class B>
struct JoinRow {
  const Key k;
  const A * a;
  const B * b;
};
template <class A, class B, class Key>
struct Joiner
{
  Key key;
  template <typename T> struct C {
    T cur;
    T end;
    bool at_end() const {return cur == end;}
  };
  C<A> a;
  C<B> b;
  template <typename T>
  auto adv(T & i) {
    auto prev = i.cur;
    ++i.cur;
    if (!i.at_end())
      assert(key(*prev) <= key(*i.cur));
    return &*i.cur;
  }

  bool at_end() const {return a.at_end(); b.at_end();}

  JoinRow<decltype(key(*a.cur)), no_reference<decltype(*a.cur)>, no_reference<decltype(*b.cur)> >
  next() {
    if (a.at_end())                 return {key(*b.cur), NULL,   adv(b)};
    if (b.at_end())                 return {key(*a.cur), adv(a), NULL};
    if (key(*a.cur) == key(*b.cur)) return {key(*a.cur), adv(a), adv(b)};
    if (key(*a.cur) <  key(*b.cur)) return {key(*a.cur), adv(a), NULL};
    else                            return {key(*b.cur), NULL,   adv(b)};
  }
};
template <class A, class B, class Key>
Joiner<A,B,Key> mk_joiner(Key key, A a, A a_end, B b, B b_end) {
  return {key, {a, a_end}, {b, b_end}};
}

template <class A, class B, class Key>
auto join(const A & a, const B & b, Key key) {
  return mk_joiner(key, a.begin(), a.end(), b.begin(), b.end());
}

#endif
