#include "source_file.hxx"
#include "miscellaneous.hxx"
#include <boost/numeric/conversion/cast.hpp>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <optional>
#include <limits>
#include <sstream>
#include <type_traits>

SourceFile::SourceFile(const char* path)
: m_path{path}
{
  assert(path);
  try {
    std::ifstream file;
    file.exceptions(file.failbit | file.badbit);
    file.open(path, file.binary);
    file.seekg(0, file.end);
    auto file_size = boost::numeric_cast<std::size_t>(
      std::streamoff(file.tellg())
    );
    file.seekg(0, file.beg);
    m_buffer = std::make_unique<char[]>(file_size);
    m_begin  = m_buffer.get();
    m_end    = m_begin + file_size;
    file.read(m_begin, file_size);
  } catch (const std::ifstream::failure&) {
    throw make_runtime_error("unable to open file '", m_path, "'");
  }
  if (utf8::starts_with_bom(m_begin, m_end))
    m_begin += 3;
  if (!utf8::is_valid(m_begin, m_end))
    throw make_runtime_error("file '", m_path, "' contains invalid utf8");
}

SourceFile::iterator SourceFile::begin()
{
  #ifdef BUCKET_DEBUG
  return iterator(m_begin, m_begin, m_end);
  #else
  return iterator(m_begin);
  #endif
}

SourceFile::iterator SourceFile::end()
{
  #ifdef BUCKET_DEBUG
  return iterator(m_end, m_begin, m_end);
  #else
  return iterator(m_end);
  #endif
}

void SourceFile::highlight(std::ostream& stream, iterator position)
{
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

void SourceFile::highlight(std::ostream& stream, iterator_range range)
{
  // print header
  auto [line, column] = getLineAndColumn(range.first);
  stream << "file '" BUCKET_BOLD << m_path << BUCKET_BLACK "': starting from li"
         "ne " << line << ", column " << column << ":\n|";
  auto start_of_line = range.first;
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
  while (iter != range.first) {
    utf8::append(*iter, std::ostream_iterator<char>(stream));
    ++number_of_underline_spaces;
    ++iter;
  }
  stream << BUCKET_RED;
  // write rest of the first line
  while (true) {
    if (iter == range.second) {
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
  while (iter != range.second) {
    stream << "|" BUCKET_RED;
    number_of_underline_highlights = 0;
    while (*iter != '\n') {
      utf8::append(*iter, std::ostream_iterator<char>(stream));
      ++number_of_underline_highlights;
      ++iter;
      if (iter == range.second) {
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

void SourceFile::highlight(
  std::ostream& stream,
  iterator_range_list const& ranges)
{
  iterator start_of_line = begin();
  // iterator to the first character in the line

  unsigned highlight_depth = 0;
  // Every time you enter a range this is incremented and every time you exit a
  // range it is decremented. If this is zero then the characters shouldn't be
  // highlighted but if its nonzero characters should be highlighted.

  bool previous_line_was_highlighted = false;
  // true if the previous line had any highlighted characters on it.

  bool any_line_was_highlighted = false;
  // true if any previous line had any highlighted characters on it.

  auto getFirstCharacterToHighlight = [&, this]() -> std::optional<iterator>
  // Returns an iterator to the first character on the line starting at
  // 'start_of_line' that is highlighted. If no characters on this line need to
  // be highlighted, this function returns an empty optional.
  {
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
        return std::make_optional(iter);
    }
    //highlight_depth = new_highlight_depth;// LINE SUSPECT
    return std::nullopt;
  };

  auto highlightCharacters = [&, this](
    auto output_normal,
    auto output_highlight
  )
  // Iterates through the line that starts with 'start_of_line' and highlights
  // all the characters, outputing the result to 'stream'. 'output_normal' and
  // 'output_highlight' both take an iterator and write the unhighlighted and
  // highlighted versions of the character respectively. This is done so
  // this function can both highlight and underline characters. This function
  // returns a std::pair containing the iterator to the start of the next line
  // (or the end of the file) and the highlight depth at that point.
  {
    auto iter = start_of_line;
    // current character

    auto new_highlight_depth = highlight_depth;
    // highlight depth at current character

    while (true) {

      // update highlight depth
      for (auto range : ranges) {
        if (iter == range.first)
          ++new_highlight_depth;
        if (iter == range.second) {
          assert(new_highlight_depth);
          --new_highlight_depth;
        }
      }

      // check for end of line or file
      if (iter == end()) {
        assert(!new_highlight_depth);
        break;
      }
      if (*iter == '\n') {
        ++iter;
        break;
      }

      // highlight (or don't)
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
    auto first_highlight_on_line = getFirstCharacterToHighlight();
    if (first_highlight_on_line) {
      if (!previous_line_was_highlighted) {
        if (any_line_was_highlighted)
          stream << "...\n";
        // this line needs to be highlighted and the previous line wasn't
        auto [line, column] = getLineAndColumn(*first_highlight_on_line);
        stream << "file '" BUCKET_BOLD << m_path << BUCKET_BLACK "': start"
          "ing from line " << line << ", column " << column << ":\n";
      }

      // highlight line
      stream << '|';
      auto [iter1, depth1] = highlightCharacters(
        [&stream](iterator iter){
          utf8::append(*iter, std::ostream_iterator<char>(stream));
        },
        [&stream](iterator iter){
          utf8::append(*iter, std::ostream_iterator<char>(stream));
        }
      );

      // underline line
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
      // line does not need to be highlighted
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

std::string SourceFile::highlight(iterator position)
{
  std::stringstream ss;
  highlight(ss, position);
  return ss.str();
}

std::string SourceFile::highlight(iterator_range range)
{
  std::stringstream ss;
  highlight(ss, range);
  return ss.str();
}

std::string SourceFile::highlight(iterator_range_list const& ranges)
{
  std::stringstream ss;
  highlight(ss, ranges);
  return ss.str();
}

std::pair<unsigned, unsigned> SourceFile::getLineAndColumn(iterator position)
{
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
