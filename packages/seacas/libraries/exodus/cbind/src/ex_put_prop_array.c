/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
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

#include <stddef.h>                     // for size_t
#include <stdio.h>                      // for sprintf
#include <string.h>                     // for strcpy, memset, strlen, etc
#include "exodusII.h"                   // for ex_err, exerrval, etc
#include "exodusII_int.h"               // for EX_FATAL, ATT_PROP_NAME, etc
#include "netcdf.h"                     // for NC_NOERR, nc_set_fill, etc

/*!

The function ex_put_prop_array() stores an array of ({num_elem_blk},
\c num_node_sets, or \c num_side_sets) integer property values for all
element blocks, node sets, or side sets. The order of the values in
the array must correspond to the order in which the element blocks,
node sets, or side sets were introduced into the file. For instance,
if the parameters for element block with ID 20 were written to a file
(via ex_put_elem_block()), and then parameters for element block with
ID 10, followed by the parameters for element block with ID 30, the
first, second, and third elements in the property array would
correspond to element block 20, element block 10, and element block
30, respectively.

One should note that this same functionality (writing properties to
multiple objects) can be accomplished with multiple calls to
ex_put_prop().

Although it is not necessary to invoke ex_put_prop_names(), since
ex_put_prop_array() will allocate space within the data file if it
hasn't been previously allocated, it is more efficient to use
ex_put_prop_names() if there is more than one property to store. \see
See Efficiency for a discussion of efficiency issues.

\return In case of an error, ex_put_prop_array() returns a negative
number; a warning will return a positive number. Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  invalid object type specified.

\param[in] exoid      exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] obj_type   Type of object; use one of the options in the table below.
\param[in] prop_name  The name of the property for which the values will be stored. Maximum
                      length of this string is \p MAX_STR_LENGTH .
\param[in] values     An array of property values.

<table>
<tr><td> \c EX_NODE_SET   </td><td>  Node Set entity type     </td></tr>
<tr><td> \c EX_EDGE_BLOCK </td><td>  Edge Block entity type   </td></tr>
<tr><td> \c EX_EDGE_SET   </td><td>  Edge Set entity type     </td></tr>
<tr><td> \c EX_FACE_BLOCK </td><td>  Face Block entity type   </td></tr>
<tr><td> \c EX_FACE_SET   </td><td>  Face Set entity type     </td></tr>
<tr><td> \c EX_ELEM_BLOCK </td><td>  Element Block entity type</td></tr>
<tr><td> \c EX_ELEM_SET   </td><td>  Element Set entity type  </td></tr>
<tr><td> \c EX_SIDE_SET   </td><td>  Side Set entity type     </td></tr>
<tr><td> \c EX_ELEM_MAP   </td><td>  Element Map entity type  </td></tr>
<tr><td> \c EX_NODE_MAP   </td><td>  Node Map entity type     </td></tr>
<tr><td> \c EX_EDGE_MAP   </td><td>  Edge Map entity type     </td></tr>
<tr><td> \c EX_FACE_MAP   </td><td>  Face Map entity type     </td></tr>
</table>

For an example of code to write an array of object properties, refer
to the description for ex_put_prop_names().
 */

