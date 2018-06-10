#include "source.hxx"
#include "misc.hxx"
#include <boost/numeric/conversion/cast.hpp>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <limits>
#include <type_traits>

#ifndef BUCKET_DONT_USE_COLOR
  #define BUCKET_BOLD  "\033[1;30m"
  #define BUCKET_BLACK "\033[0;30m"
  #define BUCKET_RED   "\033[0;31m"
#else
  #define BUCKET_BOLD  ""
  #define BUCKET_BLACK ""
  #define BUCKET_RED   ""
#endif

Source::Source(const char* path) : m_path{path} {
  assert(path);
  try {
    std::ifstream file;
    file.exceptions(file.failbit | file.badbit);
    file.open(path, file.binary);
    file.seekg(0, file.end);
    auto file_size = boost::numeric_cast<std::size_t>(std::streamoff(file.tellg()));
    file.seekg(0, file.beg);
    m_buffer = std::make_unique<char[]>(file_size);
    m_begin  = m_buffer.get();
    m_end    = m_begin + file_size;
    file.read(m_begin, file_size);
  } catch (const std::ifstream::failure&) {
    throw misc::make_runtime_error("unable to open file '", m_path, "'");
  }
  if (utf8::starts_with_bom(m_begin, m_end))
    m_begin += 3;
  if (!utf8::is_valid(m_begin, m_end))
    throw misc::make_runtime_error("file '", m_path, "' contains invalid utf8");
}

Source::iterator Source::begin() {
  #ifdef BUCKET_DEBUG
  return iterator(m_begin, m_begin, m_end);
  #else
  return iterator(m_begin);
  #endif
}

Source::iterator Source::end() {
  #ifdef BUCKET_DEBUG
  return iterator(m_end, m_begin, m_end);
  #else
  return iterator(m_end);
  #endif
}

void Source::highlight(std::ostream& stream, iterator position) {
  // print header
  auto [line, column] = getLineAndColumn(position);
  stream << "file '" BUCKET_BOLD << m_path << BUCKET_BLACK "': line " << line
         << ", column " << column << ":\n|";
  // get iterator to start of line containing 'position'
  auto start_of_line = position;
  while (start_of_line != begin()) {
    --start_of_line;
    if (*start_of_line == '\n') {
      ++start_of_line;
      break;
    }
  }
  auto iter = start_of_line;
  unsigned number_of_underline_spaces = 0;
  // write part of line before position
  while (iter != position) {
    utf8::append(*iter, std::ostream_iterator<char>(stream));
    ++number_of_underline_spaces;
    ++iter;
  }
  // write position
  if (*position != '\n') {
    stream << BUCKET_RED;
    utf8::append(*position, std::ostream_iterator<char>(stream));
  }
  // write after position
  stream << BUCKET_BLACK;
  ++iter;
  while (iter != end() && *iter != '\n') {
    utf8::append(*iter, std::ostream_iterator<char>(stream));
    ++iter;
  }
  stream << "\n|";
  // write underline
  for (unsigned i = 0; i != number_of_underline_spaces; ++i)
    stream << ' ';
  stream << BUCKET_RED "^\n" BUCKET_BLACK;
}

