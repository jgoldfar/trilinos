/* /////////////////////////////////////////////
// RTOp_ROp_combined_nu_comp_err.c
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
//
// Note: This file was created automatically by 'new_rtop.pl'
//       on 6/27/2002 at 15:7
//
*/

#include <assert.h>
#include <math.h>

#define max(a,b) ( (a) > (b) ? (a) : (b) )
#define min(a,b) ( (a) < (b) ? (a) : (b) )

#include "RTOp_ROp_combined_nu_comp_err.h"
#include "RTOp_obj_null_vtbl.h"  /* vtbl for operator object instance data */
#include "RTOp_reduct_max_value.h"


/* Implementation functions for RTOp_RTOp */

static int RTOp_ROp_combined_nu_comp_err_apply_op(
  const struct RTOp_RTOp_vtbl_t* vtbl, const void* obj_data
  , const int num_vecs, const struct RTOp_SubVector vecs[]
  , const int num_targ_vecs, const struct RTOp_MutableSubVector targ_vecs[]
  , RTOp_ReductTarget reduct_obj )
{
  /*
  // Declare local variables
  */

    /* Access to the reduction object data */
    RTOp_value_type *comp_err = (RTOp_value_type*)reduct_obj;
    /* Vector data */
    RTOp_index_type           sub_dim;
    /* v0 */
    const RTOp_value_type     *v0_val;
    ptrdiff_t                 v0_val_s;
    /* v1 */
    const RTOp_value_type     *v1_val;
    ptrdiff_t                 v1_val_s;
    /* v2 */
    const RTOp_value_type     *v2_val;
    ptrdiff_t                 v2_val_s;
    /* v3 */
    const RTOp_value_type     *v3_val;
    ptrdiff_t                 v3_val_s;

    register RTOp_index_type  k;

  /*
  // Validate the input
  */
    if( num_vecs != 4 || ( num_vecs && vecs == NULL ) )
        return RTOp_ERR_INVALID_NUM_VECS;
    if( num_targ_vecs != 0 || ( num_targ_vecs && targ_vecs == NULL ) )
        return RTOp_ERR_INVALID_NUM_TARG_VECS;
    if( /* Validate sub_dim */
        vecs[1].sub_dim != vecs[0].sub_dim
        || vecs[2].sub_dim != vecs[0].sub_dim
        || vecs[3].sub_dim != vecs[0].sub_dim
        )
        return RTOp_ERR_INCOMPATIBLE_VECS;
    assert(reduct_obj);


  /*
  // Get pointers to data
  */
    sub_dim       = vecs[0].sub_dim;
    /* v0 */
    v0_val        = vecs[0].values;
    v0_val_s      = vecs[0].values_stride;
    /* v1 */
    v1_val        = vecs[1].values;
    v1_val_s      = vecs[1].values_stride;
    /* v2 */
    v2_val        = vecs[2].values;
    v2_val_s      = vecs[2].values_stride;
    /* v3 */
    v3_val        = vecs[3].values;
    v3_val_s      = vecs[3].values_stride;


  /*
  // Apply the operator:
  //
    //    element-wise reduction      : if (v0(i) < 0)
    */
    for( k = 0; k < sub_dim; ++k, v0_val += v0_val_s, v1_val += v1_val_s, v2_val += v2_val_s, v3_val += v3_val_s )
    {
        /* Element-wise reduction */
    (*comp_err) = max( *comp_err, (*v0_val) * ((*v3_val) - (*v1_val)));
    (*comp_err) = max( *comp_err, -(*v0_val) * ((*v1_val) - (*v2_val)));
    }

  return 0; /* success? */
}



/* Name of this transformation operator class */
const char RTOp_ROp_combined_nu_comp_err_name[] = "ROp_combined_nu_comp_err";

/* Virtual function table */
const struct RTOp_RTOp_vtbl_t RTOp_ROp_combined_nu_comp_err_vtbl =
{
  &RTOp_obj_null_vtbl
  ,&RTOp_obj_value_vtbl
  ,NULL
  ,RTOp_ROp_combined_nu_comp_err_apply_op
  ,RTOp_reduct_max_value
  ,RTOp_get_reduct_max_value_op
};

/* Class specific functions */

int RTOp_ROp_combined_nu_comp_err_construct(  struct RTOp_RTOp* op )
{
#ifdef RTOp_DEBUG
  assert(op);
#endif
  op->obj_data  = NULL;
  op->vtbl      = &RTOp_ROp_combined_nu_comp_err_vtbl;
  op->vtbl->obj_data_vtbl->obj_create(NULL,NULL,&op->obj_data);
  return 0;
}

