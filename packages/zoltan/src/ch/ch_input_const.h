/*====================================================================
 * ------------------------
 * | CVS File Information |
 * ------------------------
 *
 * $RCSfile$
 *
 * $Author$
 *
 * $Date$
 *
 * $Revision$
 *
 *====================================================================*/

#ifndef __CH_INPUT_CONST_H
#define __CH_INPUT_CONST_H

#ifndef lint
static char *cvs_ch_input_const_h = "$Id$";
#endif

#include <mpi.h>
#include "dr_const.h"
#include "dr_input_const.h"

extern int chaco_input_graph(FILE *, char *, int **, int **, int *, int **, 
                             float **);

extern int chaco_input_geom(FILE *, char *, int, int *, float **, float **, 
                            float **);

extern int chaco_dist_graph(MPI_Comm, PARIO_INFO_PTR, 
                            int, int *, int *, int **, int **, 
                            int **, float **, 
                            int *, float **, float **, float **);

extern double read_val(FILE *, int *);
extern int read_int(FILE *, int *);

extern int CHECK_INPUT;

#ifndef TRUE
#define FALSE (0)
#define TRUE  (1)
#endif /* !TRUE */

#endif
