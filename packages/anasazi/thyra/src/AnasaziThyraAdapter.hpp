// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
//                 Copyright (2004) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER

/*! \file AnasaziThyraAdapter.hpp
  \brief Specializations of the Anasazi multi-vector and operator traits 
  classes using Thyra base classes LinearOpBase and MultiVectorBase.
*/

#ifndef ANASAZI_THYRA_ADAPTER_HPP
#define ANASAZI_THYRA_ADAPTER_HPP

#include "AnasaziMultiVecTraits.hpp"
#include "AnasaziOperatorTraits.hpp"
#include "AnasaziConfigDefs.hpp"

#include <Thyra_DetachedMultiVectorView.hpp>
#include <Thyra_MultiVectorBase.hpp>
#include <Thyra_MultiVectorStdOps.hpp>

namespace Anasazi {
  
  ////////////////////////////////////////////////////////////////////////////
  //
  // Implementation of the Anasazi::MultiVecTraits for Thyra::MultiVectorBase
  //
  ////////////////////////////////////////////////////////////////////////////

  /*! 
    \brief Template specialization of Anasazi::MultiVecTraits class using the
    Thyra::MultiVectorBase class.

    This interface will ensure that any implementation of MultiVectorBaseClass 
    will be accepted by the Anasazi templated solvers.  

  */
  template<class ScalarType>
  class MultiVecTraits< ScalarType, Thyra::MultiVectorBase<ScalarType> >
  {
  private:
    typedef Thyra::MultiVectorBase<ScalarType> TMVB;
  public:

    /** \name Creation methods */
    //@{

    /*! \brief Creates a new empty MultiVectorBase containing \c numvecs columns.
      
    \return Reference-counted pointer to the new MultiVectorBase.
    */
    static Teuchos::RCP< Thyra::MultiVectorBase<ScalarType> > Clone( const  Thyra::MultiVectorBase<ScalarType> & mv, const int numvecs )
    { 
      Teuchos::RCP< Thyra::MultiVectorBase<ScalarType> > c = Thyra::createMembers( mv.range(), numvecs ); 
      return c;
    }

    /*! \brief Creates a new MultiVectorBase and copies contents of \c mv into the new vector (deep copy).
      
      \return Reference-counted pointer to the new MultiVectorBase.
    */
    static Teuchos::RCP< Thyra::MultiVectorBase<ScalarType> > CloneCopy( const  Thyra::MultiVectorBase< ScalarType > & mv )
    { 
      int numvecs = mv.domain()->dim();
      // create the new multivector
      Teuchos::RCP<  Thyra::MultiVectorBase< ScalarType >  > cc = Thyra::createMembers( mv.range(), numvecs );
      // copy the data from the source multivector to the new multivector
      Thyra::assign(&*cc, mv);
      return cc;
    }

    /*! \brief Creates a new MultiVectorBase and copies the selected contents of \c mv into the new vector (deep copy).  

      The copied vectors from \c mv are indicated by the \c indeX.size() indices in \c index.      
      \return Reference-counted pointer to the new MultiVectorBase.
    */
    static Teuchos::RCP< Thyra::MultiVectorBase< ScalarType > > CloneCopy( const  Thyra::MultiVectorBase< ScalarType > & mv, const std::vector<int>& index )
    { 
      int numvecs = index.size();
      // create the new multivector
      Teuchos::RCP<  Thyra::MultiVectorBase< ScalarType >  > cc = Thyra::createMembers( mv.range(), numvecs );
      // create a view to the relevant part of the source multivector
      Teuchos::RCP< const  Thyra::MultiVectorBase< ScalarType >  > view = mv.subView( numvecs, &(index[0]) );
      // copy the data from the relevant view to the new multivector
      Thyra::assign(&*cc, *view);
      return cc;
    }

