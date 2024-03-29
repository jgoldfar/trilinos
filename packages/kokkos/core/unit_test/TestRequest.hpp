/*
//@HEADER
// ************************************************************************
//
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <stdio.h>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <Kokkos_Parallel.hpp>

/*--------------------------------------------------------------------------*/

namespace Test {

template< typename ScalarType , class DeviceType >
class ReduceRequestFunctor
{
public:
  typedef DeviceType  device_type ;
  typedef typename device_type::size_type size_type ;

  struct value_type {
    ScalarType value[3] ;
  };

  const size_type nwork ;

  ReduceRequestFunctor( const size_type & arg_nwork ) : nwork( arg_nwork ) {}

  ReduceRequestFunctor( const ReduceRequestFunctor & rhs )
    : nwork( rhs.nwork ) {}

  KOKKOS_INLINE_FUNCTION
  void init( value_type & dst ) const
  {
    dst.value[0] = 0 ;
    dst.value[1] = 0 ;
    dst.value[2] = 0 ;
  }

  KOKKOS_INLINE_FUNCTION
  void join( volatile value_type & dst ,
             const volatile value_type & src ) const
  {
    dst.value[0] += src.value[0] ;
    dst.value[1] += src.value[1] ;
    dst.value[2] += src.value[2] ;
  }

  KOKKOS_INLINE_FUNCTION
  void operator()( device_type dev , value_type & dst ) const
  {
    const int thread_rank = dev.team_rank() + dev.team_size() * dev.league_rank();
    const int thread_size = dev.team_size() * dev.league_size();
    const int chunk = ( nwork + thread_size - 1 ) / thread_size ;

    size_type iwork = chunk * thread_rank ;
    const size_type iwork_end = iwork + chunk < nwork ? iwork + chunk : nwork ;

    for ( ; iwork < iwork_end ; ++iwork ) {
      dst.value[0] += 1 ;
      dst.value[1] += iwork + 1 ;
      dst.value[2] += nwork - iwork ;
    }
  }
};

} // namespace Test

namespace {

template< typename ScalarType , class DeviceType >
class TestReduceRequest
{
public:
  typedef DeviceType    device_type ;
  typedef typename device_type::size_type size_type ;

  //------------------------------------

  TestReduceRequest( const size_type & nwork )
  {
    run_test(nwork);
  }

  void run_test( const size_type & nwork )
  {
    typedef Test::ReduceRequestFunctor< ScalarType , device_type > functor_type ;
    typedef typename functor_type::value_type value_type ;

    enum { Count = 3 };
    enum { Repeat = 100 };

    value_type result[ Repeat ];

    const unsigned long nw   = nwork ;
    const unsigned long nsum = nw % 2 ? nw * (( nw + 1 )/2 )
                                      : (nw/2) * ( nw + 1 );

    enum { TEAM_SIZE = 256 };

    Kokkos::ParallelWorkRequest request ; 

    request.team_size   = TEAM_SIZE ;
    request.league_size = ( nwork + TEAM_SIZE - 1 ) / TEAM_SIZE ;

    for ( unsigned i = 0 ; i < Repeat ; ++i ) {
      Kokkos::parallel_reduce( request , functor_type(nwork) , result[i] );
    }

    device_type::fence();

    for ( unsigned i = 0 ; i < Repeat ; ++i ) {
      for ( unsigned j = 0 ; j < Count ; ++j ) {
        const unsigned long correct = 0 == j % 3 ? nw : nsum ;
        ASSERT_EQ( (ScalarType) correct , result[i].value[j] );
      }
    }
  }
};

}

/*--------------------------------------------------------------------------*/

namespace Test {

template< class DeviceType >
class ScanRequestFunctor
{
public:
  typedef DeviceType  device_type ;
  typedef long int    value_type ;
  Kokkos::View< value_type , device_type > accum ;
  Kokkos::View< value_type , device_type > total ;

  ScanRequestFunctor() : accum("accum"), total("total") {}

  KOKKOS_INLINE_FUNCTION
  void init( value_type & error ) const { error = 0 ; }

  KOKKOS_INLINE_FUNCTION
  void join( value_type volatile & error ,
             value_type volatile const & input ) const
    { if ( input ) error = 1 ; }

