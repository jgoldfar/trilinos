#ifndef KOKKOS_TEST_UNORDERED_MAP_HPP
#define KOKKOS_TEST_UNORDERED_MAP_HPP

#include <gtest/gtest.h>
#include <iostream>
#include <impl/Kokkos_Timer.hpp>


namespace Test {

namespace Impl {

  template <typename MapType, bool Near>
  struct TestInsert
  {
    typedef TestInsert<MapType,Near> self_type;

    typedef MapType map_type;
    typedef typename MapType::device_type device_type;
    typedef uint32_t value_type;

    map_type m_map;
    uint32_t m_num_insert;
    uint32_t m_num_duplicates;

    TestInsert(map_type map, uint32_t num_inserts, uint32_t num_duplicates)
      : m_map(map)
      , m_num_insert(num_inserts)
      , m_num_duplicates(num_duplicates)
    {}

    value_type apply()
    {
      value_type num_failures = 0;
      Kokkos::parallel_reduce(m_num_insert, *this, num_failures);
      return num_failures;
    }


    KOKKOS_INLINE_FUNCTION
    void init(value_type & value) const { value = 0; }

    KOKKOS_INLINE_FUNCTION
    void join(value_type volatile & dst, value_type const volatile & src) const { dst += src; }

    KOKKOS_INLINE_FUNCTION
    void operator()(typename device_type::size_type i, value_type & num_failures) const
    {
      if (Near) {
        if ( m_map.insert(i/m_num_duplicates, uint32_t(i)).failed() ) ++num_failures;
      }
      else {
        if ( m_map.insert(i%(m_num_insert/m_num_duplicates), uint32_t(i)).failed() ) ++num_failures;
      }
    }
  };

  template <typename MapType, bool Near>
  struct TestErase
  {
    typedef TestErase<MapType, Near> self_type;

    typedef MapType map_type;
    typedef typename MapType::device_type device_type;

    map_type m_map;
    uint32_t m_num_erase;
    uint32_t m_num_duplicates;

    TestErase(map_type map, uint32_t num_erases, uint32_t num_duplicates)
      : m_map(map)
      , m_num_erase(num_erases)
      , m_num_duplicates(num_duplicates)
    {
      Kokkos::parallel_for(num_erases, *this);
    }

    KOKKOS_INLINE_FUNCTION
    void operator()(typename device_type::size_type i) const
    {
      if (Near) {
        m_map.erase(i/m_num_duplicates);
      }
      else {
        m_map.erase(i%(m_num_erase/m_num_duplicates));
      }

    }
  };

  template <typename MapType>
  struct TestFind
  {
    typedef MapType map_type;
    typedef typename MapType::device_type device_type;
    typedef uint32_t value_type;

    map_type m_map;
    uint32_t m_num_insert;
    uint32_t m_num_duplicates;

    TestFind(map_type map, uint32_t num_inserts, uint32_t num_duplicates, value_type & errors)
      : m_map(map)
      , m_num_insert(num_inserts)
      , m_num_duplicates(num_duplicates)
    {
      Kokkos::parallel_reduce(num_inserts, *this, errors);
      device_type::fence();
    }

    KOKKOS_INLINE_FUNCTION
    static void init( value_type & dst)
    {
      dst = 0;
    }

    KOKKOS_INLINE_FUNCTION
    static void join( volatile value_type & dst, const volatile value_type & src)
    { dst += src; }

