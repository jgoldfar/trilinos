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

#ifndef LINE_SEARCH_2ND_ORDER_CORRECT_STEP_H
#define LINE_SEARCH_2ND_ORDER_CORRECT_STEP_H

#include "MoochoPack/src/rSQPAlgo_StepBaseClasses.h"
#include "MoochoPack_FeasibilityStep_Strategy.hpp"
#include "ConstrainedOptPack_MeritFuncNLP.hpp"
#include "ConstrainedOptPack_DirectLineSearch_Strategy.hpp"
#include "Teuchos_StandardCompositionMacros.hpp"
#include "MiStandardAggregationMacros.h"
#include "Teuchos_StandardMemberCompositionMacros.hpp"

namespace MoochoPack {

/** \brief Implements second order correction.
  *
  * Let the printed documentation describe these parameters.
  */
class LineSearch2ndOrderCorrect_Step : public LineSearch_Step {
public:

  /** \brief . */
  enum ENewtonOutputLevel {
    PRINT_USE_DEFAULT
    ,PRINT_NEWTON_NOTHING      = 0
    ,PRINT_NEWTON_SUMMARY_INFO = 1
    ,PRINT_NEWTON_STEPS        = 2
    ,PRINT_NEWTON_VECTORS      = 3
  };

  /** \brief . */
  enum EForcedConstrReduction { CONSTR_LESS_X_D, CONSTR_LESS_X };

  /** \brief <<std comp>> members for direct_ls_sqp.
    *
    * This is the line search strategy object for the SQP step
    * for x_k+1 = x_k + alpha_k * d_k + alpha_k^2 * w.
    */
  STANDARD_COMPOSITION_MEMBERS(DirectLineSearch_Strategy,direct_ls_sqp)

  /** \brief <<std comp>> members for merit_func.
    *
    * This is the merit function object for SQP step line search.
    */
  STANDARD_COMPOSITION_MEMBERS(MeritFuncNLP,merit_func)

  /** \brief <<std comp>> members for feasibility_step.
    *
    * This is the strategy object that is used to compute feasibility
    * steps for the newton iterations.
    */
  STANDARD_COMPOSITION_MEMBERS(FeasibilityStep_Strategy,feasibility_step)

  /** \brief <<std comp>> members for direct_ls_newton.
    *
    * This is the line search strategy object for the internal
    * newton iterations for determining the second order correction w.
    */
  STANDARD_COMPOSITION_MEMBERS(DirectLineSearch_Strategy,direct_ls_newton)

  /** \brief . */
  LineSearch2ndOrderCorrect_Step(
    const direct_ls_sqp_ptr_t&			direct_ls_sqp			= NULL
    ,const merit_func_ptr_t&			merit_func				= NULL
    ,const feasibility_step_ptr_t&		feasibility_step        = NULL
    ,const direct_ls_newton_ptr_t&		direct_ls_newton		= 0
    ,value_type							eta						= 1.0e-4
    ,ENewtonOutputLevel					newton_olevel			= PRINT_USE_DEFAULT
    ,value_type							constr_norm_threshold	= 1.0
    ,value_type							constr_incr_ratio		= 10.0
    ,int								after_k_iter			= 0
    ,EForcedConstrReduction				forced_constr_reduction	= CONSTR_LESS_X
    ,value_type                         forced_reduct_ratio     = 1.0
    ,value_type							max_step_ratio			= 1.0
    ,int								max_newton_iter			= 3
    );

  /** @name Options for 2nd order correction
    *
    */
  //@{

  /// the Armijo cord fractional reduction test parameter eta
  STANDARD_MEMBER_COMPOSITION_MEMBERS(value_type,eta)

  /// Optput level for newton iterations.
  STANDARD_MEMBER_COMPOSITION_MEMBERS(ENewtonOutputLevel,newton_olevel)

  /// constr_norm_threshold.
  STANDARD_MEMBER_COMPOSITION_MEMBERS(value_type,constr_norm_threshold)

  /// constr_incr_ratio
  STANDARD_MEMBER_COMPOSITION_MEMBERS(value_type,constr_incr_ratio)

  /// after_k_iter.
  STANDARD_MEMBER_COMPOSITION_MEMBERS(int,after_k_iter)

  /// forced_constr_reduction.
  STANDARD_MEMBER_COMPOSITION_MEMBERS(EForcedConstrReduction,forced_constr_reduction)

  /// forced_reduct_ratio
  STANDARD_MEMBER_COMPOSITION_MEMBERS(value_type,forced_reduct_ratio)

  /// max_step_ratio.
  STANDARD_MEMBER_COMPOSITION_MEMBERS(value_type,max_step_ratio)

  /// max_netwon_iter.
  STANDARD_MEMBER_COMPOSITION_MEMBERS(int,max_newton_iter)
  
  //@}

  // ////////////////////
  // Overridden

  /** \brief . */
  bool do_step(Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    , poss_type assoc_step_poss);

  /** \brief . */
  void print_step( const Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    , poss_type assoc_step_poss, std::ostream& out, const std::string& leading_str ) const;

};	// end class LineSearch2ndOrderCorrect_Step

}	// end namespace MoochoPack 

#endif	// LINE_SEARCH_2ND_ORDER_CORRECT_STEP_H
