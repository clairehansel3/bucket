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
#include <exception>
#include <functional>
#include <iostream>
#include <utility>

GarbageCollector::GarbageCollector(std::unique_ptr<GarbageCollectable> root_object)
{
  root_object_ptr = root_object.get();
  terminate_flag = false;
  tracked_objects.push_back(std::move(root_object));
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
    std::cout << concatenate("failed to terminate garbage collector: ", e.what()) << std::endl;
    //printErrorMessage("failed to terminate garbage collector: ", e.what());
    std::terminate();
  }
}

void GarbageCollector::track(std::unique_ptr<GarbageCollectable> object)
{
  std::unique_lock<std::mutex> tracked_objects_lock{tracked_objects_mutex};
  BUCKET_ASSERT(object->is_marked);
  tracked_objects.push_back(std::move(object));
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
        std::cout << "--> collecting garbage" << std::endl;
        std::cout << "--> marking" << std::endl;
        root_object_ptr->mark();
        std::cout << "--> sweeping" << std::endl;
        std::unique_lock<std::mutex> tracked_objects_lock{tracked_objects_mutex};
        decltype(tracked_objects)::size_type i = 0;
        while (i != tracked_objects.size()) {
          if (tracked_objects[i]->is_marked) {
            tracked_objects[i]->is_marked = false;
            ++i;
          }
          else {
            std::cout << "--> deleting object " << tracked_objects[i].get() << std::endl;
            tracked_objects.erase(tracked_objects.begin() + static_cast<decltype(tracked_objects)::difference_type>(i));
          }
        }
      }
    }
  } catch (std::exception& e) {
    std::cout << concatenate("garbage collector cycle failed: ", e.what()) << std::endl;
    //printErrorMessage("garbage collector cycle failed: ", e.what());
    return;
  }
}

GarbageCollectable::GarbageCollectable()
: is_marked{true}
{}

void GarbageCollectable::mark()
{
  if (!is_marked) {
    std::cout << "--> marking object " << this << std::endl;
    is_marked = true;
    trace();
  }
}
