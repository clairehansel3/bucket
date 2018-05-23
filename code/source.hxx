#ifndef BUCKET_SOURCE_FILE_HXX
#define BUCKET_SOURCE_FILE_HXX

#include <memory>

class SourceFile {
public:
  using Position = const char*;
  explicit SourceFile(const char* path);
  int character() const noexcept;
  Position position() const noexcept;
  void next();
private:
  std::unique_ptr<char[]> m_begin;
  const char* m_iter;
  const char* m_end;
};

#endif
