
/*************************************************************************************       
  HAIM: GLOBAL EIGENVALUE CALCULATIONS FOR MULTIGRID           
  GLOBAL BASIS (GB) METHOD / GENERALIZED GLOBAL BASIS (GGB) METHOD USING ARPACK                      
**************************************************************************************/     
/*                                                            
   THE IDEA:                                                  
        We want to solve     (STS)*x = lam*x                      
        S - Smoothing iteration matrix
	T - Multilevel projector
		
        + We solve the problem by supplying ARPACK with the function that
	  produces Matrix-vector product 
	+ We use simple non-symmetric ARPACK mode                            
                                                              
	*/                                                            

#define SHIFTS   0 
#define MAX_ITRS 2 
#define MODE     6 

#include <stdio.h>
#include <stdlib.h>
//#include "ml_struct.h" 
#include "ml_mat_formats.h" 
struct ML_Eigenvalue_Struct  {
  int Max_Iter;
  int Num_Eigenvalues;
  int Arnoldi;
  double Residual_Tol;
};


void  ML_ARPACK_driver(char which[],
			 char bmat[], int iparam[], int mode,
			 int nev, int ncv, double tol,  ML *ml,
			 struct ML_CSR_MSRdata  *mydata);


void ML_GGB_2_CSR (double **eigvec, int nconv, int MatSize, struct ML_CSR_MSRdata  *mydata);

extern void ML_ARPACK_GGB( 
		    struct ML_Eigenvalue_Struct *eigen_struct,ML *ml,
		    struct ML_CSR_MSRdata *mydata);


