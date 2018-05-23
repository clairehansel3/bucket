#include "source.hxx"
#include <boost/format.hpp>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>

SourceFile::SourceFile(const char* path) {
  try {
    std::ifstream file;
    file.exceptions(file.failbit | file.badbit);
    file.open(path, file.binary);
    file.seekg(0, file.end);
    std::streampos file_size = file.tellg();
    file.seekg(0, file.beg);
    assert(file_size >= 0);
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-type-limit-compare"
    if (static_cast<std::make_unsigned_t<std::streamoff>>(file_size) > std::numeric_limits<std::size_t>::max())
      throw std::runtime_error((boost::format("unable to open file '%1%' because the file size exceeds %2%") % path % std::numeric_limits<std::size_t>::max()).str());
    #pragma clang diagnostic pop
    m_begin = std::make_unique<char[]>(static_cast<std::size_t>(file_size));
    m_iter = m_begin.get();
    m_end = m_begin.get() + file_size;
    file.read(m_begin.get(), file_size);
  } catch (const std::ifstream::failure&) {
    throw std::runtime_error((boost::format("failed to read file '%1%'") % path).str());
  }
}

int SourceFile::character() const noexcept {
  return (m_iter == m_end) ? -1 : *m_iter;
}

SourceFile::Position SourceFile::position() const noexcept {
  return m_iter;
}

void SourceFile::next() {
  if (m_iter != m_end)
    ++m_iter;
}
