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

#include <iostream>

#include <Kokkos_Threads.hpp>
#include <Kokkos_hwloc.hpp>

#include <Kokkos_LegendrePolynomial.hpp>
#include <Kokkos_SymmetricDiagonalSpec.hpp>
#include <Kokkos_BlockCrsMatrix.hpp>
#include <Kokkos_CrsMatrix.hpp>


//

#include <Host/Kokkos_Host_SymmetricDiagonalSpec.hpp>
#include <Host/Kokkos_Host_BlockCrsMatrix.hpp>
#include <Host/Kokkos_Host_CrsMatrix.hpp>

//

#include <TestBlockCrsMatrix.hpp>
#include <TestStochastic.hpp>

namespace unit_test {

template<typename Scalar>
struct performance_test_driver<Scalar,Kokkos::Threads> {

  static void run(bool test_flat, bool test_orig, bool test_block, bool check) {
    typedef Kokkos::Threads Device;

    int nGrid;
    int nIter; 
    bool print;
    
    // All methods compared against flat-original
    if (test_flat) {
      nGrid = 5 ;
      nIter = 1 ; 
      print = false ;
      performance_test_driver_all<Scalar,Device>( 
	3 , 1 ,  9 , nGrid , nIter , print , test_block , check );
      performance_test_driver_all<Scalar,Device>( 
	5 , 1 ,  5 , nGrid , nIter , print , test_block , check );
    }
    
    // Just polynomial methods compared against original
    if (test_orig) {
      nGrid = 64 ;
      nIter = 1 ; 
      print = false ;
      performance_test_driver_poly<Scalar,Device>( 
	3 , 1 , 12 , nGrid , nIter , print , test_block , check );
      performance_test_driver_poly<Scalar,Device>( 
	5 , 1 ,  6 , nGrid , nIter , print , test_block , check );
    }
    
    //------------------------------
    
    /*
      std::cout << std::endl
      << "\"CRS flat-matrix ~27 nonzeros/row (CUDA uses cusparse)\""
      << std::endl
      << "\"nGrid\" , "
      << "\"VectorSize\" , "
      << "\"MXV-Time\""
      << std::endl ;
      
      for ( int n_grid = 10 ; n_grid <= 100 ; n_grid += 5 ) {
      
      const std::pair<size_t,double> perf_flat =
      test_flat_matrix<double,Device>( n_grid , nIter , print );
      
      std::cout << n_grid << " , "
      << perf_flat.first << " , "
      << perf_flat.second
      << std::endl ;
      }
    */

    //------------------------------
  }

};

}

template <typename Scalar>
int mainHost(bool test_flat, bool test_orig, bool test_block, bool check)
{
  const unsigned team_count = Kokkos::hwloc::get_available_numa_count();
  const unsigned threads_per_team = ( Kokkos::hwloc::get_available_cores_per_numa() + 1 ) / 2 ;

  Kokkos::Threads::initialize( team_count * threads_per_team );

  typedef Kokkos::Threads device ;

  //------------------------------------
  // Quick correctness check:

  const std::vector<int> var( 3 , 2 ); // #Stochastic variables = 3 , polynomical degree = 2
  const int ngrid = 3 ; // 3x3x3 element grid

  unit_test::test_product_flat_original_matrix<  float, device>( var , ngrid , 1 , true );
  unit_test::test_product_flat_original_matrix<  double,device>( var , ngrid , 1 , true );

  unit_test::test_product_flat_commuted_matrix<  float, device>( var , ngrid , 1 , true );
  unit_test::test_product_flat_commuted_matrix<  double,device>( var , ngrid , 1 , true );

  unit_test::test_product_tensor_diagonal_matrix<float, device>( var , ngrid , 1 , true );
  unit_test::test_product_tensor_diagonal_matrix<double,device>( var , ngrid , 1 , true );

  unit_test::test_product_tensor_legendre< Kokkos::CrsProductTensorLegendre< float , device > , float , float >( var , ngrid , 1 , true );
  unit_test::test_product_tensor_legendre< Kokkos::CrsProductTensorLegendre< double, device > , double, double>( var , ngrid , 1 , true );

  unit_test::test_product_tensor_legendre< Kokkos::SparseProductTensorLegendre< float , device > , float , float >( var , ngrid , 1 , true );
  unit_test::test_product_tensor_legendre< Kokkos::SparseProductTensorLegendre< double, device > , double, double>( var , ngrid , 1 , true );

  //------------------------------------

  std::cout << std::endl << "\"Threads Performance with "
            << league_gang.first * league_gang.second << " threads\"" << std::endl ;

  unit_test::performance_test_driver<Scalar,device>::run(
    test_flat, test_orig, test_block, check);

  Kokkos::Threads::finalize();

  return 0 ;
}

template int mainHost<float>(bool, bool, bool, bool);
template int mainHost<double>(bool, bool, bool, bool);
