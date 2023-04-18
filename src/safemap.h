#ifndef __SAFEMAP_H__
#define __SAFEMAP_H__
 
/*
safe::map: std::map with threadsafe iterators that don't invalidate upon
calls by other threads to erase()

License: Public domain

*/


// I N C L U D E S ////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <algorithm>
#include <type_traits>
#include <atomic>
#include <memory>
#include <deque>

#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "number.h"

namespace safe
{

//#define DEBUG
//#define DEBUG_IMMEDIATELY
//#define SEGFAULT_ON_ASSERT

// D E F I N E S //////////////////////////////////////////////////////////////

#ifdef DEBUG
 #ifndef DEBUG_IMMEDIATELY
  #define MAX_DEBUG	30000
 #endif

 #define SAFE_FIRST(x)	((x) == map_real_end() ? -1 : (x)->first)

 #define DEBUG_SIMPLE	{ char buf[512]; sprintf(buf, "%s%d: %s", safe::get_thread_spacing().c_str(), __LINE__,__FUNCTION__); safe::debug(buf); }
 #define DEBUG_THIS	{ char buf[512]; sprintf(buf, "%s%d:%p %s", safe::get_thread_spacing().c_str(), __LINE__, this, __FUNCTION__); safe::debug(buf); }
 #define DEBUG_FIRST	{ char buf[512]; sprintf(buf, "%s%d:%p:%d %s %d%d", safe::get_thread_spacing().c_str(), __LINE__, this, SAFE_FIRST(*this), __FUNCTION__, *this == map_real_begin(), *this == map_real_end()); safe::debug(buf); }
 #define DEBUG_FIRST_ITER { char buf[512]; sprintf(buf, "%s%d:%p:%d %s %p:%d", safe::get_thread_spacing().c_str(), __LINE__, this, SAFE_FIRST(*this), __FUNCTION__, &_iter, SAFE_FIRST(_iter)); safe::debug(buf); }
 #define DEBUG_REFCOUNT	{ char buf[512]; sprintf(buf, "%s%d:%p:%d %s %d", safe::get_thread_spacing().c_str(), __LINE__, this, SAFE_FIRST(*this), __FUNCTION__, int((*this)->second._reference_count)); safe::debug(buf); }
 #define DEBUG_ITER	{ char buf[512]; sprintf(buf, "%s%d:%p:%d %s %d%d", safe::get_thread_spacing().c_str(), __LINE__, &iter, iter->first, __FUNCTION__, iter == m_map.cbegin(), iter == m_map.cend()); safe::debug(buf); }
 #define DEBUG_ITER2	{ char buf[512]; sprintf(buf, "%s%d:%p:%d %s %d%d", safe::get_thread_spacing().c_str(), __LINE__, &iter, iter->first, __FUNCTION__, iter == map.cbegin(), iter == map.cend()); safe::debug(buf); }
 #define DEBUG_RITER	{ char buf[512]; sprintf(buf, "%s%d:%p:%d %s %d%d", safe::get_thread_spacing().c_str(), __LINE__, &iter, iter->first, __FUNCTION__, iter == m_map.crbegin(), iter == m_map.crend()); safe::debug(buf); }
 #define DEBUG_RITER2	{ char buf[512]; sprintf(buf, "%s%d:%p:%d %s %d%d", safe::get_thread_spacing().c_str(), __LINE__, &iter, iter->first, __FUNCTION__, iter == map.crbegin(), iter == map.crend()); safe::debug(buf); }
#else
 #define DEBUG_SIMPLE
 #define DEBUG_THIS
 #define DEBUG_FIRST
 #define DEBUG_FIRST_ITER
 #define DEBUG_REFCOUNT
 #define DEBUG_ITER
 #define DEBUG_ITER2
 #define DEBUG_RITER
 #define DEBUG_RITER2
#endif

#ifdef SEGFAULT_ON_ASSERT
 #define ASSERT(x) { if (!(x)) ((int*)(NULL))[0] = 0; }
#else
 #ifdef DEBUG_IMMEDIATELY
  #define ASSERT(x) assert(x)
 #else
  #ifdef DEBUG
   #define ASSERT(x) debug_assert((x) ? true : false); assert(x);
  #else
   #define ASSERT(x) assert(x)
  #endif
 #endif
#endif

#define FN(x)	[this](const T& last){ x(last); }
#define DEBUG_FIRST_SIMPLE DEBUG_FIRST

// F U N C T I O N S //////////////////////////////////////////////////////////

#ifdef DEBUG
std::map<std::thread::id, std::string> thread_spacings;
std::mutex thread_spacing_mutex;

std::string get_thread_spacing()
{
  std::string ret;
  const std::thread::id id = std::this_thread::get_id();
  std::lock_guard<std::mutex> g(thread_spacing_mutex);
  auto spacing = thread_spacings.find(id);
  if (spacing != thread_spacings.end())
    ret = spacing->second;
  else
  {
    for (size_t i = 0; i < thread_spacings.size() * 16; ++i)
      ret += " ";
    thread_spacings.insert(std::make_pair(id, ret));
  }

  return ret;
}
#endif

#ifndef DEBUG_IMMEDIATELY
 #ifdef DEBUG
std::deque<std::string> debug_buf;
std::mutex debug_mutex;

void debug(const std::string& str)
{
  std::lock_guard<std::mutex> g(debug_mutex);
  if (debug_buf.size() > MAX_DEBUG)
    debug_buf.pop_front();
  debug_buf.push_back(str);
}

void debug_assert(const bool b)
{
  std::lock_guard<std::mutex> g(debug_mutex);
  if (!b)
  {
    debug_buf.begin();
    for (auto& line : debug_buf)
      std::cout << line << std::endl;
  }
}
 #endif
#else
 #ifdef DEBUG
void debug(const std::string& str)
{
  std::cout << str << std::endl;
}
 #endif
#endif

// C L A S S E S //////////////////////////////////////////////////////////////

#ifdef DEBUG
class WrappedMutex : public std::mutex
{
public: 
  void lock()
  {
    DEBUG_SIMPLE;
    std::mutex::lock();
  };
  
  void unlock()
  {
    DEBUG_SIMPLE;
    std::mutex::unlock();
  };
};

class WrappedGuard : public std::lock_guard<WrappedMutex>
{
public:
  WrappedGuard(WrappedMutex& m) : std::lock_guard<WrappedMutex>(m)
  {
    DEBUG_SIMPLE;
  };

