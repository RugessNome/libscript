// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SOURCE_FILE_H
#define LIBSCRIPT_SOURCE_FILE_H

#include "libscriptdefs.h"

namespace script {

struct SourceFileImpl;

class LIBSCRIPT_API SourceFile
{
public:
  SourceFile() = default;
  SourceFile(const SourceFile &) = default;
  ~SourceFile() = default;
  explicit SourceFile(const std::string & path);
  explicit SourceFile(const std::shared_ptr<SourceFileImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  typedef size_t Offset;

  struct Position {
    Offset pos;
    uint16 line;
    uint16 col;
  };

  const std::string & filepath() const;

  Position map(Offset off) const;

  void load();
  bool isLoaded() const;
  bool isLocked() const;
  void unload();

  const char * data() const;
  const std::string & content() const;

  static SourceFile fromString(const std::string & src);

  inline const std::shared_ptr<SourceFileImpl> & impl() const { return d; }

private:
  std::shared_ptr<SourceFileImpl> d;
};

struct LIBSCRIPT_API SourceLocation
{
  SourceFile m_source;
  SourceFile::Position m_pos;
};

} // namespace script

#endif // LIBSCRIPT_SOURCE_FILE_H