  KOKKOS_INLINE_FUNCTION
  void operator()( device_type dev , value_type & error ) const
  {
    if ( 0 == dev.league_rank() && 0 == dev.team_rank() ) {
      const long int thread_count = dev.league_size() * dev.team_size();
      *total = ( thread_count * ( thread_count + 1 ) ) / 2 ;
    }

    const long int answer =
      ( dev.league_rank() + 1 ) * dev.team_rank() +
      ( dev.team_rank() * ( dev.team_rank() + 1 ) ) / 2 ;
    
    const long int result =
      dev.team_scan( dev.league_rank() + 1 + dev.team_rank() + 1 );

    const long int result2 =
      dev.team_scan( dev.league_rank() + 1 + dev.team_rank() + 1 );

    if ( answer != result || answer != result2 ) {
      printf("ScanRequestFunctor[%d.%d of %d.%d] %ld != %ld or %ld\n",
             dev.league_rank(), dev.team_rank(),
             dev.league_size(), dev.team_size(),
             answer,result,result2);
      error = 1 ;
    }

    const long int thread_rank = dev.team_rank() +
                                 dev.team_size() * dev.league_rank();
    dev.team_scan( 1 + thread_rank , accum.ptr_on_device() );
  }
};

template< class DeviceType >
class TestScanRequest
{
public:
  typedef DeviceType  device_type ;
  typedef long int    value_type ;

  typedef Test::ScanRequestFunctor<DeviceType> functor_type ;

  //------------------------------------

  TestScanRequest( const size_t nteam )
  {
    run_test(nteam);
  }

  void run_test( const size_t nteam )
  {
    const unsigned REPEAT = 100000 ;
    const unsigned Repeat = ( REPEAT + nteam - 1 ) / nteam ;

    Kokkos::ParallelWorkRequest request ; 

    request.team_size   = device_type::team_max();
    request.league_size = nteam ;

    functor_type functor ;

    for ( unsigned i = 0 ; i < Repeat ; ++i ) {
      long int accum = 0 ;
      long int total = 0 ;
      long int error = 0 ;
      Kokkos::deep_copy( functor.accum , total );
      Kokkos::parallel_reduce( request , functor , error );
      DeviceType::fence();
      Kokkos::deep_copy( accum , functor.accum );
      Kokkos::deep_copy( total , functor.total );

      ASSERT_EQ( error , 0 );
      ASSERT_EQ( total , accum );
    }

    device_type::fence();
  }
};

} // namespace Test

/*--------------------------------------------------------------------------*/

namespace Test {

template< class Device >
struct SharedRequestFunctor {

  typedef Device device_type ;
  typedef int    value_type ;

  KOKKOS_INLINE_FUNCTION
  void init( value_type & update ) const
  { update = 0 ; }

  KOKKOS_INLINE_FUNCTION
  void join( volatile value_type & update ,
             volatile const value_type & input ) const
  { update += input ; }
  

  enum { SHARED_COUNT = 1000 };

  typedef Kokkos::View<int*,device_type,Kokkos::MemoryUnmanaged> shared_int_array_type ;

  // Tell how much shared memory will be required by this functor:
  inline
  unsigned shmem_size() const
  {
    return shared_int_array_type::shmem_size( SHARED_COUNT );
  }

  KOKKOS_INLINE_FUNCTION
  void operator()( device_type dev , value_type & update ) const
  {
    const shared_int_array_type shared( dev , SHARED_COUNT );

    for ( int i = dev.team_rank() ; i < SHARED_COUNT ; i += dev.team_size() ) {
      shared[i] = i + dev.league_rank();
    }

    dev.team_barrier();

    if ( dev.team_rank() + 1 == dev.team_size() ) {
      for ( int i = 0 ; i < SHARED_COUNT ; ++i ) {
        if ( shared[i] != i + dev.league_rank() ) {
          ++update ;
        }
      }
    }
  }
};

}

namespace {

template< class Device >
struct TestSharedRequest {

  TestSharedRequest()
  { run(); }

  void run()
  {
    typedef Test::SharedRequestFunctor<Device> Functor ;

    Kokkos::ParallelWorkRequest request ;

    request.team_size   = 256 ;
    request.league_size = 64 ;

    int error_count = 0 ;

    Kokkos::parallel_reduce( request , Functor() , error_count );

    ASSERT_EQ( error_count , 0 );
  }
};

}

/*--------------------------------------------------------------------------*/
