
#ifndef __NUMBER_H__
#define __NUMBER_H__

// I N C L U D E S ////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

#include <stdint.h>

namespace number {

// D E F I N E S //////////////////////////////////////////////////////////////

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
      #define IS_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
      #define IS_BIG_ENDIAN 0
#else
    #error "I don't know what architecture this is!"
#endif

// C L A S S E S //////////////////////////////////////////////////////////////

template <class T>
class base
{
public:
  base() { };
  base(const base<T>& _var) : var(_var.var) { };
  base(const T _var) : var(_var) { };
  base(const base<T>&& _var) : var(_var.var) { };
  ~base() { };

  operator T() const { return var; };
  base<T>& operator=(base<T>&& rhs) { var = rhs.var; return *this; };
  explicit operator std::string() const { return std::to_string(var); };
  static int size() { return sizeof(T); };
  std::string serialize() const
  { 
#if IS_BIG_ENDIAN == 1
    return std::string(reinterpret_cast<const char*>(&var), sizeof(var));
#else
    char endian_converted[sizeof(var)];
    auto s = reinterpret_cast<const char*>(&var);
    for (unsigned int i = 0; i < sizeof(var); ++i)
      endian_converted[i] = s[sizeof(var) - i - 1];
    return std::string(endian_converted, sizeof(var));
#endif
  };

  int deserialize(const std::string& s)
  {
#if IS_BIG_ENDIAN == 1
    var=*(reinterpret_cast<const T*>(s.data()));
#else
    char endian_converted[sizeof(var)];
    for (unsigned int i = 0; i < sizeof(var); ++i)
      endian_converted[i] = s[sizeof(var) - i - 1];
    var=*(reinterpret_cast<T*>(endian_converted));
#endif
    return sizeof(var);
  };

  T var;
};

template <class T>
class weak : public base<T>
{
public:
  using base<T>::base;

/*
  auto operator x (const weak<long double>& i) y -> decltype(base<T>::var x i.var) { return base<T>::var x i.var; };	\
  auto operator x (const weak<double>& i) y { return base<T>::var x i.var; };		\
  auto operator x (const weak<float>& i) y { return base<T>::var x i.var; };		\
  auto operator x (const weak<unsigned long long>& i) y { return base<T>::var x i.var; };	\
  auto operator x (const weak<long long>& i) y { return base<T>::var x i.var; };	\
  auto operator x (const weak<unsigned long>& i) y { return base<T>::var x i.var; };	\
  auto operator x (const weak<long>& i) y { return base<T>::var x i.var; };		\
  auto operator x (const weak<unsigned int>& i) y { return base<T>::var x i.var; };	\
  auto operator x (const weak<int>& i) y { return base<T>::var x i.var; };		\
  auto operator x (const weak<unsigned short>& i) y { return base<T>::var x i.var; };	\
  auto operator x (const weak<short>& i) y { return base<T>::var x i.var; };		\
  auto operator x (const weak<unsigned char>& i) y { return base<T>::var x i.var; };	\
  auto operator x (const weak<char>& i) y { return base<T>::var x i.var; };		\
  auto operator x (const weak<bool>& i) y { return base<T>::var x i.var; };		\
*/
  #define DECLARE_HELPER_INTEGERS(x, y) \
  auto operator x (const unsigned long long i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };		\
  auto operator x (const long long i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };			\
  auto operator x (const unsigned long i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };		\
  auto operator x (const long i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };			\
  auto operator x (const unsigned int i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };		\
  auto operator x (const int i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };				\
  auto operator x (const unsigned short i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };		\
  auto operator x (const short i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };			\
  auto operator x (const unsigned char i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };		\
  auto operator x (const char i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };			\
  auto operator x (const bool i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };

  #define DECLARE_HELPER_FLOATS(x, y) \
  auto operator x (const long double i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };			\
  auto operator x (const double i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };			\
  auto operator x (const float i) y -> decltype(base<T>::var x i) { return base<T>::var x i; };

  #define DECLARE_HELPER(x, y) \
  DECLARE_HELPER_INTEGERS(x, y) \
  DECLARE_HELPER_FLOATS(x, y)

  #define DECLARE(x)       DECLARE_HELPER(x, )
  #define DECLARE_CONST(x) DECLARE_HELPER(x, const)
  #define DECLARE_INTEGERS(x)       DECLARE_HELPER_INTEGERS(x, )
  #define DECLARE_INTEGERS_CONST(x) DECLARE_HELPER_INTEGERS(x, const)
  #define DECLARE_UNARY(x) auto operator x () -> decltype(x base<T>::var) { return x base<T>::var; };
  #define DECLARE_UNARY_CONST(x) auto operator x () const -> decltype(x base<T>::var) { return x base<T>::var; };

