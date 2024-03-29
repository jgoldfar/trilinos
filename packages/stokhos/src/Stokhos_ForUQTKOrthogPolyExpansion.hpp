// @HEADER
// ***********************************************************************
// 
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef STOKHOS_FORUQTKORTHOGPOLYEXPANSION_HPP
#define STOKHOS_FORUQTKORTHOGPOLYEXPANSION_HPP

#include "Stokhos_ConfigDefs.h"
#ifdef HAVE_STOKHOS_FORUQTK

#include "Stokhos_OrthogPolyExpansionBase.hpp"

namespace Stokhos {

  /*! 
   * Orthogonal polynomial expansions based on methods provided by UQTK 
   * (fortran version).  These include Taylor series and time integration
   * calculations for transcendental operations.
   */
  template <typename ordinal_type, typename value_type> 
  class ForUQTKOrthogPolyExpansion : 
    public OrthogPolyExpansionBase<ordinal_type, value_type, 
				   Stokhos::StandardStorage<ordinal_type, value_type> > {
  public:

    typedef Stokhos::StandardStorage<ordinal_type, value_type> node_type;

    enum EXPANSION_METHOD {
      TAYLOR,
      INTEGRATION
    };

    //! Constructor
    ForUQTKOrthogPolyExpansion(
      const Teuchos::RCP<const OrthogPolyBasis<ordinal_type,value_type> >& basis,
      const Teuchos::RCP<const Stokhos::Sparse3Tensor<ordinal_type, value_type> >& Cijk,
			       EXPANSION_METHOD method = TAYLOR,
			       value_type rtol = 1.0e-12);

    //! Destructor
    virtual ~ForUQTKOrthogPolyExpansion() {}
 
    // Operations
    void timesEqual(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
		    const value_type& x);
    void divideEqual(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
		     const value_type& x);

    void timesEqual(
      OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
      const OrthogPolyApprox<ordinal_type, value_type, node_type>& x);
    void divideEqual(
      OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
      const OrthogPolyApprox<ordinal_type, value_type, node_type>& x);

   
    void times(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void times(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const value_type& a, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void times(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
	       const value_type& b);
    void divide(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
		const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
		const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void divide(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
		const value_type& a, 
		const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void divide(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
		const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
		const value_type& b);

    void exp(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void log(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void log10(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void sqrt(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void pow(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void pow(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const value_type& a, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void pow(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
	     const value_type& b);
    void cos(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void sin(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void tan(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	     const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void cosh(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void sinh(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void tanh(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void acos(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void asin(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void atan(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	      const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void atan2(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a,
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void atan2(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const value_type& a, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& b);
    void atan2(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a, 
	       const value_type& b);
    void acosh(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void asinh(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);
    void atanh(OrthogPolyApprox<ordinal_type, value_type, node_type>& c, 
	       const OrthogPolyApprox<ordinal_type, value_type, node_type>& a);

  private:

    // Prohibit copying
    ForUQTKOrthogPolyExpansion(const ForUQTKOrthogPolyExpansion&);

    // Prohibit Assignment
    ForUQTKOrthogPolyExpansion& operator=(const ForUQTKOrthogPolyExpansion& b);

  protected:

    //! Order
    int order;
    
    //! Dimension
    int dim;

    //! Total size
    ordinal_type sz;

    //! Tolerance for Taylor method
    double rtol;

    //! Expansion method
    EXPANSION_METHOD method;
    
  }; // class ForUQTKOrthogPolyExpansion

} // namespace Stokhos

#include "Stokhos_ForUQTKOrthogPolyExpansionImp.hpp"

#endif // HAVE_STOKHOS_FORUQTK

#endif // STOKHOS_FORUQTKORTHOGPOLYEXPANSION_HPP