int RTOp_ROp_combined_nu_comp_err_destroy( struct RTOp_RTOp* op )
{
  op->vtbl->obj_data_vtbl->obj_free(NULL,NULL,&op->obj_data);
  op->obj_data  = NULL;
  op->vtbl      = NULL;
  return 0;
}


RTOp_value_type RTOp_ROp_combined_nu_comp_err_val(RTOp_ReductTarget reduct_obj)
{
    return *((RTOp_value_type*)reduct_obj);
}



/* Implementation functions for RTOp_RTOp */

static int RTOp_ROp_combined_nu_comp_err_one_only_apply_op(
  const struct RTOp_RTOp_vtbl_t* vtbl, const void* obj_data
  , const int num_vecs, const struct RTOp_SubVector vecs[]
  , const int num_targ_vecs, const struct RTOp_MutableSubVector targ_vecs[]
  , RTOp_ReductTarget reduct_obj )
{
  /*
  // Declare local variables
  */

    /* Access to the reduction object data */
    RTOp_value_type *comp_err = (RTOp_value_type*)reduct_obj;
    /* Vector data */
    RTOp_index_type           sub_dim;
    /* v0 */
    const RTOp_value_type     *v0_val;
    ptrdiff_t                 v0_val_s;
    /* v1 */
    const RTOp_value_type     *v1_val;
    ptrdiff_t                 v1_val_s;
    /* v2 */
    const RTOp_value_type     *v2_val;
    ptrdiff_t                 v2_val_s;

    register RTOp_index_type  k;

  /*
  // Validate the input
  */
    if( num_vecs != 3 || ( num_vecs && vecs == NULL ) )
        return RTOp_ERR_INVALID_NUM_VECS;
    if( num_targ_vecs != 0 || ( num_targ_vecs && targ_vecs == NULL ) )
        return RTOp_ERR_INVALID_NUM_TARG_VECS;
    if( /* Validate sub_dim */
        vecs[1].sub_dim != vecs[0].sub_dim
        || vecs[2].sub_dim != vecs[0].sub_dim
        )
        return RTOp_ERR_INCOMPATIBLE_VECS;
    assert(reduct_obj);


  /*
  // Get pointers to data
  */
    sub_dim       = vecs[0].sub_dim;
    /* v0 */
    v0_val        = vecs[0].values;
    v0_val_s      = vecs[0].values_stride;
    /* v1 */
    v1_val        = vecs[1].values;
    v1_val_s      = vecs[1].values_stride;
    /* v2 */
    v2_val        = vecs[2].values;
    v2_val_s      = vecs[2].values_stride;


  /*
  // Apply the operator:
  //
    //    element-wise reduction      : comp_err = max(comp_err, v0(i)*(v1(i)-v2(i)));
    */
    for( k = 0; k < sub_dim; ++k, v0_val += v0_val_s, v1_val += v1_val_s, v2_val += v2_val_s )
    {
        /* Element-wise reduction */
        (*comp_err) = max((*comp_err), (*v0_val)*((*v1_val)-(*v2_val)));
    }

  return 0; /* success? */
}



/* Name of this transformation operator class */
const char RTOp_ROp_combined_nu_comp_err_one_only_name[] = "combined_nu_comp_err_one_only";

/* Virtual function table */
const struct RTOp_RTOp_vtbl_t RTOp_ROp_combined_nu_comp_err_one_only_vtbl =
{
  &RTOp_obj_null_vtbl
  ,&RTOp_obj_value_vtbl
  ,NULL
  ,RTOp_ROp_combined_nu_comp_err_one_only_apply_op
  ,RTOp_reduct_max_value
  ,RTOp_get_reduct_max_value_op
};

/* Class specific functions */

int RTOp_ROp_combined_nu_comp_err_one_only_construct(  struct RTOp_RTOp* op )
{
#ifdef RTOp_DEBUG
  assert(op);
#endif
  op->obj_data  = NULL;
  op->vtbl      = &RTOp_ROp_combined_nu_comp_err_one_only_vtbl;
  op->vtbl->obj_data_vtbl->obj_create(NULL,NULL,&op->obj_data);
  return 0;
}

int RTOp_ROp_combined_nu_comp_err_one_only_destroy( struct RTOp_RTOp* op )
{
  op->vtbl->obj_data_vtbl->obj_free(NULL,NULL,&op->obj_data);
  op->obj_data  = NULL;
  op->vtbl      = NULL;
  return 0;
}


RTOp_value_type RTOp_ROp_combined_nu_comp_err_one_only_val(RTOp_ReductTarget reduct_obj)
{
    return *((RTOp_value_type*)reduct_obj);
}