  DECLARE_INTEGERS_CONST(%);
  DECLARE_CONST(+);
  DECLARE_CONST(-);
  DECLARE_CONST(*);
  DECLARE_CONST(/);
  DECLARE_INTEGERS_CONST(&);
  DECLARE_INTEGERS_CONST(|);
  DECLARE_INTEGERS_CONST(^);
  DECLARE_INTEGERS_CONST(<<);
  DECLARE_INTEGERS_CONST(>>);
  DECLARE(=);
  DECLARE_INTEGERS(%=);
  DECLARE(+=);
  DECLARE(-=);
  DECLARE(*=);
  DECLARE(/=);
  DECLARE_INTEGERS(&=);
  DECLARE_INTEGERS(|=);
  DECLARE_INTEGERS(^=);
  DECLARE_INTEGERS(<<=);
  DECLARE_INTEGERS(>>=);
  DECLARE_CONST(==);
  DECLARE_CONST(!=);
  DECLARE_CONST(>);
  DECLARE_CONST(<);
  DECLARE_CONST(>=);
  DECLARE_CONST(<=);
  DECLARE_UNARY(++);
  DECLARE_UNARY(--);
  DECLARE_UNARY_CONST(+);
  DECLARE_UNARY_CONST(-);
  DECLARE_UNARY_CONST(~);
  DECLARE_UNARY_CONST(!);
};
/*
template <class T>
class strong : public base<T>
{
public:
  using base<T>::base;
  
//  auto operator x (const strong<long double>& i) y -> decltype(strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var)) { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };	\
  #undef DECLARE_HELPER
  #define DECLARE_HELPER(x, y) \
  auto operator x (const strong<long double>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<double>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<float>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<unsigned long long>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };	\
  auto operator x (const strong<long long>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };	\
  auto operator x (const strong<unsigned long>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };\
  auto operator x (const strong<long>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<unsigned int>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };	\
  auto operator x (const strong<int>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<unsigned short>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };	\
  auto operator x (const strong<short>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<unsigned char>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };\
  auto operator x (const strong<char>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const strong<bool>& i) y { return strong<decltype(base<T>::var x i.var)>(base<T>::var x i.var); };		\
  auto operator x (const long double i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };			\
  auto operator x (const double i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };				\
  auto operator x (const float i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };				\
  auto operator x (const unsigned long long i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };		\
  auto operator x (const long long i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };			\
  auto operator x (const unsigned long i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };			\
  auto operator x (const long i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };				\
  auto operator x (const unsigned int i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };			\
  auto operator x (const int i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };				\
  auto operator x (const unsigned short i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };			\
  auto operator x (const short i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };				\
  auto operator x (const unsigned char i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };			\
  auto operator x (const char i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };				\
  auto operator x (const bool i) y { return strong<decltype(base<T>::var x i)>(base<T>::var x i); };

  #undef DECLARE
  #undef DECLARE_CONST
  #undef DECLARE_UNARY
  #undef DECLARE_UNARY_CONST
  #define DECLARE(x)       DECLARE_HELPER(x, )
  #define DECLARE_CONST(x) DECLARE_HELPER(x, const)
  #define DECLARE_UNARY(x) auto operator x () { return strong<decltype(x base<T>::var)>(x base<T>::var); };
  #define DECLARE_UNARY_CONST(x) auto operator x () const { return strong<decltype(x base<T>::var)>(x base<T>::var); };

  DECLARE_CONST(%);
  DECLARE_CONST(+);
  DECLARE_CONST(-);
  DECLARE_CONST(*);
  DECLARE_CONST(/);
  DECLARE_CONST(&);
  DECLARE_CONST(|);
  DECLARE_CONST(^);
  DECLARE_CONST(<<);
  DECLARE_CONST(>>);
  DECLARE(=);
  DECLARE(%=);
  DECLARE(+=);
  DECLARE(-=);
  DECLARE(*=);
  DECLARE(/=);
  DECLARE(&=);
  DECLARE(|=);
  DECLARE(^=);
  DECLARE(<<=);
  DECLARE(>>=);
  DECLARE_CONST(==);
  DECLARE_CONST(!=);
  DECLARE_CONST(>);
  DECLARE_CONST(<);
  DECLARE_CONST(>=);
  DECLARE_CONST(<=);
  DECLARE_UNARY(++);
  DECLARE_UNARY(--);
  DECLARE_UNARY_CONST(+);
  DECLARE_UNARY_CONST(-);
  DECLARE_UNARY_CONST(~);
  DECLARE_UNARY_CONST(!);
};
*/
///////////////////////////////////////////////////////////////////////////////

};	// end namespace "number"

#endif	// __NUMBER_H__