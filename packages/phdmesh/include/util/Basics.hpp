/*------------------------------------------------------------------------*/
/*      phdMesh : Parallel Heterogneous Dynamic unstructured Mesh         */
/*                Copyright (2007) Sandia Corporation                     */
/*                                                                        */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*                                                                        */
/*  This library is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU Lesser General Public License as        */
/*  published by the Free Software Foundation; either version 2.1 of the  */
/*  License, or (at your option) any later version.                       */
/*                                                                        */
/*  This library is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     */
/*  Lesser General Public License for more details.                       */
/*                                                                        */
/*  You should have received a copy of the GNU Lesser General Public      */
/*  License along with this library; if not, write to the Free Software   */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   */
/*  USA                                                                   */
/*------------------------------------------------------------------------*/
/**
 * @author H. Carter Edwards  <hcedwar@sandia.gov>
 * @date   November 2006
 */

#ifndef util_Basics_hpp
#define util_Basics_hpp

namespace phdmesh {

//----------------------------------------------------------------------
// Compile time assertion
//     enum { ok = StaticAssert< logical_expression >::OK };
//     StaticAssert< logical_expression >::ok();
//  For logical_expression == true  it generates a valid no-op
//  For logical_expression == false it generates a compile error

template<bool> struct StaticAssert ;

template<> struct StaticAssert<true> {
  enum { OK = true };
  static bool ok() { return true ; }
};

template<> struct StaticAssert<false> {};

//----------------------------------------------------------------------
// Compile time comparison of types

template<typename T1, typename T2> struct SameType ;

template<typename T> struct SameType<T,T> { enum { value = true }; };

template <typename T1, typename T2> struct SameType { enum { value = false }; };

//----------------------------------------------------------------------

} // namespace phdmesh

#endif

