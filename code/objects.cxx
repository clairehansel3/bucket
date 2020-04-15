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

#include "objects.hxx"
#include "miscellaneous.hxx"

Object* Object::get(const std::string& name)
{
  std::unique_lock<std::mutex> lock{mutex};
  auto result = map.find(name);
  return result == map.end() ? nullptr : result->second;
}

void Object::set(std::string name, Object* object)
{
  std::unique_lock<std::mutex> lock{mutex};
  BUCKET_ASSERT(object);
  map[std::move(name)] = object;
}

void Object::del(const std::string& name)
{
  std::unique_lock<std::mutex> lock{mutex};
  auto result = map.find(name);
  BUCKET_ASSERT(result != map.end());
  map.erase(result);
}

void Object::trace()
{
  std::unique_lock<std::mutex> lock{mutex};
  for (auto& iter : map)
    iter.second->mark();
}
