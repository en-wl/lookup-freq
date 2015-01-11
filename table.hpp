#ifndef table__hpp
#define table__hpp

#include <vector>

#include <type_traits>
template <typename T>
using no_reference = typename std::remove_reference<T>::type;

#include "mmap_vector.hpp"

template <typename T> 
struct TableCreator {
  FILE * fd;
  TableCreator() {
    fd = fopen(T::fn, "w");
  };
  TableCreator(const TableCreator &) = delete;
  void operator=(const TableCreator &) = delete;
  void append_row(const typename T::Row & row) {
    fwrite(&row, sizeof(typename T::Row), 1, fd);
  }
  ~TableCreator() {
    fclose(fd);
  }
};

template <typename T, class Key, class Merge>
struct MergeTableCreator {
  MergeTableCreator(Key k, Merge m) : key(k), merge(m) {}
  TableCreator<T> table;
  Key key;
  Merge merge;
  bool first = false;
  bool sorted = true;
  typename T::Row prev_row;
  void append_row(const typename T::Row & row) {
    if (first) {
      prev_row = row;
    } else if (key(prev_row) != key(row)) {
      table.append_row(prev_row);
      if (key(prev_row) > key(row)) sorted = false;
      prev_row = row;
    } else {
      merge(prev_row, row);
    }
  }
};
template <typename T, class Key, class Merge>
MergeTableCreator<T,Key,Merge>
merge_table_creator(Key key, Merge merge) {return {key, merge};}

template <typename T, typename Vec>
struct TableLike : Vec {
  typedef typename Vec::value_type Row;
  typedef typename T::Idx Idx;
  Row & row (Idx i) {return this->at(T::idx(i));}
  const Row & row (Idx i) const {return this->at(T::idx(i));}
  Idx min() const {return T::key(0);}
  Idx max() const {return T::key(this->size()-1);}
  using Vec::Vec;
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
  A a, a_end;
  B b, b_end;
  bool at_end() const {return a == a_end && b == b_end;}

  JoinRow<decltype(key(*a)), no_reference<decltype(*a)>, no_reference<decltype(*b)> >
  next() {
    if (a == a_end)         return {key(*b), NULL, &*b++};
    if (b == b_end)         return {key(*a), &*a++, NULL};
    if (key(*a) == key(*b)) return {key(*a), &*a++, &*b++};
    if (key(*a) < key(*b))  return {key(*a), &*a++, NULL};
    else                    return {key(*b), NULL, &*b++};
  }
};
template <class A, class B, class Key>
Joiner<A,B,Key> mk_joiner(Key key, A a, A a_end, B b, B b_end) {
  return {key, a, a_end, b, b_end};
}

template <class A, class B, class Key>
auto join(const A & a, const B & b, Key key) {
  return mk_joiner(key, a.begin(), a.end(), b.begin(), b.end());
}

#endif
