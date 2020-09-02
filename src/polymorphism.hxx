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

#ifndef BUCKET_POLYMORPHISM_HXX
#define BUCKET_POLYMORPHISM_HXX

// Introduction:
// polymorphism.hxx is a really unique header file that implements the Visitor
// pattern and a constant time version of dynamic_cast<> all without rtti.
// Unfortunately it is macro heavy but here is a good explanation of how to use
// it and what it does:
//
// Some Important Notes:
// (1) Only one polymorphic class heirarchy can be created per namespace. If you
//     create a polymorphic class heirarchy please put it in it's own namespace.
// (2) All of the macros in polymorphism.hxx begin with `BUCKET_POLYMORPHISM_`.
// (3) None of the macros in polymorphism.hxx should be followed by a semicolon.
// (4) Always use 'class' and not 'struct' when creating a class in a heirarchy.
//     This is because BUCKET_POLYMORPHISM_BASE_CLASS() and
//     BUCKET_POLYMORPHISM_DERIVED_CLASS() contain `private:` and so putting
//     them in a struct would be misleading because then the struct members
//     would default to private.
// (4) The following are defined when you create a polymorphic class heirarchy:
//
//  Visitor:
//    • Abstract base class for visitor classes.
//    • Protected nonvirtual constructor and destructor.
//    • Should not be used directly but should be inherited from.
//    • When you inherit from visitor, you should implement the member functions
//      `void visit(T&) override;` for some or all of the classes in the
//      heirarchy. These member functions should be private. The default 'visit'
//      member functions calls BUCKET_UNREACHABLE().
//
//  dispatch(visitor, object)
//    • Dispatches a visitor to an object
//
//  cast<Class*>(instance_ptr)
//    • Similar to 'dynamic_cast' except no RTTI is required and it works in
//      constant time. Upcasts and casts to the same type are devirtualized.
//
// Example of how to use polymorphism.hxx:
//
// namespace example {
//
// BUCKET_POLYMORPHISM_INIT()
//
// class Base {
//   BUCKET_POLYMORPHISM_BASE()
// public:
//   virtual ~Base() = default;
// };
//
// class Derived1 : public Base {
//   BUCKET_POLYMORPHISM_DERIVED()
// };
//
// class Derived2 : public Base {
//   BUCKET_POLYMORPHISM_DERIVED()
// };
//
// class Derived3 : public Derived2 {
//   BUCKET_POLYMORPHISM_DERIVED()
// };
//
// BUCKET_POLYMORPHISM_SETUP_BASE_1(Base)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Derived1)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Derived2)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Derived3)
// BUCKET_POLYMORPHISM_SETUP_BASE_2(Base)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Derived1)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Derived2)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Derived3)
// BUCKET_POLYMORPHISM_SETUP_BASE_3(Base)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Derived1)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Derived2)
// BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Derived3)
//
// }
//

#include "miscellaneous.hxx"
#include <optional>
#include <type_traits>

#define BUCKET_POLYMORPHISM_INIT()                                             \
class Visitor;

#define BUCKET_POLYMORPHISM_BASE_CLASS()                                       \
private:                                                                       \
  template <typename T, typename VisitorType>                                  \
  friend void dispatch(T&, VisitorType&);                                      \
  virtual void receive(Visitor&);

#define BUCKET_POLYMORPHISM_DERIVED_CLASS()                                    \
private:                                                                       \
  template <typename T, typename VisitorType>                                  \
  friend void dispatch(T&, VisitorType&);                                      \
  void receive(Visitor&) override;

#define BUCKET_POLYMORPHISM_SETUP_BASE_1(base)                                 \
class Visitor                                                                  \
{                                                                              \
protected:                                                                     \
  Visitor() = default;                                                         \
  ~Visitor() = default;                                                        \
private:                                                                       \
  template <typename T, typename VisitorType>                                  \
  friend void dispatch(T&, VisitorType&);                                      \
  friend class base;                                                           \
  virtual void visit(base&) {BUCKET_UNREACHABLE();}

#define BUCKET_POLYMORPHISM_SETUP_DERIVED_1(derived)                           \
  friend class derived;                                                        \
  virtual void visit(derived&) {BUCKET_UNREACHABLE();}

#define BUCKET_POLYMORPHISM_SETUP_BASE_2(base)                                 \
};                                                                             \
                                                                               \
template <typename VisitorType, typename T>                                    \
void dispatch(VisitorType& visitor, T& obj)                                    \
{                                                                              \
  static_assert(std::is_base_of_v<base, T>);                                   \
  static_assert(std::is_base_of_v<Visitor, VisitorType>);                      \
  obj.receive(visitor);                                                        \
}                                                                              \
                                                                               \
namespace details                                                              \
{                                                                              \
                                                                               \
template <typename To>                                                         \
class Caster final : public Visitor                                            \
{                                                                              \
public:                                                                        \
  To* m_result;                                                                \
private:                                                                       \
  void visit(base& ref) override {cast(ref);}

#define BUCKET_POLYMORPHISM_SETUP_DERIVED_2(derived)                           \
  void visit(derived& ref) override {cast(ref);}

#define BUCKET_POLYMORPHISM_SETUP_BASE_3(base)                                 \
  template <typename From>                                                     \
  void cast([[maybe_unused]] From& ref)                                        \
  {                                                                            \
    if constexpr (std::is_base_of_v<To, From>)                                 \
      m_result = static_cast<To*>(&ref);                                       \
    else                                                                       \
      m_result = nullptr;                                                      \
  }                                                                            \
};                                                                             \
                                                                               \
}                                                                              \
                                                                               \
template <typename ToPtr, typename FromPtr>                                    \
std::enable_if_t<std::is_pointer_v<ToPtr> && std::is_pointer_v<FromPtr>,       \
ToPtr> cast(FromPtr ptr) noexcept                                              \
{                                                                              \
  using To   = std::remove_pointer_t<ToPtr>;                                   \
  using From = std::remove_pointer_t<FromPtr>;                                 \
  static_assert(std::is_base_of_v<base, To>);                                  \
  static_assert(std::is_base_of_v<base, From>);                                \
  if constexpr (std::is_same_v<To, From>)                                      \
    return ptr;                                                                \
  if constexpr (std::is_base_of_v<To, From>)                                   \
    return static_cast<ToPtr>(ptr);                                            \
  if (!ptr)                                                                    \
    return nullptr;                                                            \
  details::Caster<To> caster;                                                  \
  dispatch(caster, *ptr);                                                      \
  return caster.m_result;                                                      \
}                                                                              \
                                                                               \
inline void base::receive(Visitor& ref) {ref.visit(*this);}

#define BUCKET_POLYMORPHISM_SETUP_DERIVED_3(derived)                           \
inline void derived::receive(Visitor& ref) {ref.visit(*this);}

#endif
