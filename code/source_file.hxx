// Copyright (C) 2019  Claire Hansel
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef BUCKET_SOURCE_FILE_HXX
#define BUCKET_SOURCE_FILE_HXX

#include <boost/noncopyable.hpp>
#include <forward_list>
#include <memory>
#include <ostream>
#include <utf8cpp/utf8.h>
#include <utility>

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
  void highlight(std::ostream& stream, iterator begin, iterator end);
  void highlight(std::ostream& stream, iterator_range_list const& ranges);
  // Creates a formatted excerpt of the code contaning in which a single
  // character, a range of characters, or a list of ranges of characters are
  // highlighted and underlined and then writes it to 'stream'.

  std::string highlight(iterator position);
  std::string highlight(iterator begin, iterator end);
  std::string highlight(iterator_range_list const& ranges);
  // The same as the first three highlight functions except the result is
  // returned as a string rather than written to a stream.

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
  // Get line and column number from a file position. Note that these count from
  // 1 not 0.

};

#endif
