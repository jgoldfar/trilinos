/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#ifndef SUNDANCE_DISCRETEFUNCELEMENT_H
#define SUNDANCE_DISCRETEFUNCELEMENT_H


#include "SundanceDefs.hpp"
#include "SundanceFuncElementBase.hpp"
#include "SundanceDiscreteFuncEvaluator.hpp"
#include "SundanceDiscreteFuncDataStub.hpp"

#ifndef DOXYGEN_DEVELOPER_ONLY

namespace SundanceCore
{
  using namespace SundanceUtils;

  namespace Internal
  {
    using namespace Teuchos;

    using std::string;
    using std::ostream;

    /** 
     * DiscreteFuncElement represents a scalar-valued element
     * of a (possibly) vector-valued discrete function. 
     *
     * DiscreteFuncElement is framework-independent. Any framework-specific
     * information should go in a subclass of DiscreteFuncDataStub.
     * The DiscreteFuncDataStub object can be accessed through the
     * <tt>master()</tt> method of this class.
     */
    class DiscreteFuncElement : public virtual EvaluatableExpr,
                                public FuncElementBase,
                                public virtual GenericEvaluatorFactory<DiscreteFuncElement, DiscreteFuncElementEvaluator>
    {
    public:
      /** */
      DiscreteFuncElement(const RefCountPtr<DiscreteFuncDataStub>& data,
        const string& name,
        const string& suffix,
        int commonFuncID,
        int myIndex);

      /** virtual destructor */
      virtual ~DiscreteFuncElement() {;}


      /** Get the data associated with the vector-valued function 
       * that contains this function element. */
      const DiscreteFuncDataStub* commonData() const {return commonData_.get();}

      /** Get the data associated with the vector-valued function 
       * that contains this function element. */
      DiscreteFuncDataStub* commonData() {return commonData_.get();}

      /** Get my index into the master's list of elements */
      int myIndex() const {return myIndex_;}

      /** Inform this function that it will need to be evaluated using the specified
       * multiIndex*/
      void addMultiIndex(const MultiIndex& newMi) const ;
      
      /** */
      virtual Set<MultipleDeriv> 
      internalFindW(int order, const EvalContext& context) const ;
      /** */
      virtual Set<MultipleDeriv> 
      internalFindV(int order, const EvalContext& context) const ;
      /** */
      virtual Set<MultipleDeriv> 
      internalFindC(int order, const EvalContext& context) const ;

      /** */
      virtual RefCountPtr<Array<Set<MultipleDeriv> > > 
      internalDetermineR(const EvalContext& context,
                         const Array<Set<MultipleDeriv> >& RInput) const ;

      /** */
      virtual XMLObject toXML() const ;

      /** */
      const Set<MultiIndex>& multiIndexSet() const {return miSet_;}

      /** */
      virtual RefCountPtr<Internal::ExprBase> getRcp() {return rcp(this);}
      
    private:

      RefCountPtr<DiscreteFuncDataStub> commonData_;

      int myIndex_;

      mutable Set<MultiIndex> miSet_;
      
#endif /* DOXYGEN_DEVELOPER_ONLY */
    };
  }
}

#endif
