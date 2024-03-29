/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2009 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA

    (2009) kraftche@cae.wisc.edu    

  ***************************************************************** */


/** \file SizeMetric.hpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#ifndef MSQ_SIZE_METRIC_HPP
#define MSQ_SIZE_METRIC_HPP

#include "Mesquite.hpp"
#include "ElementQM.hpp"

namespace MESQUITE_NS {

/**\brief Element size (area or volume)
 *
 * This metric is intended primarily for use with QualityAssessor
 * for obtaining a summary of the element sizes in the mesh.
 */
class SizeMetric : public ElementQM
{
public:
     MESQUITE_EXPORT virtual ~SizeMetric();

     MESQUITE_EXPORT virtual std::string get_name() const;

     MESQUITE_EXPORT virtual int get_negate_flag() const;
  
     MESQUITE_EXPORT virtual
     bool evaluate( PatchData& pd, 
                    size_t handle, 
                    double& value, 
                    MsqError& err );
};

} // namespace MESQUITE_NS

#endif