    static Teuchos::RCP<TMVB>
    CloneCopy (const TMVB& mv, const Teuchos::Range1D& index)
    { 
      const int numVecs = index.size();      
      // Create the new multivector
      Teuchos::RCP<TMVB> cc = Thyra::createMembers (mv.range(), numVecs);
      // Create a view to the relevant part of the source multivector
      Teuchos::RCP<const TMVB> view = mv.subView (index);
      // Copy the data from the view to the new multivector.
      // &*cc converts cc from an RCP to a Ptr.
      Thyra::assign (&*cc, *view);
      return cc;
    }

    /*! \brief Creates a new MultiVectorBase that shares the selected contents of \c mv (shallow copy).

    The index of the \c numvecs vectors shallow copied from \c mv are indicated by the indices given in \c index.
    \return Reference-counted pointer to the new MultiVectorBase.
    */      
    static Teuchos::RCP< Thyra::MultiVectorBase< ScalarType > > CloneViewNonConst(  Thyra::MultiVectorBase< ScalarType > & mv, const std::vector<int>& index )
    {
      int numvecs = index.size();

      // We do not assume that the indices are sorted, nor do we check that
      // index.size() > 0. This code is fail-safe, in the sense that a zero
      // length index vector will pass the error on the Thyra.

      // Thyra has two ways to create an indexed View:
      // * contiguous (via a range of columns)
      // * indexed (via a vector of column indices)
      // The former is significantly more efficient than the latter, in terms of
      // computations performed with/against the created view.
      // We will therefore check to see if the given indices are contiguous, and
      // if so, we will use the contiguous view creation method.

      int lb = index[0];
      bool contig = true;
      for (int i=0; i<numvecs; i++) {
        if (lb+i != index[i]) contig = false;
      }

      Teuchos::RCP<  Thyra::MultiVectorBase< ScalarType >  > cc;
      if (contig) {
        const Thyra::Range1D rng(lb,lb+numvecs-1);
        // create a contiguous view to the relevant part of the source multivector
        cc = mv.subView(rng);
      }
      else {
        // create an indexed view to the relevant part of the source multivector
        cc = mv.subView( numvecs, &(index[0]) );
      }
      return cc;
    }

    static Teuchos::RCP<TMVB> 
    CloneViewNonConst (TMVB& mv, const Teuchos::Range1D& index)
    {
      // We let Thyra be responsible for checking that the index range
      // is nonempty.
      //
      // Create and return a contiguous view to the relevant part of
      // the source multivector.
      return mv.subView (index);
    }

    /*! \brief Creates a new const MultiVectorBase that shares the selected contents of \c mv (shallow copy).

    The index of the \c numvecs vectors shallow copied from \c mv are indicated by the indices given in \c index.
    \return Reference-counted pointer to the new const MultiVectorBase.
    */      
    static Teuchos::RCP<const  Thyra::MultiVectorBase< ScalarType > > CloneView( const  Thyra::MultiVectorBase< ScalarType > & mv, const std::vector<int>& index )
    {
      int numvecs = index.size();

      // We do not assume that the indices are sorted, nor do we check that
      // index.size() > 0. This code is fail-safe, in the sense that a zero
      // length index vector will pass the error on the Thyra.

      // Thyra has two ways to create an indexed View:
      // * contiguous (via a range of columns)
      // * indexed (via a vector of column indices)
      // The former is significantly more efficient than the latter, in terms of
      // computations performed with/against the created view.
      // We will therefore check to see if the given indices are contiguous, and
      // if so, we will use the contiguous view creation method.

      int lb = index[0];
      bool contig = true;
      for (int i=0; i<numvecs; i++) {
        if (lb+i != index[i]) contig = false;
      }

      Teuchos::RCP< const  Thyra::MultiVectorBase< ScalarType >  > cc;
      if (contig) {
        const Thyra::Range1D rng(lb,lb+numvecs-1);
        // create a contiguous view to the relevant part of the source multivector
        cc = mv.subView(rng);
      }
      else {
        // create an indexed view to the relevant part of the source multivector
        cc = mv.subView( numvecs, &(index[0]) );
      }
      return cc;
    }

