/* /////////////////////////////////////////////
// RTOp_ROp_max_near_feas_step.h
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.
*/

#ifndef RTOP_ROP_MAX_NEAR_FEAS_STEP_H
#define RTOP_ROP_MAX_NEAR_FEAS_STEP_H

#include "RTOp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file RTOp_ROp_max_near_feas_step.h Reduction operator for finding the maximum near feasibl step.
 *
 * <tt>targ_obj <- { max alpha | v[0] - beta <= v[1] + alpha * v[2] <= v[3] + beta }</tt>
 *
 * This is a specialized reduction operation that is used in many optimization
 * methods to find the maximum step length <tt>alpha# such that the
 * iterates remain positive.  This reduction operation
 * returns a target object which consists of a set of values
 * <tt>targ_obj = { alpha_pos, alpha_neg }</tt>.  Here, <tt>alpha_pos# and <tt>alpha_neg</tt>
 * are the largets positive and negative steps <tt>alpha# respectively that
 * satifies the relaxed bounds.  If <tt>alpha_pos < 0.0</tt> on return then this is
 * a flag that <tt>v[1]# is already out of bounds and the value of <tt>alpha_neg</tt>
 * is insignificant.
 *
 * This operator is defined to allow exactly four vector arguments
 * (<tt>num_vecs == 2</tt>) <tt>v[0]</tt>, <tt>v[1]</tt>, and can only handle dense vectors.
 */
/*@{*/

/** Reduction target object for this max_step operation.
  */
struct RTOp_ROp_max_near_feas_step_reduct_obj_t {

  RTOp_value_type   alpha_pos;

  RTOp_value_type   alpha_neg;
};

/** Name of this reduction operator class */
extern const char RTOp_ROp_max_near_feas_step_name[];

/** Virtual function table */
extern const struct RTOp_RTOp_vtbl_t RTOp_ROp_max_near_feas_step_vtbl;

/** Constructor */
int RTOp_ROp_max_near_feas_step_construct( RTOp_value_type beta, struct RTOp_RTOp* op );

/** Destructor */
int RTOp_ROp_max_near_feas_step_destroy( struct RTOp_RTOp* op );

/** Reset beta */
int RTOp_ROp_max_near_feas_step_set_beta( RTOp_value_type beta, struct RTOp_RTOp* op );

/** Extract the concrete reduction target object from its pointer (handle). */
struct RTOp_ROp_max_near_feas_step_reduct_obj_t
RTOp_ROp_max_near_feas_step_val(RTOp_ReductTarget targ_obj);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif  /* RTOP_ROP_MAX_NEAR_FEAS_STEP_H */
