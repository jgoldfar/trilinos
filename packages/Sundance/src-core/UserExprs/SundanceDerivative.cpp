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

#include "SundanceDerivative.hpp"

using namespace Sundance;
using namespace Sundance;
using namespace Teuchos;

Derivative::Derivative(int direction)
	: ScalarExpr(), m_()
{
	m_[direction] = 1;
}

bool Derivative::lessThan(const ScalarExpr* other) const
{
  const Derivative* d = dynamic_cast<const Derivative*>(other);
  TEST_FOR_EXCEPTION(d==0, InternalError, "cast should never fail at this point");
  return multiIndex() < d->multiIndex();
}


std::ostream& Derivative::toText(std::ostream& os, bool /* paren */) const 
{
	os << "D[" << m_.toString() << "]";
	return os;
}


XMLObject Derivative::toXML() const 
{
	XMLObject rtn("Derivative");
	rtn.addAttribute("m", m_.toString());
	return rtn;
}


