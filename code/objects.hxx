// Copyright (C) 2020  Claire Hansel
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

#ifndef BUCKET_OBJECTS_HXX
#define BUCKET_OBJECTS_HXX

#include "garbage_collector.hxx"
#include <map>
#include <string>

class Object : public GarbageCollectable {
public:
  Object* get(const std::string& name);
  void set(std::string name, Object* object);
  void del(const std::string& name);
private:
  std::map<std::string, Object*> map;
  void trace() override;
};

#endif
