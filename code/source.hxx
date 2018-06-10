#ifndef BUCKET_SOURCE_HXX
#define BUCKET_SOURCE_HXX

#include <boost/noncopyable.hpp>
#include <forward_list>
#include <memory>
#include <ostream>
#include <utf8.h>
#include <utility>

class Source : private boost::noncopyable {
public:
  #ifdef BUCKET_DEBUG
  using iterator = utf8::iterator<char*>;
  #else
  using iterator = utf8::unchecked::iterator<char*>;
  #endif
  explicit Source(const char* path);
  iterator begin();
  iterator end();
  void highlight(std::ostream& stream, iterator position);
  void highlight(std::ostream& stream, iterator range_begin, iterator range_end);
  void highlight(std::ostream& stream, std::forward_list<std::pair<iterator, iterator>> const& ranges);
private:
  const char* const m_path;
  std::unique_ptr<char[]> m_buffer;
  char* m_begin;
  char* m_end;
  std::pair<unsigned, unsigned> getLineAndColumn(iterator position);
};

#endif