    static Teuchos::RCP<const TMVB> 
    CloneView (const TMVB& mv, const Teuchos::Range1D& index)
    {
      // We let Thyra be responsible for checking that the index range
      // is nonempty.
      //
      // Create and return a contiguous view to the relevant part of
      // the source multivector.
      return mv.subView (index);
    }

    //@}

    /** \name Attribute methods */
    //@{

    //! Obtain the vector length of \c mv.
    static int GetVecLength( const  Thyra::MultiVectorBase< ScalarType > & mv )
    { return mv.range()->dim(); }

    //! Obtain the number of vectors in \c mv
    static int GetNumberVecs( const  Thyra::MultiVectorBase< ScalarType > & mv )
    { return mv.domain()->dim(); }

    //@}

    /** \name Update methods */
    //@{

    /*! \brief Update \c mv with \f$ \alpha AB + \beta mv \f$.
     */
    static void MvTimesMatAddMv( const ScalarType alpha, const  Thyra::MultiVectorBase< ScalarType > & A, 
				 const Teuchos::SerialDenseMatrix<int,ScalarType>& B, 
				 const ScalarType beta,  Thyra::MultiVectorBase< ScalarType > & mv )
    {
      int m = B.numRows();
      int n = B.numCols();
      // Create a view of the B object!
      Teuchos::RCP< const  Thyra::MultiVectorBase< ScalarType >  >
        B_thyra = Thyra::createMembersView(
          A.domain()
          ,RTOpPack::ConstSubMultiVectorView<ScalarType>(0,m,0,n,&B(0,0),B.stride())
          );
      // perform the operation via A: mv <- alpha*A*B_thyra + beta*mv
      A.apply(Thyra::NONCONJ_ELE,*B_thyra,&mv,alpha,beta);
    }

    /*! \brief Replace \c mv with \f$\alpha A + \beta B\f$.
     */
    static void MvAddMv( const ScalarType alpha, const  Thyra::MultiVectorBase< ScalarType > & A, 
                         const ScalarType beta,  const  Thyra::MultiVectorBase< ScalarType > & B,  Thyra::MultiVectorBase< ScalarType > & mv )
    { 
      ScalarType coef[2], zero = Teuchos::ScalarTraits<ScalarType>::zero();
      const  Thyra::MultiVectorBase< ScalarType >  * in[2];

      in[0] = &A;
      in[1] = &B;
      coef[0] = alpha;
      coef[1] = beta;

      Thyra::linear_combination(2,coef,in,zero,&mv);
    }

    /*! \brief Compute a dense matrix \c B through the matrix-matrix multiply \f$ \alpha A^Tmv \f$.
    */
    static void MvTransMv( const ScalarType alpha, const  Thyra::MultiVectorBase< ScalarType > & A, const  Thyra::MultiVectorBase< ScalarType > & mv, 
                           Teuchos::SerialDenseMatrix<int,ScalarType>& B )
    { 
      // Create a multivector to hold the result (m by n)
      int m = A.domain()->dim();
      int n = mv.domain()->dim();
      // Create a view of the B object!
      Teuchos::RCP<  Thyra::MultiVectorBase< ScalarType >  >
        B_thyra = Thyra::createMembersView(
          A.domain()
          ,RTOpPack::SubMultiVectorView<ScalarType>(0,m,0,n,&B(0,0),B.stride())
          );
      A.applyTranspose(Thyra::CONJ_ELE,mv,&*B_thyra,alpha,Teuchos::ScalarTraits<ScalarType>::zero());
    }

    /*! \brief Compute a vector \c b where the components are the individual dot-products of the 
        \c i-th columns of \c A and \c mv, i.e.\f$b[i] = A[i]^Tmv[i]\f$.
     */
    static void MvDot( const  Thyra::MultiVectorBase< ScalarType > & mv, const  Thyra::MultiVectorBase< ScalarType > & A, std::vector<ScalarType> &b )
    { Thyra::dots(mv,A,&(b[0])); }

