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

#include "SundanceUnknownFunctionStub.hpp"
#include "SundanceUnknownFuncElement.hpp"
#include "SundanceSpectralBasis.hpp"
#include "SundanceSpectralExpr.hpp"


using namespace SundanceCore;
using namespace SundanceUtils;

using namespace SundanceCore::Internal;
using namespace SundanceCore::Internal;
using namespace Teuchos;



UnknownFunctionStub::UnknownFunctionStub(const string& name, 
  const Array<int>& dims,
  const RefCountPtr<const UnknownFuncDataStub>& data)
  : SymbolicFunc(), data_(data)
{
  string funcSuffix;
  int count = 0;
  for (unsigned int f=0; f<dims.size(); f++)
  {
    if (dims.size() > 1U) funcSuffix = "[" + Teuchos::toString(f) + "]";
    int commonFuncID = nextCommonID();
    for (int d=0; d<dims[f]; d++, count++)
    {
      string componentSuffix;
      if (dims[f]>1) componentSuffix = "[" + Teuchos::toString(d) + "]";
      string suffix = funcSuffix + componentSuffix;
      append(new UnknownFuncElement(data, name, suffix, commonFuncID, count));
    }
  }
}



UnknownFunctionStub::UnknownFunctionStub(const string& name, 
  const SpectralBasis& sbasis, const Array<int>& dims,
  const RefCountPtr<const UnknownFuncDataStub>& data)
  : SymbolicFunc(), data_(data)
{
  string funcSuffix;
  int count = 0;
  for (unsigned int f=0; f<dims.size(); f++)
  {
    if (dims.size() > 1U) funcSuffix = "[" + Teuchos::toString(f) + "]";
    Array<int> cfid(sbasis.nterms());
    for (int n=0; n<sbasis.nterms(); n++)
    {
      cfid[n] = nextCommonID();
    }
    for (int d=0; d<dims[f]; d++)
    {
      string componentSuffix;
      if (dims[f]>1) componentSuffix = "[" + Teuchos::toString(d) + "]";
      string suffix = funcSuffix + componentSuffix;
      Array<Expr> coeffs(sbasis.nterms());
      for (int n=0; n<sbasis.nterms(); n++, count++)
      {
        coeffs[n] = new UnknownFuncElement(data, name, suffix, cfid[n], count);
      }
      append(new SpectralExpr(sbasis, coeffs));
    }
  }
}



