/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2004 Sandia Corporation and Argonne National
    Laboratory.  Under the terms of Contract DE-AC04-94AL85000 
    with Sandia Corporation, the U.S. Government retains certain 
    rights in this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
    diachin2@llnl.gov, djmelan@sandia.gov, mbrewer@sandia.gov, 
    pknupp@sandia.gov, tleurent@mcs.anl.gov, tmunson@mcs.anl.gov      
   
  ***************************************************************** */
// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-

/*! \file CompositeOFAdd.hpp

Header file for the Mesquite:: CompositeOFAdd class

  \author Michael Brewer
  \date   2002-06-24
 */


#ifndef CompositeOFAdd_hpp
#define CompositeOFAdd_hpp

#include "Mesquite.hpp"
#include "MesquiteError.hpp"
#include "ObjectiveFunction.hpp"
#include "PatchData.hpp"
#include <list>

MSQ_USE(list);

namespace Mesquite
{
     /*!\class CompositeOFAdd
       \brief Adds two ObjectiveFunction values together.
     */
   class MsqMeshEntity;
   class PatchData;
   class CompositeOFAdd : public ObjectiveFunction
   {
   public:
     CompositeOFAdd(ObjectiveFunction*, ObjectiveFunction*);
     virtual ~CompositeOFAdd();
     virtual bool concrete_evaluate(PatchData &patch, double &fval,
                                    MsqError &err);
     virtual list<QualityMetric*> get_quality_metric_list();
     
	protected:
     bool compute_analytical_gradient(PatchData &patch,Vector3D *const &grad,
                                      double &OF_val,  MsqError &err,
				      size_t array_size);
	private:
     ObjectiveFunction* objFunc1;
     ObjectiveFunction* objFunc2;
   };
}//namespace
#endif //  CompositeOFAdd_hpp