int ex_put_prop_array (int   exoid,
                       ex_entity_type obj_type,
                       const char *prop_name,
                       const void_int  *values)
{
  int oldfill = 0;
  int temp;
  int num_props, i, propid, dimid, dims[1], status;
  int found = EX_FALSE;
  int int_type;
  size_t num_obj; 
  char name[MAX_VAR_NAME_LENGTH+1];
  char tmpstr[MAX_STR_LENGTH+1];

  char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  /* check if property has already been created */

  num_props = ex_get_num_props(exoid, obj_type);

  /* inquire id of previously defined dimension (number of objects) */
  status = ex_get_dimension(exoid, ex_dim_num_objects(obj_type), ex_name_of_object(obj_type),
			    &num_obj, &dimid, "ex_put_prop_array");
  if (status != NC_NOERR) return status;

  for (i=1; i<=num_props; i++) {
    switch (obj_type){
    case EX_ELEM_BLOCK:
      strcpy (name, VAR_EB_PROP(i));
      break;
    case EX_FACE_BLOCK:
      strcpy (name, VAR_FA_PROP(i));
      break;
    case EX_EDGE_BLOCK:
      strcpy (name, VAR_ED_PROP(i));
      break;
    case EX_NODE_SET:
      strcpy (name, VAR_NS_PROP(i));
      break;
    case EX_EDGE_SET:
      strcpy (name, VAR_ES_PROP(i));
      break;
    case EX_FACE_SET:
      strcpy (name, VAR_FS_PROP(i));
      break;
    case EX_ELEM_SET:
      strcpy (name, VAR_ELS_PROP(i));
      break;
    case EX_SIDE_SET:
      strcpy (name, VAR_SS_PROP(i));
      break;
    case EX_ELEM_MAP:
      strcpy (name, VAR_EM_PROP(i));
      break;
    case EX_FACE_MAP:
      strcpy (name, VAR_FAM_PROP(i));
      break;
    case EX_EDGE_MAP:
      strcpy (name, VAR_EDM_PROP(i));
      break;
    case EX_NODE_MAP:
      strcpy (name, VAR_NM_PROP(i));
      break;
    default:
      exerrval = EX_BADPARAM;
      sprintf(errmsg, "Error: object type %d not supported; file id %d",
	      obj_type, exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      return(EX_FATAL);
    }

    if ((status = nc_inq_varid(exoid, name, &propid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to get property array id in file id %d",
	      exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      return (EX_FATAL);
    }

    /* compare stored attribute name with passed property name   */
    memset(tmpstr, 0, MAX_STR_LENGTH+1);
    if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, tmpstr)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get property name in file id %d", exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      return (EX_FATAL);
    }

    if (strcmp(tmpstr, prop_name) == 0) {
      found = EX_TRUE;
      break;
    }
  }

  /* if property array has not been created, create it */
  if (!found) {

    /* put netcdf file into define mode  */
    if ((status = nc_redef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      return (EX_FATAL);
    }

    /*   create a variable with a name xx_prop#, where # is the new number   */
    /*   of properties                                                       */
    switch (obj_type){
    case EX_ELEM_BLOCK:
      strcpy (name, VAR_EB_PROP(num_props+1));
      break;
    case EX_FACE_BLOCK:
      strcpy (name, VAR_FA_PROP(num_props+1));
      break;
    case EX_EDGE_BLOCK:
      strcpy (name, VAR_ED_PROP(num_props+1));
      break;
    case EX_NODE_SET:
      strcpy (name, VAR_NS_PROP(num_props+1));
      break;
    case EX_EDGE_SET:
      strcpy (name, VAR_ES_PROP(num_props+1));
      break;
    case EX_FACE_SET:
      strcpy (name, VAR_FS_PROP(num_props+1));
      break;
    case EX_ELEM_SET:
      strcpy (name, VAR_ELS_PROP(num_props+1));
      break;
    case EX_SIDE_SET:
      strcpy (name, VAR_SS_PROP(num_props+1));
      break;
    case EX_ELEM_MAP:
      strcpy (name, VAR_EM_PROP(num_props+1));
      break;
    case EX_FACE_MAP:
      strcpy (name, VAR_FAM_PROP(num_props+1));
      break;
    case EX_EDGE_MAP:
      strcpy (name, VAR_EDM_PROP(num_props+1));
      break;
    case EX_NODE_MAP:
      strcpy (name, VAR_NM_PROP(num_props+1));
      break;
    default:
      exerrval = EX_BADPARAM;
      sprintf(errmsg, "Error: object type %d not supported; file id %d",
	      obj_type, exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      goto error_ret;        /* Exit define mode and return */
    }

    dims[0] = dimid;
    nc_set_fill(exoid, NC_FILL, &oldfill); /* fill with zeros per routine spec */

    int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
      int_type = NC_INT64;
    }

    if ((status = nc_def_var(exoid, name, int_type, 1, dims, &propid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to create property array variable in file id %d",
	      exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }
    nc_set_fill(exoid, oldfill, &temp); /* default: nofill */


    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, propid, ATT_PROP_NAME, 
				  strlen(prop_name)+1, prop_name)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to store property name %s in file id %d",
	      prop_name,exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    /* leave define mode  */

    if ((status = nc_enddef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to leave define mode in file id %d",
	      exoid);
      ex_err("ex_put_prop_array",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* put num_obj values in property array */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_put_var_longlong(exoid, propid, values);
  } else {
    status = nc_put_var_int(exoid, propid, values);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to store property values in file id %d",
	    exoid);
    ex_err("ex_put_prop_array",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  nc_set_fill(exoid, oldfill, &temp); /* default: nofill */
  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
	    "Error: failed to complete definition for file id %d",
	    exoid);
    ex_err("ex_put_prop_array",errmsg,exerrval);
  }
  return (EX_FATAL);
}
