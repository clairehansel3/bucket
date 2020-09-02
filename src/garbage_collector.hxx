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

#ifndef BUCKET_GARBAGE_COLLECTOR_HXX
#define BUCKET_GARBAGE_COLLECTOR_HXX

#include <atomic>
#include <boost/noncopyable.hpp>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class GarbageCollectable;

class GarbageCollector : boost::noncopyable
{
// A tracing garbage collector. When constructed, a new thread is started that
// handles garbage collection. This thread is stopped and all resources are
// freed when the GarbageCollector object is destructed.

public:

  template <typename T>
  using root_ptr = std::unique_ptr<T, void(*)(T*)>;

  explicit GarbageCollector();
  // Initializes the garbage collector and begins running the garbage collector
  // thread.

  ~GarbageCollector();
  // Terminates the garbage collector thread and frees all tracked objects.

  template <typename T, typename... Args>
  T* create_root(Args&&... args)
  // Creates an object of type T where T must be a subclass of
  // GarbageCollectable. This object is owned by the garbage collector but is
  // considered a member of the root set unless '.demote()' is called on the
  // returned object.
  {
    static_assert(std::is_base_of_v<GarbageCollectable, T>);
    auto object_uptr = std::make_unique<T>(std::forward<Args>(args)...);
    auto object_ptr = object_uptr.get();
    std::unique_lock<std::mutex> objects_lock{objects_mutex};
    objects.push_back(std::move(object_uptr));
    return object_ptr;
  }

  template <typename T, typename... Args>
  root_ptr<T> create(Args&&... args)
  // Creates an object of type T where T must be a subclass of
  // GarbageCollectable. This object is owned by the garbage collector. This
  // function returns a unique pointer to the object with a custom deleter.
  // Essentially this object is a root object and thus won't be GC'd until the
  // custom unique_ptr goes out of scope, after which it will become a regular
  // object.
  {
    static_assert(std::is_base_of_v<GarbageCollectable, T>);
    auto object_uptr = std::make_unique<T>(std::forward<Args>(args)...);
    static void(*deleter)(T*) = [](T* ptr){--(ptr->root_counter);};
    auto result = root_ptr<T>(object_uptr.get(), deleter);
    std::unique_lock<std::mutex> objects_lock{objects_mutex};
    objects.push_back(std::move(object_uptr));
    return result;
  }

private:

  static constexpr auto collection_delay = std::chrono::milliseconds(100);

  std::vector<std::unique_ptr<GarbageCollectable>> objects;
  std::mutex objects_mutex;
  bool terminate_flag;
  std::mutex terminate_flag_mutex;
  std::condition_variable terminate_condition;
  std::thread gc_thread;

  void run() noexcept;

};

class GarbageCollectable {
// The base class for all objects which are tracked by the garbage collector.
// Each subclass is required to implement a 'trace()'.

friend class GarbageCollector;

public:

  GarbageCollectable();
  // Creates a GarbageCollectable object. Note that this does not mean the
  // object is managed by the garbage collector. To do this you should use the
  // 'create' method on a GarbageCollector object. Note that after this is done,
  // the resulting object will be part of the root set and so will never be
  // garbage collected (although if the GarbageCollector object goes out of
  // scope it will be deleted). To demote a GarbageCollectable object so it is
  // no longer part of the root set, you must call '.demote()'.

  virtual ~GarbageCollectable() = default;

  void mark();
  // Marks the object as safe from garbage collection, and then calls 'trace'.
  // This should only be called when implementing a 'trace()' method.

  std::atomic<unsigned short> root_counter;

protected:

  std::mutex mutex;

private:

  virtual void trace() = 0;
  // This method should call 'mark' on every other object referenced by the
  // object.

  std::atomic<bool> is_marked;

};

#endif
