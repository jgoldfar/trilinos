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
/*****************************************************************************
*
* ne_gennm - ex_get_partial_elem_num_map
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*	int	exoid			exodus file id
*	int	start_ent		starting location for read
*	int	num_ents		number of elemental points
*
* exit conditions - 
*	int*	elem_map		element number map array
*
* revision history - 
*
*****************************************************************************/

#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for sprintf
#include <sys/types.h>                  // for int64_t
#include "exodusII.h"                   // for exerrval, ex_err, etc
#include "exodusII_int.h"               // for EX_FATAL, DIM_NUM_ELEM, etc
#include "netcdf.h"                     // for NC_NOERR, nc_get_vara_int, etc

/*
 *  reads the element numbering map from the database; allows element numbers
 *  to be noncontiguous
 */

int ex_get_partial_elem_num_map (int  exoid,
			   int64_t  start_ent,
			   int64_t  num_ents,
			   void_int *elem_map)
{
  int numelemdim, mapid, status;
  size_t i;
  size_t num_elem,  start[1], count[1]; 
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire id's of previously defined dimensions and variables  */
  if ((status = nc_inq_dimid (exoid, DIM_NUM_ELEM, &numelemdim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate number of elements in file id %d",
	    exoid);
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_dimlen (exoid, numelemdim, &num_elem)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get number of elements in file id %d",
	    exoid);
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* Check input parameters for a valid range of numbers */
  if (start_ent < 0 || start_ent > num_elem) {
    exerrval = status;
    sprintf(errmsg,
            "Error: Invalid input to function ex_get_partial_elem_num_map!\n");
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (num_ents < 0) {
    exerrval = status;
    sprintf(errmsg, "Error: Invalid number of entries in map!\n");
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* start_ent now starts at 1, not 0 */
  if ((start_ent + num_ents - 1) > num_elem) {
    exerrval = status;
    sprintf(errmsg, "Error: request range invalid!\n");
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_varid (exoid, VAR_ELEM_NUM_MAP, &mapid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Warning: elem numbering map not stored in file id %d; returning default map",
	    exoid);
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);

    /* generate default map of 1..n, where n is num_elem */
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      int64_t *lmap = (int64_t*)elem_map;
      for (i=0; i<num_ents; i++) {
	lmap[i] = start_ent+i;
      }
    } else {
      int *lmap = (int*)elem_map;
      for (i=0; i<num_ents; i++) {
	lmap[i] = start_ent+i;
      }
    }
    return (EX_WARN);
  }


  /* read in the element numbering map  */
  start[0] = --start_ent;
  count[0] = num_ents;

  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_get_vara_longlong(exoid, mapid, start, count, elem_map);
  } else {
    status = nc_get_vara_int(exoid, mapid, start, count, elem_map);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to get element number map in file id %d",
	    exoid);
    ex_err("ex_get_partial_elem_num_map",errmsg,exerrval);
    return (EX_FATAL);
  }

  return(EX_NOERR);

}
