#ifndef BUCKET_SOURCE_FILE_HXX
#define BUCKET_SOURCE_FILE_HXX

#include <boost/noncopyable.hpp>
#include <forward_list>
#include <memory>
#include <ostream>
#include <utf8.h>
#include <utility>

#define BUCKET_BOLD  "\033[1;30m"
#define BUCKET_BLACK "\033[0;30m"
#define BUCKET_RED   "\033[0;31m"
// Colors for highlighting

class SourceFile : private boost::noncopyable {
// Represents a file containing Bucket source code. SourceFile objects store the
// source code in a buffer and do not keep a file descriptor open after the
// object is constructed.

public:

  #ifdef BUCKET_DEBUG
    using iterator = utf8::iterator<char*>;
  #else
    using iterator = utf8::unchecked::iterator<char*>;
  #endif
  using iterator_range = std::pair<iterator, iterator>;
  using iterator_range_list = std::forward_list<iterator_range>;
  // Since the file contents are checked to be valid UTF-8 when the SourceFile
  // object is constructed, it is safe to used the unchecked iterator. If
  // debugging is on the standard iterator which throws on invalid UTF-8 will be
  // used.

  explicit SourceFile(const char* path);
  // The file given by 'path' is opened, read into a buffer, and closed. If
  // there is an error reading the file or if the file does not contain valid
  // UTF-8 code, an exception is thrown.

  iterator begin();
  iterator end();
  // Returns a UTF-8 aware iterator to either the beginning or the end of the
  // file contents.

  void highlight(std::ostream& stream, iterator position);
  void highlight(std::ostream& stream, iterator_range range);
  void highlight(std::ostream& stream, iterator_range_list const& ranges);
  // Creates a formatted excerpt of the code contaning in which a single
  // character, a range of characters, or a list of ranges of characters are
  // highlighted and underlined and then writes it to 'stream'.

private:

  const char* const m_path;
  // The path of the file. This is stored so if there is an error later such as
  // during parsing the error message will say what file the error was in.

  std::unique_ptr<char[]> m_buffer;
  // Buffer containing the contents of the file.

  char* m_begin;
  char* m_end;
  // Pointers to the beginning and of the file. The pointer to the beginning of
  // the file is the same as m_buffer.get() unless the file starts with a byte
  // order mark in which case it is m_buffer.get() + 3 (since byte order marks
  // are three bytes long).

  std::pair<unsigned, unsigned> getLineAndColumn(iterator position);

};

#endif
