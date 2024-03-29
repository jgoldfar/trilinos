/*
 * Copyright (c) 1998 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.  
 * 
 *     * Neither the name of Sandia Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_put_n_node_set_df()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      node_set_id        - ID of node set to write.
 *      start_num          - The starting index of the df's to be written.
 *      num_df_to_get      - The number of distribution factors to write out.
 *      node_set_dist_fact - List of node distribution factors in node set.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include <sys/types.h>                  // for int64_t
#include "exodusII.h"


/*!
 * \deprecated Use ex_put_partial_set_dist_fact()
 */

int ex_put_n_node_set_df (int   exoid,
                          ex_entity_id   node_set_id,
                          int64_t   start_num,
                          int64_t   num_df_to_get,
                          void *node_set_dist_fact)
{
  return ex_put_partial_set_dist_fact(exoid, EX_NODE_SET, node_set_id, start_num, num_df_to_get,
				      node_set_dist_fact);
}
