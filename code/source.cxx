#include "source.hxx"
#include "misc.hxx"
#include <cassert>
#include <cstddef>
#include <fstream>
#include <limits>
#include <type_traits>

SourceFile::SourceFile(const char* path)
: m_position{1, 1} {
  try {
    std::ifstream file;
    file.exceptions(file.failbit | file.badbit);
    file.open(path, file.binary);
    file.seekg(0, file.end);
    std::streampos file_size = file.tellg();
    file.seekg(0, file.beg);
    assert(file_size >= 0);
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-type-limit-compare"
    #endif
    if (static_cast<std::make_unsigned_t<std::streamoff>>(file_size) > std::numeric_limits<std::size_t>::max())
      misc::error("unable to open file '", path, "' because the file size exceeds ", std::numeric_limits<std::size_t>::max());
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
    m_begin = std::make_unique<char[]>(static_cast<std::size_t>(file_size));
    m_iter = m_begin.get();
    m_end = m_begin.get() + file_size;
    file.read(m_begin.get(), file_size);
  } catch (const std::ifstream::failure&) {
    misc::error("failed to read file '", path, "'");
  }
}

int SourceFile::character() const noexcept {
  return (m_iter == m_end) ? -1 : *m_iter;
}

SourceFile::Position SourceFile::position() const noexcept {
  return m_position;
}

void SourceFile::next() noexcept {
  if (m_iter == m_end)
    return;
  ++m_iter;
  if (m_iter == m_end)
    m_position.line = m_position.column = 0;
  else if (*m_iter == '\n')
    ++m_position.line;
  else
    ++m_position.column;
}