    KOKKOS_INLINE_FUNCTION
    void operator()(typename device_type::size_type i, value_type & errors) const
    {
      const uint32_t max_i = (m_num_insert + m_num_duplicates -1u) / m_num_duplicates;
      const bool expect_to_find_i = (i < max_i);

      const bool exists = m_map.exists(i);

      if (expect_to_find_i && !exists)  ++errors;
      if (!expect_to_find_i && exists)  ++errors;
    }
  };

} // namespace Impl




template <typename Device>
void test_insert_close(  uint32_t num_nodes
                       , uint32_t num_inserts
                       , uint32_t num_duplicates
                      )
{
  typedef Kokkos::UnorderedMap<uint32_t,uint32_t, Device> map_type;
  typedef Kokkos::UnorderedMap<const uint32_t,const uint32_t, Device> const_map_type;

  const uint32_t expected_inserts = (num_inserts + num_duplicates -1u) / num_duplicates;

  map_type map(num_nodes);
  Device::fence();

  Impl::TestInsert<map_type,true> test_insert_close(map, num_inserts, num_duplicates);
  test_insert_close.apply();
  Device::fence();

  const uint32_t map_size = map.size();

  ASSERT_FALSE( map.has_failed_inserts() );
  {
    EXPECT_EQ(map_size, expected_inserts);
    {
      uint32_t find_errors = 0;
      Impl::TestFind<const_map_type> test_find(map, num_inserts, num_duplicates, find_errors);
      EXPECT_EQ( find_errors, 0u);
    }

    map.begin_erase();
    Impl::TestErase<map_type,true> erase_close(map, num_inserts, num_duplicates);
    map.end_erase();
    EXPECT_EQ(map.size(), 0u);
  }
}

template <typename Device>
void test_insert_far(  uint32_t num_nodes
                       , uint32_t num_inserts
                       , uint32_t num_duplicates
                      )
{
  typedef Kokkos::UnorderedMap<uint32_t,uint32_t, Device> map_type;
  typedef Kokkos::UnorderedMap<const uint32_t,const uint32_t, Device> const_map_type;

  const uint32_t expected_inserts = (num_inserts + num_duplicates -1u) / num_duplicates;

  map_type map(num_nodes);
  Device::fence();

  Impl::TestInsert<map_type,false> test_insert_far(map, num_inserts, num_duplicates);
  test_insert_far.apply();
  Device::fence();

  const uint32_t map_size = map.size();

  ASSERT_FALSE( map.has_failed_inserts());
  {
    EXPECT_EQ(map_size, expected_inserts);

    {
      uint32_t find_errors = 0;
      Impl::TestFind<const_map_type> test_find(map, num_inserts, num_duplicates, find_errors);
      EXPECT_EQ( find_errors, 0u);
    }

    map.begin_erase();
    Impl::TestErase<map_type,false> erase_far(map, num_inserts, num_duplicates);
    map.end_erase();
    EXPECT_EQ(map.size(), 0u);
  }
}

template <typename Device>
void test_failed_insert( uint32_t num_nodes)
{
  typedef Kokkos::UnorderedMap<uint32_t,uint32_t, Device> map_type;

  map_type map(num_nodes);
  Impl::TestInsert<map_type,false> test_insert_far(map, 2u*num_nodes, 1u);
  test_insert_far.apply();
  Device::fence();

  EXPECT_TRUE( map.has_failed_inserts() );
}



template <typename Device>
void test_deep_copy( uint32_t num_nodes )
{
  typedef typename Device::host_mirror_device_type host_type ;

  typedef Kokkos::UnorderedMap<uint32_t,uint32_t, Device> map_type;
  typedef Kokkos::UnorderedMap<const uint32_t, uint32_t, Device> non_insertable_map_type;
  typedef Kokkos::UnorderedMap<const uint32_t, const uint32_t, Device> const_map_type;

  typedef Kokkos::UnorderedMap<uint32_t, uint32_t, host_type> host_map_type;
  typedef Kokkos::UnorderedMap<const uint32_t, const uint32_t, host_type> const_host_map_type;

  // mfh 14 Feb 2014: This function doesn't actually create instances
  // of non_insertable_map_type or const_host_map_type, but I'm
  // guessing that ensuring that the typedefs make sense at compile
  // time is important.  I preserve the typedefs without the warnings
  // by declaring an empty instance of each type, and marking it with
  // "(void)" to avoid a compiler warning for the unused variable.
  {
    non_insertable_map_type thing;
    (void) thing;
  }
  {
    const_host_map_type thing;
    (void) thing;
  }


  map_type map(num_nodes);
  Device::fence();

  {
    Impl::TestInsert<map_type,false> test_insert_far(map, num_nodes, 1);
    test_insert_far.apply();
    Device::fence();
    EXPECT_EQ( map.size(), num_nodes);
  }

  host_map_type hmap;
  Kokkos::deep_copy(hmap, map);

  map_type mmap;
  Kokkos::deep_copy(mmap, hmap);

  const_map_type cmap = mmap;

  EXPECT_EQ( cmap.size(), num_nodes);

  uint32_t find_errors = 0;
  Impl::TestFind<const_map_type> test_find(cmap, num_nodes/2u, 1, find_errors);
  EXPECT_EQ( find_errors, 0u);

}

} // namespace Test

#endif //KOKKOS_TEST_UNORDERED_MAP_HPP
