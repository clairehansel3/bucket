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

#include "garbage_collector.hxx"
#include "miscellaneous.hxx"
#include <cassert>
#include <exception>
#include <functional>

GarbageCollector::GarbageCollector()
{
  gc_thread = std::thread(std::bind(&GarbageCollector::run, this));
}

GarbageCollector::~GarbageCollector()
{
  try {
    {
      std::unique_lock<std::mutex> terminate_flag_lock{terminate_flag_mutex};
      terminate_flag = true;
      terminate_condition.notify_one();
    }
    gc_thread.join();
  } catch (std::exception& e) {
    error("garbage collector", "unable to terminate: ", e.what());
  }
}

void GarbageCollector::run() noexcept
{
  try {
    while (true) {
      {
        std::unique_lock<std::mutex> terminate_flag_lock{terminate_flag_mutex};
        terminate_condition.wait_for(
          terminate_flag_lock, collection_delay,
          [this]{return terminate_flag;}
        );
        if (terminate_flag)
          return;
      }
      {
        std::unique_lock<std::mutex> objects_lock{objects_mutex};
        for (auto& ptr : objects)
          if (!ptr->root_counter)
            ptr->mark();
        decltype(objects)::size_type i = 0;
        while (i != objects.size()) {
          if (objects[i]->is_marked) {
            objects[i]->is_marked = false;
            ++i;
          }
          else {
            objects.erase(
              objects.begin()
              + static_cast<decltype(objects)::difference_type>(i)
            );
          }
        }
      }
    }
  } catch (std::exception& e) {
    error("garbage collector", "cycle failed: ", e.what());
  }
}


GarbageCollectable::GarbageCollectable()
: root_counter{1}, is_marked{true}
{}

void GarbageCollectable::mark()
{
  if (!is_marked) {
    is_marked = true;
    std::unique_lock<std::mutex> lock{mutex};
    trace();
  }
}