    /*! \brief Scale each element of the vectors in \c *this with \c alpha.
     */
    static void MvScale (  Thyra::MultiVectorBase< ScalarType > & mv, const ScalarType alpha )
    { Thyra::scale(alpha,&mv); }
    
    /*! \brief Scale each element of the \c i-th vector in \c *this with \c alpha[i].
     */
    static void MvScale (  Thyra::MultiVectorBase< ScalarType > & mv, const std::vector<ScalarType>& alpha ) 
    { 
      for (unsigned int i=0; i<alpha.size(); i++) {
        Thyra::scale(alpha[i],mv.col(i).get());
      }
    }
    
    //@}

    /** \name Norm method */
    //@{

    /*! \brief Compute the 2-norm of each individual vector of \c mv.  
      Upon return, \c normvec[i] holds the value of \f$||mv_i||_2\f$, the \c i-th column of \c mv.
    */
    static void MvNorm( const  Thyra::MultiVectorBase< ScalarType > & mv, std::vector<typename Teuchos::ScalarTraits<ScalarType>::magnitudeType> &normvec )
    { Thyra::norms_2(mv,&(normvec[0])); }

    //@}

    /** \name Initialization methods */
    //@{

    /*! \brief Copy the vectors in \c A to a set of vectors in \c mv indicated by the indices given in \c index.
     */
    static void SetBlock( const  Thyra::MultiVectorBase< ScalarType > & A, const std::vector<int>& index,  Thyra::MultiVectorBase< ScalarType > & mv )
    { 
      // Extract the "numvecs" columns of mv indicated by the index vector.
      int numvecs = index.size();
      std::vector<int> indexA(numvecs);
      int numAcols = A.domain()->dim();
      for (int i=0; i<numvecs; i++) {
        indexA[i] = i;
      }
      // Thyra::assign requires that both arguments have the same number of 
      // vectors. Enforce this, by shrinking one to match the other.
      if ( numAcols < numvecs ) {
        // A does not have enough columns to satisfy index_plus. Shrink
        // index_plus.
        numvecs = numAcols;
      }
      else if ( numAcols > numvecs ) {
        numAcols = numvecs;
        indexA.resize( numAcols );
      }
      // create a view to the relevant part of the source multivector
      Teuchos::RCP< const  Thyra::MultiVectorBase< ScalarType >  > relsource = A.subView( numAcols, &(indexA[0]) );
      // create a view to the relevant part of the destination multivector
      Teuchos::RCP<  Thyra::MultiVectorBase< ScalarType >  > reldest = mv.subView( numvecs, &(index[0]) );
      // copy the data to the destination multivector subview
      Thyra::assign(&*reldest, *relsource);
    }