  ~WrappedGuard()
  {
    DEBUG_SIMPLE;
  };
};
#endif

template <class T>
class mapped : public T	// Helper class for map - adds in the reference counter and erasure flag
{
public:

  template <class U>
  mapped(U rhs) :
    T(rhs),
    _reference_count(0),
    _erase_when_unused(false)
    {
      DEBUG_THIS;
    }

  mapped() :
    _reference_count(0),
    _erase_when_unused(false)
    {
      DEBUG_THIS;
    };

  mapped(mapped<T>&& rhs) :	// Don't steal reference counts - each object has to be unique, even if copied.
    T(std::move(rhs)),
    _reference_count(0),
    _erase_when_unused(false)
    {
      DEBUG_THIS;
    };

  mapped(T&& rhs) : 
    T(std::move(rhs)),
    _reference_count(0),
    _erase_when_unused(false)
    {
      DEBUG_THIS;
    };
 
  mapped(const T& rhs) : 
    T(rhs),
    _reference_count(0),
    _erase_when_unused(false)
    {
      DEBUG_THIS;
    };
  
  mapped(const mapped<T>& rhs) : 
    T(rhs),
    _reference_count(0),
    _erase_when_unused(false)
    {
      DEBUG_THIS;
    };
  
  template <class U>
  mapped<T>& operator=(U rhs)
  {
    DEBUG_THIS;
    T::operator=(rhs);
    return *this;
  };

  mapped<T>& operator=(T&& rhs)
  {
    DEBUG_THIS;
    T::operator=(std::move(rhs));
    return *this;
  };

  mapped<T>& operator=(mapped<T>&& rhs)
  {
    DEBUG_THIS;
    ASSERT(!_reference_count);
    _reference_count = rhs._reference_count;
    _erase_when_unused = rhs._erase_when_unused;
    T::operator=(std::move(rhs));
    return *this;
  };

  mapped<T>& operator=(const T& rhs)
  {
    DEBUG_THIS;
    T::operator=(rhs);
    return *this;
  };

  mapped<T>& operator=(const mapped<T>& rhs)
  {
    DEBUG_THIS;
    ASSERT(!_reference_count);
    _reference_count = rhs._reference_count;
    _erase_when_unused = rhs._erase_when_unused;
    T::operator=(rhs);
    return *this;
  };

  std::atomic<int> _reference_count;
  bool _erase_when_unused = false;
};

enum DestructorSafetyType
{
  NoDestructorChecks = 0,
  SharedPointer = 1,
};

enum IterationType
{
  OnlyForward = 0,
  ForwardThenBackward = 1,
  ForwardSameThenBackward = 2,
  EvenErased = 3
};

template <class key_type, class mapped_type,
          bool circular = false,
          IterationType iteration = OnlyForward,
          DestructorSafetyType destructor = SharedPointer,
          typename Compare = std::less<key_type>,
          typename Allocator = typename std::conditional<std::is_class<mapped_type>{}, 
                                                         std::allocator<std::pair<const key_type, mapped<mapped_type>>>,
                                                         std::allocator<std::pair<const key_type, mapped<number::weak<mapped_type>>>>
                                                        >::type >
class map 
{
public:

  typedef typename std::conditional<std::is_class<mapped_type>::value, 
                                    mapped<mapped_type>,
                                    mapped<number::weak<mapped_type>>
                                    >::type safe_mapped_type;

  typedef std::map<key_type, safe_mapped_type> basetype;

#ifdef DEBUG
  typedef WrappedMutex					mutex_type;
  typedef WrappedGuard					guard_type;
#else
  typedef std::mutex					mutex_type;
  typedef std::lock_guard<mutex_type>			guard_type;
#endif

  #define GUARD			guard_type g1(const_cast<mutex_type&>(*m_lock));
  #define GUARD_RHS(x)		guard_type g2(const_cast<mutex_type&>(*x.m_lock));
//  #define GUARD_COUNT		guard_type g3(const_cast<mutex_type&>(*m_lock));	// Switch to these if you experience problems
  #define GUARD_COUNT
//  #define GUARD_SIZE		guard_type g4(const_cast<mutex_type&>(*m_lock));
  #define GUARD_SIZE

  typedef typename basetype::size_type			size_type;
  typedef std::pair<key_type, mapped_type>		value_type;
  typedef typename basetype::value_type			safe_value_type;
  typedef typename basetype::iterator			base_iterator;
  typedef typename basetype::const_iterator		base_const_iterator;
  typedef typename basetype::iterator			base_reverse_iterator;
  typedef typename basetype::const_iterator		base_const_reverse_iterator;
  typedef typename basetype::key_compare		key_compare;
  typedef typename basetype::value_compare		value_compare;

  typedef typename std::conditional<destructor == SharedPointer, 
                                    std::shared_ptr<std::map<key_type, safe_mapped_type, Compare, Allocator>>,
                                    std::unique_ptr<std::map<key_type, safe_mapped_type, Compare, Allocator>>>::type map_pointer_type;
  typedef typename std::conditional<destructor == SharedPointer, std::shared_ptr<mutex_type>, std::unique_ptr<mutex_type>>::type lock_pointer_type;
  typedef typename std::conditional<destructor == SharedPointer, 
                                    map_pointer_type,
                                    std::map<key_type, safe_mapped_type, Compare, Allocator>*>::type iter_map_pointer_type;
  typedef typename std::conditional<destructor == SharedPointer, lock_pointer_type, mutex_type*>::type iter_lock_pointer_type;
  
  template <class T> struct Dummy {};
  template <bool B> struct DummyB {};

  // Note: in this class we only perform assert tests on the map, not the lock. Why? Because
  // the lock always gets transfered along with the map, so there's no point to two tests.
  template <class T, bool Reversed>
  class iterator_base : public T
  {
  public:

    iterator_base() = delete;

