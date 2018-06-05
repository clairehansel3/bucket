#ifndef BUCKET_SOURCE_HXX
#define BUCKET_SOURCE_HXX

#include <memory>

class SourceFile {
public:
  struct Position {
    unsigned line, column;
    constexpr Position() : line{0}, column{0} {}
    constexpr Position(unsigned line, unsigned column) : line{line}, column{column} {}
  };
  explicit SourceFile(const char* path);
  int character() const noexcept;
  Position position() const noexcept;
  void next() noexcept;
private:
  Position m_position;
  std::unique_ptr<char[]> m_begin;
  const char* m_iter;
  const char* m_end;
};

#endif