    static void 
    SetBlock (const TMVB& A, const Teuchos::Range1D& index, TMVB& mv)
    { 
      const int numColsA = A.domain()->dim();
      const int numColsMv = mv.domain()->dim();
      // 'index' indexes into mv; it's the index set of the target.
      const bool validIndex = index.lbound() >= 0 && index.ubound() < numColsMv;
      // We can't take more columns out of A than A has.
      const bool validSource = index.size() <= numColsA;

      if (! validIndex || ! validSource)
	{
	  std::ostringstream os;
	  os << "Anasazi::MultiVecTraits<Scalar, Thyra::MultiVectorBase<Scalar> "
	    ">::SetBlock(A, [" << index.lbound() << ", " << index.ubound() 
	     << "], mv): ";
	  TEST_FOR_EXCEPTION(index.lbound() < 0, std::invalid_argument,
			     os.str() << "Range lower bound must be nonnegative.");
	  TEST_FOR_EXCEPTION(index.ubound() >= numColsMv, std::invalid_argument,
			     os.str() << "Range upper bound must be less than "
			     "the number of columns " << numColsA << " in the "
			     "'mv' output argument.");
	  TEST_FOR_EXCEPTION(index.size() > numColsA, std::invalid_argument,
			     os.str() << "Range must have no more elements than"
			     " the number of columns " << numColsA << " in the "
			     "'A' input argument.");
	  TEST_FOR_EXCEPTION(true, std::logic_error, "Should never get here!");
	}

      // View of the relevant column(s) of the target multivector mv.
      // We avoid view creation overhead by only creating a view if
      // the index range is different than [0, (# columns in mv) - 1].
      Teuchos::RCP<TMVB> mv_view;
      if (index.lbound() == 0 && index.ubound()+1 == numColsMv)
	mv_view = Teuchos::rcpFromRef (mv); // Non-const, non-owning RCP
      else
	mv_view = mv.subView (index);

      // View of the relevant column(s) of the source multivector A.
      // If A has fewer columns than mv_view, then create a view of
      // the first index.size() columns of A.
      Teuchos::RCP<const TMVB> A_view;
      if (index.size() == numColsA)
	A_view = Teuchos::rcpFromRef (A); // Const, non-owning RCP
      else
	A_view = A.subView (Teuchos::Range1D(0, index.size()-1));

      // Copy the data to the destination multivector.  "&*" is a
      // typical Thyra idiom for turning an RCP into a raw pointer.
      Thyra::assign (&*mv_view, *A_view);
    }

    /*! \brief Replace the vectors in \c mv with random vectors.
     */
    static void MvRandom(  Thyra::MultiVectorBase< ScalarType > & mv )
    { 
      // Thyra::randomize generates via a uniform distribution on [l,u]
      // We will use this to generate on [-1,1]
      Thyra::randomize(-Teuchos::ScalarTraits<ScalarType>::one(),
                        Teuchos::ScalarTraits<ScalarType>::one(),
                       &mv);
    }

    /*! \brief Replace each element of the vectors in \c mv with \c alpha.
     */
    static void MvInit(  Thyra::MultiVectorBase< ScalarType > & mv, ScalarType alpha = Teuchos::ScalarTraits<ScalarType>::zero() )
    { Thyra::assign(&mv,alpha); }

    //@}

    /** \name Print method */
    //@{

    /*! \brief Print the \c mv multi-vector to the \c os output stream.
     */
    static void MvPrint( const  Thyra::MultiVectorBase< ScalarType > & mv, std::ostream& os )
    { 
       Teuchos::RCP<Teuchos::FancyOStream> out = Teuchos::getFancyOStream(Teuchos::rcp(&os,false));    
       out->setf(std::ios_base::scientific);    
       out->precision(16);   
       mv.describe(*out,Teuchos::VERB_EXTREME);
    }

    //@}

  };        

  ///////////////////////////////////////////////////////////////////////// 
  //
  // Implementation of the Anasazi::OperatorTraits for Thyra::LinearOpBase
  //
  ///////////////////////////////////////////////////////////////////////// 

  /*! 
                         
    \brief Template specialization of Anasazi::OperatorTraits class using the
    Thyra::LinearOpBase virtual base class and Thyra::MultiVectorBase class.

    This interface will ensure that any LinearOpBase and MultiVectorBase
    implementations will be accepted by the Anasazi templated solvers.

  */
  template <class ScalarType> 
  class OperatorTraits < ScalarType, Thyra::MultiVectorBase<ScalarType>, Thyra::LinearOpBase<ScalarType> >
  {
  public:
    
    /*! \brief This method takes the MultiVectorBase \c x and
      applies the LinearOpBase \c Op to it resulting in the MultiVectorBase \c y.
    */    
    static void Apply ( const Thyra::LinearOpBase< ScalarType >& Op, const  Thyra::MultiVectorBase< ScalarType > & x,  Thyra::MultiVectorBase< ScalarType > & y )
    { 
      Op.apply(Thyra::NONCONJ_ELE,x,&y);
    }
    
  };
  
} // end of Anasazi namespace 

#endif 
// end of file ANASAZI_THYRA_ADAPTER_HPP
