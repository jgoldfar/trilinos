// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef EVAL_NEW_POINT_TAILORED_APPROACH_STEP_H
#define EVAL_NEW_POINT_TAILORED_APPROACH_STEP_H

#include "MoochoPack_Types.hpp"
#include "IterationPack_AlgorithmStep.hpp"
#include "ConstrainedOptPack_VariableBoundsTester.hpp"
#include "NLPInterfacePack_NLPDirectTester.hpp"
#include "Teuchos_StandardCompositionMacros.hpp"
#include "Teuchos_StandardMemberCompositionMacros.hpp"

namespace MoochoPack {

/** \brief Base class for evaluating a new point for the "Tailored Approach".
 *
 * Uses the \c NLPDirect interface to compute <tt>Z = [ -inv(C)*N; I ]</tt>
 * and <tt>py = -inv(C)*c(decomp)</tt> explicitly.  Subclasses determine how
 * <tt>py</tt> and <tt>Y</tt> are updated.
 */
class EvalNewPointTailoredApproach_Step
  : public IterationPack::AlgorithmStep // doxygen needs full path
{
public:

  /** @name Public types */
  //@{
  /** \brief . */
  enum EFDDerivTesting { FD_DEFAULT, FD_TEST, FD_NO_TEST };
  //@}

  /** @name Constructors / initializers */
  //@{

  /** \brief . */
  typedef Teuchos::RefCountPtr<const MatrixOp> D_ptr_t;
  /// <<std comp>> members for testing object for NLPDirect
  STANDARD_COMPOSITION_MEMBERS( NLPDirectTester, deriv_tester )
  /// <<std comp>> Members for variable bounds tester object
  STANDARD_COMPOSITION_MEMBERS( VariableBoundsTester, bounds_tester )
  /** \brief Set how and if finite derivatives are tested.
   *
   * ToDo: Finish documentation.
   */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( EFDDerivTesting, fd_deriv_testing )

  /** \brief . */
  EvalNewPointTailoredApproach_Step(
      const deriv_tester_ptr_t& 	deriv_tester
    , const bounds_tester_ptr_t&	bounds_tester
    , EFDDerivTesting				fd_deriv_testing = FD_DEFAULT
    );

  //@}

  /** @name Overridden from AlgorithmStep */
  //@{
  /** \brief . */
  bool do_step(Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    , poss_type assoc_step_poss);
  /** \brief . */
  void print_step( const Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    , poss_type assoc_step_poss, std::ostream& out, const std::string& leading_str ) const;
  //@}

  /** @name To be overridden by subclasses */
  //@{

  /** \brief Call to uninitialize the matrices.
   *
   * ToDo: Finish documentation!
   */
  virtual void uninitialize_Y_Uy(
    MatrixOp         *Y
    ,MatrixOp        *Uy
    ) = 0;

  /** \brief Overridden by subclass to compute \c py, \c Y and \c Uy.
   *
   * @param  D    [in/out] Smart pointer to matrix <tt>D = -inv(C)*N</tt>.
   *              On output, D->count() may be incremented in order to
   *              initialize <tt>Y</tt>.
   * @param  py   [in/out] On input <tt>py = -inv(C)*c(decomp)</tt>.
   *              On output <tt>py = -inv((Gc(decomp)'*Y)*c(decomp)</tt>
   * @param  Y    [in/out] On ouput <tt>Y</tt> is initialized properly.
   * @param  Uy   [in/out] On ouput <tt>Uy</tt> is initialized properly.
   * @param  olevel
   *              [in] Determines output level.
   * @param  out  [out] Journal outputting.
   */
  virtual void calc_py_Y_Uy(
    const NLPDirect       &nlp
    ,const D_ptr_t        &D
    ,VectorMutable        *py
    ,MatrixOp             *Y
    ,MatrixOp             *Uy
    ,EJournalOutputLevel  olevel
    ,std::ostream         &out
    ) = 0;

  /** \brief Overridden by subclass to recompute \c py and \c Ypy.
   *
   * @param  D    [in] matrix <tt>D = -inv(C)*N</tt>
   * @param  py   [in/out] On input <tt>py = -inv(C)*c(decomp)</tt>.
   *              On output <tt>py = -inv((Gc(decomp)'*Y)*c(decomp)</tt>
   */
  virtual void recalc_py(
    const MatrixOp          &D
    ,VectorMutable          *py
    ,EJournalOutputLevel    olevel
    ,std::ostream           &out
    ) = 0;
  
  /** \brief Overridden by subclass to print how \c py and \c Y are computed.
   */
  virtual void print_calc_py_Y_Uy(
    std::ostream& out, const std::string& leading_str
    ) const = 0;

  //@}

private:

  // Not defined and not to be called
  EvalNewPointTailoredApproach_Step();

};	// end class EvalNewPointTailoredApproach_Step

}	// end namespace MoochoPack 

#endif	// EVAL_NEW_POINT_TAILORED_APPROACH_STEP_H