    iterator_base(const map_pointer_type& _map, const lock_pointer_type& _lock) : 
      T(_map->end()),
      m_map(convert_map_pointer_type(_map)),
      m_lock(convert_lock_pointer_type(_lock))
    {
      DEBUG_FIRST_SIMPLE;
    };
    iterator_base(T&& _iter, const map_pointer_type& _map, const lock_pointer_type& _lock) :
      T(std::move(_iter)),
      m_map(convert_map_pointer_type(_map)),
      m_lock(convert_lock_pointer_type(_lock))
    {
      ASSERT(m_map);
      ASSERT(!m_lock->try_lock());	// Nýbreytt
      DEBUG_FIRST_ITER;
      while ((*this != map_real_end()) && ((*this)->second._erase_when_unused))
        T::operator++();
      reference();
    };
    iterator_base(iterator_base&& _iter) :
      T(std::move(_iter)),
      m_map(_iter.m_map),
      m_lock(_iter.m_lock)
    { 
      ASSERT(m_map);
      DEBUG_FIRST_ITER;
      _iter.m_map = NULL; 	// Gera það ógildt
    }
    template<class U, bool R>
    iterator_base(iterator_base<U, R>&& _iter) :
      T(std::move(_iter)),
      m_map(_iter.m_map),
      m_lock(_iter.m_lock)
    { 
      ASSERT(m_map);
      DEBUG_FIRST_ITER;
      _iter.m_map = NULL; 	// Gera það ógildt
    }
    iterator_base(const T& _iter, const map_pointer_type& _map, const lock_pointer_type& _lock) :
      T(_iter),
      m_map(convert_map_pointer_type(_map)),
      m_lock(convert_lock_pointer_type(_lock))
    {
      ASSERT(m_map);
      ASSERT(!m_lock->try_lock());	// Nýbreytt
      DEBUG_FIRST_ITER;
      while ((*this != map_real_end()) && ((*this)->second._erase_when_unused))
        T::operator++();
      reference();
    };
    iterator_base(const iterator_base& _iter) :
      T(_iter),
      m_map(_iter.m_map),
      m_lock(_iter.m_lock)
    { 
      DEBUG_FIRST_ITER;
      ASSERT(m_map);
      reference();
    }
    template<class U, bool R>
    iterator_base(const iterator_base<U, R>& _iter) :
      T(_iter),
      m_map(_iter.m_map),
      m_lock(_iter.m_lock)
    { 
      DEBUG_FIRST_ITER;
      ASSERT(m_map);
      reference();
    }
    ~iterator_base()
    { 
      if (m_map)
      {
        DEBUG_FIRST_SIMPLE;
        dereference(); 
      }
      else
        DEBUG_THIS;
    };
    
    iterator_base<T, Reversed>& operator--() { DEBUG_FIRST_SIMPLE; return (Reversed ? do_plus() : do_minus()); };
    iterator_base<T, Reversed>& operator++() { DEBUG_FIRST_SIMPLE; return (Reversed ? do_minus() : do_plus()); };
    iterator_base<T, Reversed> operator--(int x) { DEBUG_FIRST_SIMPLE; return (Reversed ? do_plus(x) : do_minus(x)); };
    iterator_base<T, Reversed> operator++(int x) { DEBUG_FIRST_SIMPLE; return (Reversed ? do_minus(x) : do_plus(x)); };

    template<class U, bool R> iterator_base<T, Reversed>& operator=(iterator_base<U, R>&& rhs)
    {
      DEBUG_FIRST_SIMPLE;
      ASSERT(rhs.m_map);
      if (m_map)
      {
        m_map = rhs.m_map;
        m_lock = rhs.m_lock;
        guard_type guard(*m_lock);
        auto need_erase = delayed_dereference();
        T::operator=(std::move(rhs));
        reference();
        if (need_erase != map_real_end())
        {
          if (need_erase->first != (*this)->first)
            map_erase(need_erase);
        }
      }
      else
      {
        m_map = rhs.m_map;
        m_lock = rhs.m_lock;
        T::operator=(std::move(rhs));
        guard_type guard(*m_lock);
        reference();
      }
      return *this;
    };
    
    template<class U, bool R> iterator_base<T, Reversed>& operator=(const iterator_base<U, R>& rhs)
    {
      DEBUG_FIRST_SIMPLE;
      ASSERT(rhs.m_map);
      if (m_map)
      {
        m_map = rhs.m_map;
        m_lock = rhs.m_lock;
        guard_type guard(*m_lock);
        auto need_erase = delayed_dereference();
        T::operator=(rhs);
        reference();
        if (need_erase != map_real_end())
        {
          if (need_erase->first != (*this)->first)
            map_erase(need_erase);
        }
      }
      else
      {
        m_map = rhs.m_map;
        m_lock = rhs.m_lock;
        T::operator=(rhs);
        guard_type guard(*m_lock);
        reference();
      }
      return *this;
    };
    
    template<class U, bool R> iterator_base<T, Reversed>& operator=(iterator_base<U, R>& rhs)
    {
      DEBUG_FIRST_SIMPLE;
      ASSERT(rhs.m_map);
      if (m_map)
      {
        m_map = rhs.m_map;
        m_lock = rhs.m_lock;
        guard_type guard(*m_lock);
        auto need_erase = delayed_dereference();
        T::operator=(rhs);
        reference();
        if (need_erase != map_real_end())
        {
          if (need_erase->first != (*this)->first)
            map_erase(need_erase);
        }
      }
      else
      {
        m_map = rhs.m_map;
        m_lock = rhs.m_lock;
        T::operator=(rhs);
        guard_type guard(*m_lock);
        reference();
      }
      return *this;
    };
    
    template <class R> iterator_base<T, Reversed>& operator=(R rhs)
    {
      ASSERT(m_map);
      guard_type guard(*m_lock);
      DEBUG_FIRST;
      auto need_erase = delayed_dereference();
      T::operator=(rhs);
      while ((*this != map_real_end()) && ((*this)->second._erase_when_unused))
        T::operator++();
      reference();
      if (need_erase != map_real_end())
      {
        if (need_erase->first != (*this)->first)
          map_erase(need_erase);
      }
      return *this;
    };

    iterator_base<T, Reversed>& do_minus()
    {
      DEBUG_FIRST_SIMPLE;
      if (circular)
      {
        if (iteration != EvenErased)
          do_increment_or_decrement(FN(decrement_active_circular_core));
        else
          do_increment_or_decrement(FN(decrement_even_erased_circular_core));
      }
      else
      {
        if (iteration == OnlyForward)
          do_increment_or_decrement(FN(decrement_forward_core));
        else if ((iteration == ForwardThenBackward) || (iteration == ForwardSameThenBackward))
          do_increment_or_decrement(FN(decrement_forward_then_backward_core));
        else
          do_increment_or_decrement(FN(decrement_even_erased_linear_core));
      }
      return *this;
    };

    iterator_base<T, Reversed>& do_plus()
    {
      DEBUG_FIRST_SIMPLE;
      if (circular)
      {
        if (iteration != EvenErased)
          do_increment_or_decrement(FN(increment_active_circular_core));
        else
          do_increment_or_decrement(FN(increment_even_erased_circular_core));
      }
      else
      {
        if (iteration == OnlyForward)
          do_increment_or_decrement(FN(increment_forward_core));
        else if ((iteration == ForwardThenBackward) || (iteration == ForwardSameThenBackward))
          do_increment_or_decrement(FN(increment_forward_then_backward_core));
        else
          do_increment_or_decrement(FN(increment_even_erased_linear_core));
      }
      return *this;
    };