void Source::highlight(std::ostream& stream, iterator range_begin, iterator range_end) {
  // print header
  auto [line, column] = getLineAndColumn(range_begin);
  stream << "file '" BUCKET_BOLD << m_path << BUCKET_BLACK "': starting from li"
         "ne " << line << ", column " << column << ":\n|";
  auto start_of_line = range_begin;
  while (start_of_line != begin()) {
   --start_of_line;
   if (*start_of_line == '\n') {
     ++start_of_line;
     break;
   }
  }
  auto iter = start_of_line;
  unsigned number_of_underline_spaces = 0;
  unsigned number_of_underline_highlights = 0;
  // write the part of first line before the range begins
  while (iter != range_begin) {
    utf8::append(*iter, std::ostream_iterator<char>(stream));
    ++number_of_underline_spaces;
    ++iter;
  }
  stream << BUCKET_RED;
  // write rest of the first line
  while (true) {
    if (iter == range_end) {
      // write part after range
      stream << BUCKET_BLACK;
      while (iter != end() && *iter != '\n') {
        utf8::append(*iter, std::ostream_iterator<char>(stream));
        ++iter;
      }
      stream << "\n|";
      for (unsigned i = 0; i != number_of_underline_spaces; ++i)
        stream << ' ';
      stream << BUCKET_RED;
      for (unsigned i = 0; i != number_of_underline_highlights; ++i)
        stream << '^';
      stream << BUCKET_BLACK "\n";
      return;
    }
    if (*iter == '\n')
      break;
    utf8::append(*iter, std::ostream_iterator<char>(stream));
    ++number_of_underline_highlights;
    ++iter;
  }
  stream << BUCKET_BLACK "\n|";
  for (unsigned i = 0; i != number_of_underline_spaces; ++i)
    stream << ' ';
  stream << BUCKET_RED;
  for (unsigned i = 0; i != number_of_underline_highlights; ++i)
    stream << '^';
  stream << BUCKET_BLACK "\n";
  ++iter;
  while (iter != range_end) {
    stream << "|" BUCKET_RED;
    number_of_underline_highlights = 0;
    while (*iter != '\n') {
      utf8::append(*iter, std::ostream_iterator<char>(stream));
      ++number_of_underline_highlights;
      ++iter;
      if (iter == range_end) {
        stream << BUCKET_BLACK;
        while (iter != end() && *iter != '\n') {
          utf8::append(*iter, std::ostream_iterator<char>(stream));
          ++iter;
        }
        stream << "\n|" BUCKET_RED;
        for (unsigned i = 0; i != number_of_underline_highlights; ++i)
          stream << '^';
        stream << BUCKET_BLACK "\n";
        return;
      }
    }
    stream << BUCKET_BLACK "\n|" BUCKET_RED;
    for (unsigned i = 0; i != number_of_underline_highlights; ++i)
      stream << '^';
    stream << BUCKET_BLACK "\n";
    ++iter;
  }
}

void Source::highlight(std::ostream& stream, std::forward_list<std::pair<iterator, iterator>> const& ranges) {
  iterator start_of_line = begin();
  unsigned highlight_depth = 0;
  bool previous_line_was_highlighted = false;
  bool any_line_was_highlighted = false;
  auto needToHighlightLine = [&, this](){
    auto new_highlight_depth = highlight_depth;
    for (auto iter = start_of_line; iter != end() && *iter != '\n'; ++iter) {
      for (auto& range : ranges) {
        if (iter == range.first)
          ++new_highlight_depth;
        if (iter == range.second) {
          assert(new_highlight_depth);
          --new_highlight_depth;
        }
      }
      if (new_highlight_depth)
        return true;
    }
    highlight_depth = new_highlight_depth;
    return false;
  };
  auto highlightCharacters = [&, this](auto output_normal, auto output_highlight){
    auto new_highlight_depth = highlight_depth;
    auto iter = start_of_line;
    while (true) {
      for (auto range : ranges) {
        if (iter == range.first)
          ++new_highlight_depth;
        if (iter == range.second) {
          assert(new_highlight_depth);
          --new_highlight_depth;
        }
      }
      if (iter == end()) {
        assert(!new_highlight_depth);
        break;
      }
      if (*iter == '\n') {
        ++iter;
        break;
      }
      if (new_highlight_depth) {
        stream << BUCKET_RED;
        output_highlight(iter);
        stream << BUCKET_BLACK;
      }
      else
        output_normal(iter);
      ++iter;
    }
    stream << '\n';
    return std::make_pair(iter, new_highlight_depth);
  };
  do {
    if (needToHighlightLine()) {
      if (any_line_was_highlighted && !previous_line_was_highlighted)
        stream << "...\n";
      stream << '|';
      auto [iter1, depth1] = highlightCharacters(
        [&stream](iterator iter){utf8::append(*iter, std::ostream_iterator<char>(stream));},
        [&stream](iterator iter){utf8::append(*iter, std::ostream_iterator<char>(stream));}
      );
      stream << '|';
      auto [iter2, depth2] = highlightCharacters(
        [&stream](iterator){stream << ' ';},
        [&stream](iterator){stream << '^';}
      );
      any_line_was_highlighted = previous_line_was_highlighted = true;
      assert(iter1 == iter2);
      assert(depth1 == depth2);
      start_of_line = iter1;
      highlight_depth = depth1;
    }
    else {
      previous_line_was_highlighted = false;
      auto iter = start_of_line;
      while (iter != end()) {
        if (*iter == '\n') {
          ++iter;
          break;
        }
        ++iter;
      }
      start_of_line = iter;
    }
  } while (start_of_line != end());
}

std::pair<unsigned, unsigned> Source::getLineAndColumn(iterator position) {
  unsigned line = 1;
  unsigned column = 1;
  for (iterator it = begin(); it != position; ++it) {
    if (*it == '\n') {
      ++line;
      column = 1;
    }
    else
      ++column;
  }
  return std::make_pair(line, column);
}
