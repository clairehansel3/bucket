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
#include <vector>

class GarbageCollectable;

class GarbageCollector : boost::noncopyable {
// A tracing garbage collector. When constructed, a new thread is started that
// handles garbage collection. This thread is stopped and all resources are freed
// when the GarbageCollector object is destructed.

public:

  explicit GarbageCollector(std::unique_ptr<GarbageCollectable> root_object);
  // Initializes object and creates garbage collector thread.

  ~GarbageCollector();
  // Terminates the garbage collector thread and frees all tracked objects.

  void track(std::unique_ptr<GarbageCollectable> object);
  // Takes on ownership of a new object.

  static constexpr auto collection_delay = std::chrono::milliseconds(100);

private:

  GarbageCollectable* root_object_ptr;
  std::vector<std::unique_ptr<GarbageCollectable>> tracked_objects;
  std::mutex tracked_objects_mutex;
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
  // Creates a GarbageCollectable object. Note that this does not mean the object
  // is managed by the garbage collector yet. To do that you must call
  // GarbageCollector.track(std::move(object_uptr)). Be VERY careful with
  // GarbageCollectable objects that are untracked: keep in mind that anything
  // they reference can be deleted if those things aren't referenced by any
  // tracked objects.

  virtual ~GarbageCollectable() = default;

protected:

  std::mutex mutex;
  // Locking this prevents the garbage collector from calling 'trace' on the
  // object when the object is being modified. It should be done whenever
  // accessing something also accessed by trace

  void mark();
  // Marks the object as safe from garbage collection, and then calls 'trace'.

private:

  virtual void trace() = 0;
  // This method should call 'mark' on every other object referenced by the
  // object. This should lock 'object_mutex' first thing in the function.

  std::atomic<bool> is_marked;

};

#endif