    iterator_base<T, Reversed> do_minus(int x)
    {
      DEBUG_FIRST_SIMPLE;
      if (circular)
      {
        if (iteration != EvenErased)
          return do_increment_or_decrement_return(FN(decrement_active_circular_core));
        else
          return do_increment_or_decrement_return(FN(decrement_even_erased_circular_core));
      }
      else
      {
        if (iteration == OnlyForward)
          return do_increment_or_decrement_return(FN(decrement_forward_core));
        else if ((iteration == ForwardThenBackward) || (iteration == ForwardSameThenBackward))
          return do_increment_or_decrement_return(FN(decrement_forward_then_backward_core));
        else
          return do_increment_or_decrement_return(FN(decrement_even_erased_linear_core));
      }
    };
    
    

    iterator_base<T, Reversed> do_plus(int x)
    {
      DEBUG_FIRST_SIMPLE;
      if (circular)
      {
        if (iteration != EvenErased)
          return do_increment_or_decrement_return(FN(increment_active_circular_core));
        else
          return do_increment_or_decrement_return(FN(increment_even_erased_circular_core));
      }
      else
      {
        if (iteration == OnlyForward)
          return do_increment_or_decrement_return(FN(increment_forward_core));
        else if ((iteration == ForwardThenBackward) || (iteration == ForwardSameThenBackward))
          return do_increment_or_decrement_return(FN(increment_forward_then_backward_core));
        else
          return do_increment_or_decrement_return(FN(increment_even_erased_linear_core));
      }
    };
    
    iterator_base<T, Reversed>& decrement_active_circular()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(decrement_active_circular_core)); return *this; };
    
    iterator_base<T, Reversed>& increment_active_circular()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(decrement_active_circular_core)); return *this; };

    iterator_base<T, Reversed>& decrement_forward()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(decrement_forward_core)); return *this; };

    iterator_base<T, Reversed>& increment_forward()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(increment_forward_core)); return *this; };
    
    iterator_base<T, Reversed>& decrement_forward_then_backward()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(increment_forward_then_backward_core)); return *this; };

    iterator_base<T, Reversed>& increment_forward_then_backward()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(increment_forward_then_backward_core)); return *this; };

    iterator_base<T, Reversed>& decrement_even_erased_linear()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(decrement_even_erased_linear_core)); return *this; };

    iterator_base<T, Reversed>& increment_even_erased_linear()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(increment_even_erased_linear_core)); return *this; };

    iterator_base<T, Reversed>& decrement_even_erased_circular()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(decrement_even_erased_circular_core)); return *this; };

    iterator_base<T, Reversed>& increment_even_erased_circular()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement(FN(increment_even_erased_circular_core)); return *this; };

    iterator_base<T, Reversed>& decrement_active_circular(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(decrement_active_circular_core), m_map, m_lock); };
    
    iterator_base<T, Reversed>& increment_active_circular(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(decrement_active_circular_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_forward(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(decrement_forward_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_forward(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(increment_forward_core), m_map, m_lock); };
    
    iterator_base<T, Reversed>& decrement_forward_then_backward(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(increment_forward_then_backward_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_forward_then_backward(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(increment_forward_then_backward_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_even_erased_linear(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(decrement_even_erased_linear_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_even_erased_linear(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(increment_even_erased_linear_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_even_erased_circular(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(decrement_even_erased_circular_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_even_erased_circular(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_return(FN(increment_even_erased_circular_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_active_circular_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(decrement_active_circular_core)); return *this; };
    
    iterator_base<T, Reversed>& increment_active_circular_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(decrement_active_circular_core)); return *this; };

    iterator_base<T, Reversed>& decrement_forward_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(decrement_forward_core)); return *this; };

    iterator_base<T, Reversed>& increment_forward_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(increment_forward_core)); return *this; };
    
    iterator_base<T, Reversed>& decrement_forward_then_backward_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(increment_forward_then_backward_core)); return *this; };

    iterator_base<T, Reversed>& increment_forward_then_backward_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(increment_forward_then_backward_core)); return *this; };

    iterator_base<T, Reversed>& decrement_even_erased_linear_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(decrement_even_erased_linear_core)); return *this; };

    iterator_base<T, Reversed>& increment_even_erased_linear_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(increment_even_erased_linear_core)); return *this; };

    iterator_base<T, Reversed>& decrement_even_erased_circular_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(decrement_even_erased_circular_core)); return *this; };

    iterator_base<T, Reversed>& increment_even_erased_circular_prelocked()
      { DEBUG_FIRST_SIMPLE; do_increment_or_decrement_prelocked(FN(increment_even_erased_circular_core)); return *this; };

    iterator_base<T, Reversed>& decrement_active_circular_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(decrement_active_circular_core), m_map, m_lock); };
    
    iterator_base<T, Reversed>& increment_active_circular_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(decrement_active_circular_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_forward_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(decrement_forward_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_forward_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(increment_forward_core), m_map, m_lock); };
    
    iterator_base<T, Reversed>& decrement_forward_then_backward_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(increment_forward_then_backward_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_forward_then_backward_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(increment_forward_then_backward_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_even_erased_linear_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(decrement_even_erased_linear_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_even_erased_linear_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(increment_even_erased_linear_core), m_map, m_lock); };

    iterator_base<T, Reversed>& decrement_even_erased_circular_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(decrement_even_erased_circular_core), m_map, m_lock); };

    iterator_base<T, Reversed>& increment_even_erased_circular_prelocked(int)
      { DEBUG_FIRST_SIMPLE; return do_increment_or_decrement_prelocked_return(FN(increment_even_erased_circular_core), m_map, m_lock); };

    iter_map_pointer_type m_map;
    iter_lock_pointer_type m_lock;

  protected:

    iter_map_pointer_type convert_map_pointer_type(const map_pointer_type& rhs) const
      { return convert_pointer_type<iter_map_pointer_type, map_pointer_type>(rhs); };
    iter_lock_pointer_type convert_lock_pointer_type(const lock_pointer_type& rhs) const
      { return convert_pointer_type<iter_lock_pointer_type, lock_pointer_type>(rhs); };

    template<class U, class V> U convert_pointer_type(const V& rhs) const
      { return fetch_pointer<U, V>(rhs, DummyB<std::is_pointer<U>::value>()); };

    template<class U, class V> U fetch_pointer(const V& rhs, const DummyB<false>) const { return rhs; };
    template<class U, class V> U fetch_pointer(const V& rhs, const DummyB<true> ) const { return rhs.get(); };
    
    void clear()
    {
      if (*this != map_real_end())
      {
        decrement_reference();
        if (!((*this)->second._reference_count) && ((*this)->second._erase_when_unused))
          map_erase(*this);
      }
      m_map = NULL;
    };

    void do_increment_or_decrement(std::function<void(const T&)> f)
    { 
      ASSERT(m_map);
      guard_type guard(*m_lock);
      do_increment_or_decrement_helper(f); 
    };

    iterator_base<T, Reversed> do_increment_or_decrement_return(std::function<void(const T&)> f)
    {
      ASSERT(m_map);
      guard_type guard(*m_lock);
      return iterator_base<T, Reversed>(do_increment_or_decrement_helper(f), m_map, m_lock); 
    };
    
    void do_increment_or_decrement_prelocked(std::function<void(const T&)> f)
    { 
      ASSERT(m_map);
      ASSERT(!m_lock->try_lock());
      do_increment_or_decrement_helper(f); 
    };

    iterator_base<T, Reversed> do_increment_or_decrement_prelocked_return(std::function<void(const T&)> f)
    {
      ASSERT(m_map);
      ASSERT(!m_lock->try_lock());
      return iterator_base<T, Reversed>(do_increment_or_decrement_helper(f), m_map, m_lock); 
    };
    
    T do_increment_or_decrement_helper(std::function<void(const T&)> f)
    {
      DEBUG_FIRST;
      if (!m_map->size())
      {
        T::operator=(map_real_end());
        return *this;
      }
      auto need_erase = delayed_dereference();
      const T last = *this;

      f(last);

      reference();

      if (need_erase != *this)
      {
        if (need_erase != map_real_end())
        {
          map_erase(need_erase);
          return *this;
        }
      }
     
      return last;
    };

    void decrement_forward_then_backward_core(const T& last)
    {
      DEBUG_FIRST;
      decrement_forward_core(last);
      if (*this == (Reversed ? map_real_end() : map_real_begin()))
      {
        if (Reversed || (*this)->second._erase_when_unused)
        {
          T::operator=(last);
          if ((iteration == ForwardSameThenBackward) && (!(*this)->second._erase_when_unused))
            return;
          increment_forward_core(last);
        }
      }
    };
    
    void increment_forward_then_backward_core(const T& last)
    {
      DEBUG_FIRST;
      increment_forward_core(last);
      auto endpoint = map_real_end();
      if (Reversed)
        --endpoint;
      if (*this == endpoint)
      {
        if ((!Reversed) || (*this)->second._erase_when_unused)
        {
          T::operator=(last);
          if ((iteration == ForwardSameThenBackward) && (!(*this)->second._erase_when_unused))
            return;
          decrement_forward_core(last);
        }
      }
    };
    
    void decrement_even_erased_linear_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_begin())
      {
        if (Reversed)
          T::operator=(map_real_end());
        return;
      }
      T::operator--();
    };

    void increment_even_erased_linear_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_end())
      {
        if (Reversed)
          T::operator=(map_real_begin());
        return;
      }        
      T::operator++();
    };

    void decrement_even_erased_circular_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_begin())
        T::operator=(map_real_end());
      T::operator--();
    };

    void increment_even_erased_circular_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_end())
        T::operator=(map_real_begin());
      else
      {
        T::operator++();
        if (*this == map_real_end())
          T::operator=(map_real_begin());
      }
    };

    void decrement_forward_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_begin())
      {
        if (Reversed)
          T::operator=(map_real_end());
        return;
      }
      do 
        { DEBUG_FIRST; T::operator--(); }
        while (((*this)->second._erase_when_unused) && (*this != map_real_begin()));
      if (Reversed)
      {
        if ((*this)->second._erase_when_unused)
          T::operator=(map_real_end());
      }
    };

    void increment_forward_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_end())
      {
        if (Reversed)
          T::operator=(map_real_begin());
        else
          return;
      }
      else
        T::operator++();
      while ((*this != map_real_end()) && ((*this)->second._erase_when_unused))
        { DEBUG_FIRST; T::operator++(); } 
      if ((Reversed) && (*this == map_real_end()))
        T::operator--();
    };

    void decrement_active_circular_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_begin())
      {
        DEBUG_FIRST;
        T::operator=(map_real_end());
        T::operator--();
      }
      else
      {
        if (*this == map_real_end())
          T::operator--();
        while (((*this)->second._erase_when_unused) && (*this != map_real_begin()))
          { DEBUG_FIRST; T::operator--(); }
        if ((*this)->second._erase_when_unused)
        {
          DEBUG_FIRST;
          T::operator=(map_real_end());
          if (*this == last)
            return;
          T::operator--();
        }
      }
      if ((*this)->second._erase_when_unused)
      {
        while ((*this != last) && ((*this)->second._erase_when_unused))
          { DEBUG_FIRST; T::operator--(); }
      }
      if ((*this)->second._erase_when_unused)
        T::operator=(map_real_end());
    };

    void increment_active_circular_core(const T& last)
    {
      DEBUG_FIRST;
      if (*this == map_real_end())
        T::operator=(map_real_begin());
      else
        T::operator++();
      while ((*this != map_real_end()) && ((*this)->second._erase_when_unused))
        { DEBUG_FIRST; T::operator++(); } 
      if ((*this == map_real_end()) && (*this != last))
      {
        T::operator=(map_real_begin());
        while ((*this != last) && ((*this)->second._erase_when_unused))
          { DEBUG_FIRST; T::operator++(); } 
        if ((*this)->second._erase_when_unused)
          T::operator=(map_real_end());
      }
    };

    void reference()
    {
      DEBUG_FIRST_SIMPLE;
      ASSERT(m_map);
      if (*this != map_real_end())
        increment_reference();
    };
       
    void dereference()
    {
      DEBUG_FIRST_SIMPLE;
      ASSERT(m_map);
      if (*this != map_real_end())
      {
        guard_type guard(*m_lock);
        decrement_reference();
        if (!((*this)->second._reference_count) && ((*this)->second._erase_when_unused))
          map_erase(*this);
      }   
    }; 
    T delayed_dereference()
    {
      ASSERT(m_map);
      ASSERT(!m_lock->try_lock());
      DEBUG_FIRST;
      if (*this != map_real_end())
      {
        decrement_reference();
        if (!((*this)->second._reference_count) && ((*this)->second._erase_when_unused))
          return static_cast<T>(*this);
      }
      return map_real_end();    
    }; 

    void map_erase(T i) { ASSERT(m_map); DEBUG_FIRST; m_map->erase(i); };    
    T map_real_begin() { ASSERT(m_map); return map_begin(Dummy<T>()); };
    T map_real_end() { ASSERT(m_map); return map_end(Dummy<T>()); };
    T map_begin() { ASSERT(m_map); return (Reversed ? map_rbegin(Dummy<T>()) : map_begin(Dummy<T>())); };
    T map_end() { ASSERT(m_map); return (Reversed ? map_rend(Dummy<T>()) : map_end(Dummy<T>())); };
    T map_safe_begin() { ASSERT(m_map); return (Reversed ? map_safe_rbegin(Dummy<T>()) : map_begin(Dummy<T>())); };
    T map_safe_end() { ASSERT(m_map); return (Reversed ? map_safe_rend(Dummy<T>()) : map_end(Dummy<T>())); };
    base_iterator map_begin(Dummy<base_iterator>) { return m_map->begin(); };
    base_iterator map_end(Dummy<base_iterator>) { return m_map->end(); };
    base_const_iterator map_begin(Dummy<base_const_iterator>) { return m_map->cbegin(); };
    base_const_iterator map_end(Dummy<base_const_iterator>) { return m_map->cend(); };
    base_iterator map_safe_rbegin(Dummy<base_iterator>) { return m_map->end(); };
    base_iterator map_safe_rend(Dummy<base_iterator>) { auto iter = m_map->end(); if (m_map->size()) --iter; return iter; };
    base_const_iterator map_safe_rbegin(Dummy<base_const_iterator>) { return m_map->cend(); };
    base_const_iterator map_safe_rend(Dummy<base_const_iterator>) { auto iter = m_map->cend(); if (m_map->size()) --iter; return iter; };
    base_iterator map_rbegin(Dummy<base_iterator>) { return m_map->end(); };
    base_iterator map_rend(Dummy<base_iterator>) { auto iter = m_map->end(); --iter; return iter; };
    base_const_iterator map_rbegin(Dummy<base_const_iterator>) { return m_map->cend(); };
    base_const_iterator map_rend(Dummy<base_const_iterator>) { auto iter = m_map->cend(); --iter; return iter; };

    void decrement_reference()
    {
      DEBUG_REFCOUNT;
      --const_cast<std::atomic<int>&>((*this)->second._reference_count);
      ASSERT((*this)->second._reference_count >= 0);
    };

    void increment_reference()
    {
      DEBUG_REFCOUNT;
      ++const_cast<std::atomic<int>&>((*this)->second._reference_count);
    };

  };

  //////////////////////////////////////////
  // Main portion of "map" begins here.
  //////////////////////////////////////////
  
  typedef iterator_base<base_iterator, false>		iterator;
  typedef iterator_base<base_const_iterator, false>	const_iterator;
  typedef iterator_base<base_iterator, true>		reverse_iterator;
  typedef iterator_base<base_const_iterator, true>	const_reverse_iterator;

  explicit map(const Compare& comp = Compare(), const Allocator& alloc = Allocator()) : 
    m_map(new std::map<key_type, safe_mapped_type>(comp, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  explicit map(const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  map(const std::map<key_type, safe_mapped_type>& other) : 
    m_map(new std::map<key_type, safe_mapped_type>(other)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };
  
  template<class InputIterator> 
  map(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator()) :
    m_map(new std::map<key_type, safe_mapped_type>(first, last, comp, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  template<class InputIterator>
  map(InputIterator first, InputIterator last, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(first, last, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  map(const std::map<key_type, safe_mapped_type>& other, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(other, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  map(const std::map<key_type, safe_mapped_type>&& other) : 
    m_map(new std::map<key_type, safe_mapped_type>(other)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  map(const std::map<key_type, safe_mapped_type>&& other, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(other, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; };

  map(const safe::map<key_type, mapped_type>& other, const Compare& comp = Compare(), const Allocator& alloc = Allocator()) :
    m_map(new std::map<key_type, safe_mapped_type>(comp, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(other); };
  
  map(const safe::map<key_type, mapped_type>& other, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(other); };

  map(safe::map<key_type, mapped_type>&& other, const Compare& comp = Compare(), const Allocator& alloc = Allocator()) : 
    m_map(new std::map<key_type, safe_mapped_type>(comp, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(other); };

  map(safe::map<key_type, mapped_type>&& other, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(other); };

  map(const std::map<key_type, mapped_type>& other, const Compare& comp = Compare(), const Allocator& alloc = Allocator()) :
    m_map(new std::map<key_type, safe_mapped_type>(comp, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(other); };
  
  map(const std::map<key_type, mapped_type>& other, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(other); };

  map(std::initializer_list<value_type> init, const Compare& comp = Compare(), const Allocator& alloc = Allocator()) : 
    m_map(new std::map<key_type, safe_mapped_type>(comp, alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(init); };

  map(std::initializer_list<value_type> init, const Allocator& alloc) : 
    m_map(new std::map<key_type, safe_mapped_type>(alloc)),
    m_lock(new mutex_type())
    { DEBUG_SIMPLE; operator=(init); };

  ~map()
    { DEBUG_SIMPLE; };
 
  safe_mapped_type& at(const key_type& k)
    { DEBUG_SIMPLE; GUARD; return m_map->at(k); };
  const safe_mapped_type& at(const key_type& k) const
    { DEBUG_SIMPLE; GUARD; return m_map->at(k); };
  iterator begin() noexcept
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->begin(), m_map, m_lock); };
  const_iterator cbegin() const noexcept
    { DEBUG_SIMPLE; GUARD; return const_iterator(m_map->cbegin(), m_map, m_lock); };
  const_iterator cend() const noexcept
    { DEBUG_SIMPLE; GUARD; return const_iterator(m_map->cend(), m_map, m_lock); };
  size_type count(const key_type& k) const
    { DEBUG_SIMPLE; GUARD_COUNT; return m_map->count(k); };
  const_reverse_iterator crbegin() const noexcept
  {
    DEBUG_SIMPLE;
    GUARD;
    auto tmp = m_map->cend();
    if (m_map->size())
       --tmp;
    return const_reverse_iterator(std::move(tmp), m_map, m_lock);
  };
  const_reverse_iterator crend() const noexcept
    { DEBUG_SIMPLE; GUARD; return const_reverse_iterator(m_map->cend(), m_map, m_lock); };
  template <class... Args> std::pair<iterator, bool> emplace(Args&&... args)
    { DEBUG_SIMPLE; GUARD; auto ret = m_map->emplace(std::forward<Args>(args)...); return std::make_pair(iterator(ret.first, m_map, m_lock), ret.second); };
  template <class... Args> iterator emplace_hint(const_iterator position, Args&&... args)
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->emplace_hint(position, std::forward<Args>(args)...), m_map, m_lock); };
  bool empty() const noexcept { return m_map->empty(); };
  iterator end() noexcept
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->end(), m_map, m_lock); };
  const_iterator end() const noexcept
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->end(), m_map, m_lock); };
  std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
    { DEBUG_SIMPLE; GUARD; auto ret = m_map->equal_range(k); return std::make_pair(const_iterator(ret.first, m_map, m_lock), const_iterator(ret.second, m_map, m_lock)); };
  std::pair<iterator, iterator> equal_range(const key_type& k)
    { DEBUG_SIMPLE; GUARD; auto ret = m_map->equal_range(k); return std::make_pair(iterator(ret.first, m_map, m_lock), iterator(ret.second, m_map, m_lock)); };
  iterator find(const key_type& k)
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->find(k), m_map, m_lock); };
  const_iterator find(const key_type& k) const
    { DEBUG_SIMPLE; GUARD; return const_iterator(m_map->find(k), m_map, m_lock); };
  std::pair<iterator, bool> insert(const value_type& val)
    { DEBUG_SIMPLE; GUARD; auto ret = m_map->insert(safe_value(val)); return std::make_pair(iterator(ret.first, m_map, m_lock), ret.second); };
  std::pair<iterator, bool> insert(value_type&& val)
  {
    DEBUG_SIMPLE;
    GUARD;
    auto ret = m_map->insert(safe_value(val)); 
    return std::make_pair(iterator(ret.first, m_map, m_lock), ret.second);
  };
  template <class P> std::pair<iterator, bool> insert(P&& val)
    { DEBUG_SIMPLE; GUARD; auto ret = m_map->insert(safe_value(val)); return std::make_pair(iterator(ret.first, m_map, m_lock), ret.second); };
  iterator insert(const_iterator position, const value_type& val)
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->insert(position, safe_value(val)), m_map, m_lock); };
  template <class InputIterator> void insert(InputIterator first, InputIterator last)
  {
    DEBUG_SIMPLE;
    GUARD; 
    for (auto iter = first; iter != last; ++iter)
      m_map->insert(*iter);
  };
  template <class U, bool V> void insert(const iterator_base<U, V>& first, const iterator_base<U, V>& last)
  {
    DEBUG_SIMPLE;
    ASSERT(first.m_map == last.m_map); 
    GUARD; 
    for (auto iter = first; iter != last; ++iter)
      m_map->insert(*iter);
  };
  void insert(std::initializer_list<value_type> il)
  {
    DEBUG_SIMPLE;
    GUARD;
    for (auto & i : il)
      m_map->insert(i);
  };
  key_compare key_comp() const { return m_map->key_comp(); };
  iterator lower_bound(const key_type& k)
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->lower_bound(k), m_map, m_lock); };
  const_iterator lower_bound(const key_type& k) const
    { DEBUG_SIMPLE; GUARD; return const_iterator(m_map->lower_bound(k), m_map, m_lock); };
  size_type max_size() const noexcept { return m_map->max_size(); };
  map<key_type, mapped_type>& operator=(const map<key_type, mapped_type>& x)
  {
    DEBUG_SIMPLE;
    GUARD;
    GUARD_RHS(x);
    clear_prelocked();
    for (auto& i : *x.m_map)
    {
      auto second = i.second;
      if (!second._erase_when_unused)
      {
        second._reference_count = 0;
        m_map->emplace(i.first, second); 
      }
    }
    return *this;
  };
  map<key_type, mapped_type>& operator=(const std::map<key_type, mapped_type>& x)
  {
    DEBUG_SIMPLE;
    GUARD;
    clear_prelocked();
    for (auto& i : x)
      m_map->emplace(i.first, safe_mapped_type(i.second));
    return *this;
  };
  map<key_type, mapped_type>& operator=(map<key_type, mapped_type>&& x)
  {
    DEBUG_SIMPLE;
    GUARD; 
    GUARD_RHS(x); 
    clear_prelocked();	// Wish I could just call swap on the map and lock, but there's no way to make that work without a double dereference pointer setup, which would hurt performance.
    for (auto& i : *x.m_map)
    {
      auto second = i.second;
      if (!second._erase_when_unused)
      {
        second._reference_count = 0;
        m_map->emplace(i.first, second); 
      }
    }
    return *this;
  };
  map<key_type, mapped_type>& operator=(std::map<key_type, mapped_type>&& x)
  {
    DEBUG_SIMPLE;
    GUARD;
    clear_prelocked();
    for (auto i = x.m_map->begin(); i != x.m_map->end(); ++i)
      m_map->insert(*i); 
    return *this;
  };
  map<key_type, mapped_type>& operator=(std::initializer_list<value_type> il)
  {
    DEBUG_SIMPLE;
    GUARD;
    clear_prelocked();
    for (auto& val : il)
      m_map->insert(safe_value(val)); 
    return *this;
  };
  safe_mapped_type& operator[](const key_type& k)
    { DEBUG_SIMPLE; GUARD; return m_map->operator[](k); };
  safe_mapped_type& operator[](key_type&& k)
    { DEBUG_SIMPLE; GUARD; return m_map->operator[](std::forward<key_type>(k)); };
  reverse_iterator rbegin() noexcept
  {
    DEBUG_SIMPLE;
    GUARD;
    auto tmp = m_map->end();
    if (m_map->size())
       --tmp;
    return reverse_iterator(std::move(tmp), m_map, m_lock);
  };
  const_reverse_iterator rbegin() const noexcept
  {
    DEBUG_SIMPLE;
    GUARD;
    auto tmp = m_map->end();
    if (m_map->size())
       --tmp;
    return const_reverse_iterator(std::move(tmp), m_map, m_lock);
  };
  reverse_iterator rend() noexcept
    { DEBUG_SIMPLE; GUARD; return reverse_iterator(m_map->end(), m_map, m_lock); };
  const_reverse_iterator rend() const noexcept
    { DEBUG_SIMPLE; GUARD; return const_reverse_iterator(m_map->rend(), m_map, m_lock); };
  size_type size() const noexcept
    { DEBUG_SIMPLE; GUARD_SIZE; return m_map->size(); };
  void swap(map<key_type, mapped_type>& x)
  {
    DEBUG_SIMPLE;
    GUARD;
    GUARD_RHS(x);
    std::map<key_type, safe_mapped_type> tmp;
    for (auto iter = m_map->begin(); iter != m_map->end(); )
    {
      if (!iter->second._erase_when_unused)
      {
        auto second = iter->second;
        second._reference_count = 0;
        tmp.emplace(iter->first, second);
        if (!iter->second._reference_count)
        {
          iter = m_map->erase(iter);
          continue;
        }
        else
          iter->second._erase_when_unused = true;
      }
      ++iter;
    }
    for (auto& i : *x.m_map)
    {
      m_map->insert(i); 
      i.second._reference_count = 0;
    }
    for (auto& i : *x.m_map)
    {
      if (!i.second._erase_when_unused)
      {
        auto second = i.second;
        second._reference_count = 0;
        m_map->emplace(i.first, second); 
      }
    }
    for (auto& i : tmp)
      m_map->insert(i); 
  };
  void swap(std::map<key_type, mapped_type>& x)
  {
    DEBUG_SIMPLE;
    GUARD;
    std::map<key_type, mapped_type> tmp;
    for (auto iter = m_map->begin(); iter != m_map->end(); )
    {
      if (!iter->second._erase_when_unused)
      {
        tmp.emplace(iter->first, iter->second);
        if (!iter->second._reference_count)
        {
          iter = m_map->erase(iter);
          continue;
        }
        else
          iter->second._erase_when_unused = true;
      }
      ++iter;
    }
    for (auto& i : x)
      m_map->emplace(i.first, i.second); 
    tmp.swap(x);
  };
  iterator upper_bound(const key_type& k)
    { DEBUG_SIMPLE; GUARD; return iterator(m_map->upper_bound(k), m_map, m_lock); };
  const_iterator upper_bound(const key_type& k) const
    { DEBUG_SIMPLE; GUARD; return const_iterator(m_map->upper_bound(k), m_map, m_lock); };
  value_compare value_comp() const { return m_map->value_comp(); };

  void clear() noexcept
  {
    GUARD;
    clear_prelocked();
  };
  
  void clear_fast() noexcept
  {
    DEBUG_SIMPLE;
    GUARD;
    for (auto iter = m_map->begin(); iter != m_map->end(); ++iter)
      iter->second._erase_when_unused = true;
  };
  
  size_type erase(const key_type& k)
  {
    DEBUG_SIMPLE;
    GUARD;
    auto iter = m_map->find(k);
    if (iter != m_map->end())
    {
      erase_prelocked(iter);
      return 1;
    }
    else
      return 0;
  };

  size_type erase_fast(const key_type& k)
  {
    DEBUG_SIMPLE;
    GUARD;
    auto iter = m_map->find(k);
    if (iter != m_map->end())
    {
      iter->second._erase_when_unused = true;
      return 1;
    }
    else
      return 0;
  };

  iterator_base<base_iterator, false> erase(iterator_base<base_iterator, false> position) { DEBUG_SIMPLE; GUARD; return erase_prelocked(position); };
  template <class U, bool V> iterator_base<U, V> erase(iterator_base<U, V>& position) { DEBUG_SIMPLE; GUARD; return erase_prelocked(position); };

  void erase_fast(iterator_base<base_iterator, false> position) { DEBUG_SIMPLE; position->_erase_when_unused = true; };
  template <class U, bool V> void erase_fast(iterator_base<U, V>& position) { DEBUG_SIMPLE; position->_erase_when_unused = true; };

  template <class U, bool V> const iterator_base<U, V> erase(const iterator_base<U, V>& first, const iterator_base<U, V>& last)
  {
    DEBUG_SIMPLE;
    GUARD;
    ASSERT(first.m_map == last.m_map);
    auto iter = (V ? static_cast<base_iterator>(last) : static_cast<base_iterator>(first));
    auto end = (V ? static_cast<base_iterator>(first) : static_cast<base_iterator>(last));
    for (; iter != end; )
    {
      ASSERT(iter != m_map->end());
      if (!iter->second._erase_when_unused)
      {
        if (!iter->second._reference_count)
        {
          iter = m_map->erase(iter);
          continue;
        }
        else
          iter->second._erase_when_unused = true;
      }
      ++iter;
    }
    return iterator_base<U, V>(iter, m_map, m_lock);
  };

  template <class T> T erase_fast(const T& first, const T& last)
  {
    DEBUG_SIMPLE;
    GUARD;
    ASSERT(first.m_map == last.m_map);
    auto iter = first;
    for (; iter != last; ++iter)
      iter->second._erase_when_unused = true;
    return iter;
  };

  void cleanup() noexcept
  {
    DEBUG_SIMPLE;
    GUARD;
    for (auto iter = m_map->begin(); iter != m_map->end(); )
    {
      if ((iter->second._erase_when_unused) && (!iter->second._reference_count))
        iter = m_map->erase(iter);
      else
        ++iter;
    }
  };
  
  map_pointer_type m_map;
  lock_pointer_type m_lock;
  
protected:

  void clear_prelocked() noexcept
  {
    DEBUG_SIMPLE;
    for (auto iter = m_map->begin(); iter != m_map->end(); )
    {
      if (!iter->second._reference_count)
        iter = m_map->erase(iter);
      else
      {
        iter->second._erase_when_unused = true;
        ++iter;
      }
    }
  };
  
  template <class U, bool V> iterator_base<U, V>& erase_prelocked(iterator_base<U, V>& iter)
  {
    if (!iter->second._reference_count)
      iter = m_map->erase(iter);
    else
      iter->second._erase_when_unused = true;
    return iter;
  };

  template <class U> U& erase_prelocked(U& iter)
  {
    if (!iter->second._reference_count)
      iter = m_map->erase(iter);
    else
      iter->second._erase_when_unused = true;
    return iter;
  };

  iterator deconst_iter(const const_iterator& i)
    { return m_map->erase(i, i); }	// Doesn't actually erase anything
  
  safe_value_type safe_value(const value_type& val) const
    { return safe_value_type(val.first, safe_mapped_type(val.second)); };
};


}; // End namespace "safe"

///////////////////////////////////////////////////////////////////////////////

#endif	// __SAFEMAP_H__
