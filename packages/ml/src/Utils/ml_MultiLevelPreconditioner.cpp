/* Copyright (2001) Sandia Corportation. Under the terms of Contract 
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this 
 * work by or on behalf of the U.S. Government.  Export of this program
 * may require a license from the United States Government. */


/* NOTICE:  The United States Government is granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in ths data to reproduce, prepare derivative works, and
 * perform publicly and display publicly.  Beginning five (5) years from
 * July 25, 2001, the United States Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable
 * worldwide license in this data to reproduce, prepare derivative works,
 * distribute copies to the public, perform publicly and display
 * publicly, and to permit others to do so.
 * 
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
 * OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
 * ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
 * RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
 * INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
 * THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS. */

/*!
 *  \file ml_MultiLevelPreconditioner.cpp
 *
 *  \brief ML black-box preconditioner for Epetra_RowMatrix derived classes.
 *
 *  \author Marzio Sala, SNL, 9214
 *
 *  \date Last update to Doxygen: 07-Jul-2006
 *
 */

/* Notes on memory analyzer (MS):
 * - if mallinfo() is available, ML_MALLINFO is automatically defined
 *   by configure. This should be reasonably cheap and accurate.
 * - define ML_MEM_CHECK to an expensive analysis of memory corruption
 *   and memory leaks, for all blocks allocated using ML's allocating
 *   functions. Users have to set this variable explicitly (not handled by
 *   configure).
 * - define ML_MALLOC to estimate the maximum free memory by calling
 *   malloc(). This may be severely slow on some machine. Not defined by
 *   default. Users have to set this variable explicitly (not handled by
 *   configure). This option gives sometimes "strange" results, to be
 *   used only as last resort!
 */

/*#############################################################################
# CVS File Information
#    Current revision: $Revision$
#    Branch:           $Branch$
#    Last modified:    $Date$
#    Modified by:      $Author$
#############################################################################*/

#include "ml_common.h"
#include "ml_include.h"

#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS)
#include "ml_memory.h"
#include "ml_DD_prec.h"
#include <iostream>
#include <iomanip>

#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_FECrsMatrix.h"
#include "Epetra_VbrMatrix.h"
#include "Epetra_SerialDenseMatrix.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_SerialDenseSolver.h"
#include "Epetra_Import.h"
#include "Epetra_Export.h"
#include "Epetra_Time.h"
#include "Epetra_Operator.h"
#include "Epetra_RowMatrix.h"
#ifdef ML_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "ml_amesos_wrap.h"
#include "ml_agg_METIS.h"
#include "ml_epetra_utils.h"
#include "ml_epetra.h"
#include "ml_MultiLevelPreconditioner.h"
#include "ml_agg_ParMETIS.h"
#include "ml_anasazi.h"
#include "ml_FilterType.h"
#include "ml_ValidateParameters.h"

#ifdef HAVE_ML_EPETRAEXT
#include "EpetraExt_RowMatrixOut.h"
#endif

#include "ml_ifpack_wrap.h"

using namespace Teuchos;

// ================================================ ====== ==== ==== == =
void ML_Epetra::MultiLevelPreconditioner::PrintMem(char *fmt, int min, int sum, int max)
{

  if (Comm().MyPID() == 0) 
    printf(fmt,min,sum / Comm().NumProc(),max,sum);
  puts(" (Mb)");

  return;
}

// ================================================ ====== ==== ==== == =
int ML_Epetra::MultiLevelPreconditioner::DestroyPreconditioner()
{

  {
    int NumDestroy = OutputList_.get("number of destruction phases", 0);
    OutputList_.set("number of destruction phases", ++NumDestroy);
  }
  
  // may need to clean up after visualization and statistics
  if (List_.get("viz: enable", false) || List_.get("repartition: enable",0) ||
        (List_.get("aggregation: aux: enable", false) ) ) 
  {
    ML_Aggregate_VizAndStats_Clean(ml_);
    if (ml_nodes_ != 0) ML_Aggregate_VizAndStats_Clean(ml_nodes_);
  }

  // destroy aggregate information
  if ((agg_)->aggr_info != NULL) {
    for (int i = 0 ; i < NumLevels_ ; ++i) {
      if ((agg_)->aggr_info[i] != NULL) 
        ML_memory_free((void **)&((agg_)->aggr_info[i]));
    } 
  }
  // destroy main objects
  if (agg_ != 0) { ML_Aggregate_Destroy(&agg_); agg_ = 0; }
  if (ml_comm_ != 0) { ML_Comm_Destroy(&ml_comm_); ml_comm_ = 0; }

  if (TMatrixML_ != 0) {
    ML_Operator_Destroy(&TMatrixML_);
    TMatrixML_ = 0;
  }
  
  if (ML_Kn_ != 0) {
    ML_Operator_Destroy(&ML_Kn_);
    ML_Kn_ = 0;
  }

  if (TtATMatrixML_ != 0) {
    ML_Operator_Destroy(&TtATMatrixML_);
    TtATMatrixML_ = 0;
  }
  
  if (TMatrixTransposeML_ != 0) {
    ML_Operator_Destroy(&TMatrixTransposeML_);
    TMatrixTransposeML_ = 0;
  }

  if (Tmat_array != 0) {
    ML_MGHierarchy_ReitzingerDestroy(LevelID_[0]-1, &Tmat_array,
                     &Tmat_trans_array);
    Tmat_array = 0;
    Tmat_trans_array = 0;
  }

  if (nodal_args_ != 0) {
    ML_Smoother_Arglist_Delete(&nodal_args_);
    nodal_args_ = 0;
  }
  
  if (edge_args_  != 0) {
    ML_Smoother_Arglist_Delete(&edge_args_);
    edge_args_ = 0;
  }

  if (MassMatrix_array != 0) {
    for (int i=0; i<MaxLevels_; i++)
      ML_Operator_Destroy(MassMatrix_array+LevelID_[i]);
    delete [] MassMatrix_array;
    MassMatrix_array = 0;
  }

  if (CurlCurlMatrix_array != 0) {
    for (int i=0; i<MaxLevels_; i++)
      ML_Operator_Destroy(CurlCurlMatrix_array+LevelID_[i]);
    delete [] CurlCurlMatrix_array;
    CurlCurlMatrix_array = 0;
  }



  // These should be destroyed after all the individual operators, as the
  // communicator may be used in profiling some of the individual ML_Operators
  // in the hierarchy at when they are destroyed.
  if (ml_ != 0) { ML_Destroy(&ml_); ml_ = 0; }
  if (ml_nodes_ != 0) { ML_Destroy(&ml_nodes_); ml_nodes_ = 0; }

  if (Label_) { 
    delete [] Label_; 
    Label_ = 0; 
  }

  if (CreatedTMatrix_ == true) {
    delete TMatrix_;
    TMatrix_ = NULL;
  }

  if (CreatedML_Kn_ == true) {
    delete ML_Kn_;
    ML_Kn_ = NULL;
  }

  if (CreatedNodeMatrix_ == true) {
    delete NodeMatrix_;
    NodeMatrix_ = NULL;
  }

  if (CreatedEdgeMatrix_) {
    delete RowMatrix_;
    RowMatrix_ = NULL;
  }
  
  // stick data in OutputList. Note that ApplicationTime_ does not include the
  // time for the first application.

  OutputList_.set("time: total", FirstApplicationTime_+ApplicationTime_);

  OutputList_.set("time: first application", FirstApplicationTime_);

  OutputList_.set("time: construction", ConstructionTime_);

  OutputList_.set("number of applications", NumApplications_);

  int min[ML_MEM_SIZE], max[ML_MEM_SIZE], sum[ML_MEM_SIZE];
  for( int i=0 ; i<ML_MEM_SIZE ; ++i ) sum[i] = 0;

  if (AnalyzeMemory_) {
    memory_[ML_MEM_TOT2] = memory_[ML_MEM_TOT1] + memory_[ML_MEM_PREC_FIRST];
#ifdef ML_MALLOC
    memory_[ML_MEM_TOT2_MALLOC] = memory_[ML_MEM_TOT1_MALLOC] + memory_[ML_MEM_PREC_FIRST_MALLOC];
#endif
    Comm().MinAll(memory_,min,ML_MEM_SIZE);
    Comm().MaxAll(memory_,max,ML_MEM_SIZE);
    Comm().SumAll(memory_,sum,ML_MEM_SIZE);
  }
  
  if (verbose_ && NumApplications_) {

    // print on screen
    
    ML_print_line("-",78);
    double TotalTime = FirstApplicationTime_ + ApplicationTime_;
    std::cout << PrintMsg_ << "   ML time information                    total          avg" << std::endl << std::endl
     << PrintMsg_ << "   1- Construction time             = " 
         << std::setw(10) << ConstructionTime_ << "  "
         << std::setw(10) << ConstructionTime_ / NumConstructions_ << " (s)" << std::endl;
    std::cout << PrintMsg_ << "   2- Time for all applications     = " 
         << std::setw(10) << ApplicationTime_ << "  "
         << std::setw(10) << ApplicationTime_ / NumApplications_ << " (s)" << std::endl;
    std::cout << PrintMsg_ << "      (w/o first application time)" << std::endl;
    std::cout << PrintMsg_ << "   3- Time for first application(s) = " 
         << std::setw(10) << FirstApplicationTime_ << "  " 
         << std::setw(10) << FirstApplicationTime_ / NumConstructions_ << " (s)" << std::endl;
    std::cout << PrintMsg_ << "   4- Total time required by ML so far is " 
         << ConstructionTime_ + TotalTime << " (s)" << std::endl;
    std::cout << PrintMsg_ << "      (constr + all applications)" << std::endl;
  }

  if (verbose_ && AnalyzeMemory_) {
    
    // print memory usage

    std::cout << std::endl;
    std::cout << "   ML memory information:                 min     avg     max       tot" 
         << std::endl << std::endl;
#ifdef ML_MALLINFO    
    std::cout << "   1- estimated ML memory usage, using mallinfo()" << std::endl;
    PrintMem("      for the hierarchy              = %5d   %5d   %5d   %7d",
         min[ML_MEM_HIERARCHY],sum[ML_MEM_HIERARCHY],max[ML_MEM_HIERARCHY]);
    PrintMem("      for the smoother(s)            = %5d   %5d   %5d   %7d",
         min[ML_MEM_SMOOTHER],sum[ML_MEM_SMOOTHER],max[ML_MEM_SMOOTHER]);
    PrintMem("      for the coarse solver          = %5d   %5d   %5d   %7d",
         min[ML_MEM_COARSE],sum[ML_MEM_COARSE],max[ML_MEM_COARSE]);
    PrintMem("      preconditioning                = %5d   %5d   %5d   %7d",
         min[ML_MEM_PREC_FIRST],sum[ML_MEM_PREC_FIRST],max[ML_MEM_PREC_FIRST]);
    PrintMem("      total (w/o other prec data)    = %5d   %5d   %5d   %7d",
         min[ML_MEM_TOT1],sum[ML_MEM_TOT1],max[ML_MEM_TOT1]);
    PrintMem("      total (w/  other prec data)    = %5d   %5d   %5d   %7d",
         min[ML_MEM_TOT2],sum[ML_MEM_TOT2],max[ML_MEM_TOT2]);
    std::cout << std::endl;
#endif
#ifdef ML_MALLOC
    std::cout << "   3- estimated ML memory usage, using malloc()" << std::endl;
    PrintMem("      for the hierarchy              = %5d   %5d   %5d   %7d",
         min[ML_MEM_HIERARCHY_MALLOC],sum[ML_MEM_HIERARCHY_MALLOC],max[ML_MEM_HIERARCHY_MALLOC]);
    PrintMem("      for the smoother(s)            = %5d   %5d   %5d   %7d",
         min[ML_MEM_SMOOTHER_MALLOC],sum[ML_MEM_SMOOTHER_MALLOC],max[ML_MEM_SMOOTHER_MALLOC]);
    PrintMem("      for the coarse solver          = %5d   %5d   %5d   %7d",
         min[ML_MEM_COARSE_MALLOC],sum[ML_MEM_COARSE_MALLOC],max[ML_MEM_COARSE_MALLOC]);
    PrintMem("      preconditioning                = %5d   %5d   %5d   %7d",
         min[ML_MEM_PREC_FIRST_MALLOC],sum[ML_MEM_PREC_FIRST_MALLOC],max[ML_MEM_PREC_FIRST_MALLOC]);
    PrintMem("      total (w/o other prec data)    = %5d   %5d   %5d   %7d",
         min[ML_MEM_TOT1_MALLOC],sum[ML_MEM_TOT1_MALLOC],max[ML_MEM_TOT1_MALLOC]);
    PrintMem("      total (w/  other prec data)    = %5d   %5d   %5d   %7d",
         min[ML_MEM_TOT2_MALLOC],sum[ML_MEM_TOT2_MALLOC],max[ML_MEM_TOT2_MALLOC]);
    std::cout << std::endl;
#endif
    std::cout << "      (memory for Aztec smoothers reported in `preconditioning')" << std::endl;
    std::cout << "      (warning: data may be incorrect for small runs, or" << std::endl
     << "       if more processes share the same physical memory)" << std::endl
     << std::endl;
  }

  if (verbose_ && NumApplications_) 
    ML_print_line("-",78);
    
  if (NullSpaceToFree_ != 0) { 
    delete [] NullSpaceToFree_;  
    NullSpaceToFree_ = 0; 
  }
  
  if (RowMatrixAllocated_) { 
    delete RowMatrixAllocated_; 
    RowMatrixAllocated_ = 0; 
  }

  // filtering stuff

  if (flt_ml_) { 
    ML_Destroy(&flt_ml_); 
    flt_ml_ = 0; 
  }
  
  if (flt_agg_) { 
    ML_Aggregate_Destroy(&flt_agg_); 
    flt_agg_ = 0; 
  }
  
  IsComputePreconditionerOK_ = false;

#ifdef ML_MEM_CHECK
  // print out allocated memory. It should be zero.
  if( Comm().MyPID() == 0 ) 
    std::cout << PrintMsg_ << "Calling ML_Print_it()..." << std::endl
         << PrintMsg_ << "no memory should be allocated by the ML preconditioner" 
     << " at this point." << std::endl;
  ML_print_it();
#endif

  return 0;
}

// ================================================ ====== ==== ==== == =

ML_Epetra::MultiLevelPreconditioner::
MultiLevelPreconditioner(const Epetra_RowMatrix & RowMatrix,
             const bool ComputePrec) :
  RowMatrix_(&RowMatrix),
  RowMatrixAllocated_(0)
{

  ParameterList NewList;
  List_ = NewList;
  ML_Epetra::SetDefaults("SA",List_,(int *)0, (double *)0);
    
  ML_CHK_ERRV(Initialize());

  // construct hierarchy
  if (ComputePrec == true) 
    ML_CHK_ERRV(ComputePreconditioner());
}
  
// ================================================ ====== ==== ==== == =

ML_Epetra::MultiLevelPreconditioner::
MultiLevelPreconditioner( const Epetra_RowMatrix & RowMatrix,
             const ParameterList & List, const bool ComputePrec) :
  RowMatrix_(&RowMatrix),
  RowMatrixAllocated_(0)
{

  List_ = List;

  ML_CHK_ERRV(Initialize());

  // construct hierarchy
  if (ComputePrec == true) 
    ML_CHK_ERRV(ComputePreconditioner());
}

// ================================================ ====== ==== ==== == =

/*! The constructor for the Maxwell equations requires three Epetra_RowMatrices.
 * Two conditions are required on their maps:
 * - TMatrix.OperatorDomainMap() == NodeMatrix.OperatorRangeMap()
 * - TMatrix.OperatorRangeMap()  == EdgeMatrix.OperatorDomainMap()
 */
ML_Epetra::MultiLevelPreconditioner::
MultiLevelPreconditioner(const Epetra_RowMatrix & EdgeMatrix,
             const Epetra_RowMatrix & TMatrix,
             const Epetra_RowMatrix & NodeMatrix,
             const ParameterList & List,
             const bool ComputePrec,
             const bool UseNodeMatrixForSmoother):
  RowMatrix_(&EdgeMatrix),
  RowMatrixAllocated_(0)
{

  // some sanity checks
  if (! TMatrix.OperatorDomainMap().SameAs(NodeMatrix.OperatorRangeMap()) ) {
    std::cerr << ErrorMsg_ << "discrete grad DomainMap != node RangeMap..." << std::endl;
    ML_CHK_ERRV(-1); // error on discrete grad
  }

  if (! TMatrix.OperatorRangeMap().SameAs(EdgeMatrix.OperatorDomainMap()) ) {
    std::cerr << ErrorMsg_ << "discrete grad RangeMap != edge DomainMap..." <<std::endl;
    ML_CHK_ERRV(-2); // error on discrete grad
  }

  List_ = List;

  ML_CHK_ERRV(Initialize());

  // set Maxwell here.
  // NOTE: RowMatrix_ and EdgeMatrix_ pointer to the same Epetra_RowMatrix
  SolvingMaxwell_ = true;
  NodeMatrix_ = & NodeMatrix;
  TMatrix_ = & TMatrix;
  EdgeMatrix_ = & EdgeMatrix;
  UseNodeMatrixForSmoother_=UseNodeMatrixForSmoother;

  // construct hierarchy
  if (ComputePrec == true) 
    ML_CHK_ERRV(ComputePreconditioner());

}

// ================================================ ====== ==== ==== == =

/*! Constructor for the Maxwell equations.  This version takes the stiffness
 * and edge mass matrices separately.
 * Two conditions are required on their maps:
 * - TMatrix.OperatorDomainMap() == NodeMatrix.OperatorRangeMap()
 * - TMatrix.OperatorRangeMap()  == EdgeMatrix.OperatorDomainMap()
 */
ML_Epetra::MultiLevelPreconditioner::
MultiLevelPreconditioner(const Epetra_RowMatrix & CurlCurlMatrix,
                         const Epetra_RowMatrix & MassMatrix,
                         const Epetra_RowMatrix & TMatrix,
                         const Epetra_RowMatrix & NodeMatrix,
                         const ParameterList & List,
                         const bool ComputePrec) :
  RowMatrixAllocated_(0)
{

  // Check compatibility of CurlCurl and  TMatrix.
  if (! TMatrix.OperatorDomainMap().SameAs(NodeMatrix.OperatorRangeMap()) ) {
    std::cerr << ErrorMsg_ << "discrete grad DomainMap != node RangeMap..." << std::endl;
    ML_CHK_ERRV(-1); // error on discrete grad
  }

  if (! TMatrix.OperatorRangeMap().SameAs(CurlCurlMatrix.OperatorDomainMap())) {
    std::cerr << ErrorMsg_ << "discrete grad RangeMap != edge DomainMap..." <<std::endl;
    ML_CHK_ERRV(-2); // error on discrete grad
  }

  List_ = List;

  // later on, this will be reset to the sum of the curl-curl and mass
  RowMatrix_ = & CurlCurlMatrix;

  ML_CHK_ERRV(Initialize());

  SolvingMaxwell_ = true;
  EdgeMatrix_ = RowMatrix_;
  CurlCurlMatrix_ = & CurlCurlMatrix;
  MassMatrix_ = & MassMatrix;
  NodeMatrix_ = & NodeMatrix;
  TMatrix_ = & TMatrix;

  // construct hierarchy
  if (ComputePrec == true) 
    ML_CHK_ERRV(ComputePreconditioner());

}

// ================================================ ====== ==== ==== == =

#ifdef HAVE_ML_AZTECOO
/*! Another constructor for Maxwell equations that takes an Epetra_MsrMatrix,
 * an ML_Operator, and an AZ_MATRIX type.  The Epetra_MsrMatrix type is defined
 * in aztecoo.
 * Two conditions are required on their maps:
 * - TMatrix.OperatorDomainMap() == NodeMatrix.OperatorRangeMap()
 * - TMatrix.OperatorRangeMap()  == EdgeMatrix.OperatorDomainMap()
 */
ML_Epetra::MultiLevelPreconditioner::
MultiLevelPreconditioner(const Epetra_MsrMatrix & EdgeMatrix,
                         ML_Operator * ML_TMatrix,
                         AZ_MATRIX * AZ_NodeMatrix,
                         int       * proc_config,
                         const ParameterList & List,
                         const bool ComputePrec):
  RowMatrix_(&EdgeMatrix),
  RowMatrixAllocated_(0)
{
  ML_Comm *ml_comm;
  ML_Operator *loc_ML_Kn;

  int MaxNumNonzeros;
  double CPUTime;
  Epetra_CrsMatrix *NodeMatrix, *TMatrix;

  //next-to-last argument must be false because T may have zero rows due
  //to Dirichlet b.c.'s.
  ML_Operator2EpetraCrsMatrix(ML_TMatrix,TMatrix,MaxNumNonzeros,
                  false,CPUTime);

  ML_Comm_Create(&ml_comm);
#ifdef ML_MPI
  const Epetra_MpiComm *epcomm = dynamic_cast<const Epetra_MpiComm*>(&(EdgeMatrix.Comm()));
  // Get the MPI communicator, as it may not be MPI_COMM_W0RLD, and update the ML comm object
  if (epcomm) ML_Comm_Set_UsrComm(ml_comm,epcomm->Comm());
#endif
  loc_ML_Kn = ML_Operator_Create(ml_comm);
  AZ_convert_aztec_matrix_2ml_matrix(AZ_NodeMatrix,loc_ML_Kn,proc_config);
  ML_Operator2EpetraCrsMatrix(loc_ML_Kn, NodeMatrix,MaxNumNonzeros,
                  false,CPUTime);

  // some sanity checks
  if (! TMatrix->OperatorDomainMap().SameAs(NodeMatrix->OperatorRangeMap()) ) {
    std::cerr << ErrorMsg_ << "discrete grad DomainMap != node RangeMap..." << std::endl;
    ML_CHK_ERRV(-1); // error on discrete grad
  }

  if (! TMatrix->OperatorRangeMap().SameAs(EdgeMatrix.OperatorDomainMap()) ) {
    std::cerr << ErrorMsg_ << "discrete grad RangeMap != edge DomainMap..." <<std::endl;
    ML_CHK_ERRV(-2); // error on discrete grad
  }

  List_ = List;

  ML_CHK_ERRV(Initialize());

  // set Maxwell here.
  // NOTE: RowMatrix_ and EdgeMatrix_ pointer to the same Epetra_RowMatrix
  SolvingMaxwell_ = true;
  NodeMatrix_ = NodeMatrix;
  CreatedNodeMatrix_ = true;
  TMatrix_ = TMatrix;
  CreatedTMatrix_ = true;
  EdgeMatrix_ = & EdgeMatrix;
  ML_Kn_ = loc_ML_Kn;
  CreatedML_Kn_ = true;

  // construct hierarchy
  if (ComputePrec == true) 
    ML_CHK_ERRV(ComputePreconditioner());

  ML_Comm_Destroy(&ml_comm);
}
#endif

// ================================================ ====== ==== ==== == =
// FIXME: should I be deleted??
ML_Epetra::MultiLevelPreconditioner::
MultiLevelPreconditioner(ML_Operator * Operator,
             const ParameterList & List, const bool ComputePrec)
{

  // need to wrap an Epetra_RowMatrix around Operator.
  // This is quite not the best approach for small matrices

  int MaxNumNonzeros;
  double CPUTime;
  
  // FIXME: substitute with a better version
  ML_Operator2EpetraCrsMatrix(Operator,RowMatrixAllocated_,MaxNumNonzeros,
                  true,CPUTime);

  // this matrix must be freed by dtor. Keep trace of it in this pointer
  RowMatrix_ = RowMatrixAllocated_;

  // from now on as for the other constructors
  List_ = List;

  ML_CHK_ERRV(Initialize());

  // construct hierarchy
  if (ComputePrec == true) 
    ML_CHK_ERRV(ComputePreconditioner());
}

// ================================================ ====== ==== ==== == =
/*! - set to 0 all allocatable pointers
 *  - put default values in Aztec vectors.
 *  - zero-out timing
 */
int ML_Epetra::MultiLevelPreconditioner::Initialize()
{

  Comm_ = &(RowMatrix_->Comm());
  DomainMap_ = &(RowMatrix_->OperatorDomainMap());
  RangeMap_ = &(RowMatrix_->OperatorRangeMap());

  verbose_ = false;
  MaxLevels_ = 20;
  IsComputePreconditionerOK_ = false;

  NumPDEEqns_ = 1;
  
  NullSpaceToFree_ = 0;

  Label_ = 0;

  ml_ = 0;
  agg_ = 0;
  
  sprintf(ErrorMsg_,"%s","*ML*ERR* : ");
  PrintMsg_ = "";
  
#ifdef HAVE_ML_AZTECOO
  AZ_defaults(SmootherOptions_,SmootherParams_);
  SmootherOptions_[AZ_precond] = AZ_dom_decomp;
  SmootherOptions_[AZ_subdomain_solve] = AZ_ilu;
  SmootherOptions_[AZ_overlap] = 0;
#endif

  // Maxwell stuff is off by default
  SolvingMaxwell_ = false;
  NodeMatrix_ = 0;
  CreatedNodeMatrix_ = false;
  ML_Kn_ = 0;
  CreatedML_Kn_ = false;
  EdgeMatrix_ = 0;
  CurlCurlMatrix_ = 0;
  CreatedEdgeMatrix_ = false;
  MassMatrix_ = 0;
  TMatrix_ = 0;  
  TtATMatrixML_=0;
  UseNodeMatrixForSmoother_=false;
  ml_nodes_ = 0;
  TMatrixML_ = 0;
  CreatedTMatrix_ = false;
  TMatrixTransposeML_ = 0;
  Tmat_array = 0;
  Tmat_trans_array = 0;
  nodal_args_ = 0;
  edge_args_ = 0;
  MassMatrix_array = 0;
  CurlCurlMatrix_array = 0;
  
  // timing
  NumApplications_ = 0;
  ApplicationTime_ = 0.0;
  FirstApplication_ = true;
  FirstApplicationTime_ = 0.0;
  NumConstructions_ = 0;
  ConstructionTime_ = 0.0;
  
  // some tracking here
  int NumInitializations = OutputList_.get("number of initialization phases", 0);
  OutputList_.set("number of initialization phases", ++NumInitializations);

  // memory
  AnalyzeMemory_ = false;

  for (int i = 0 ; i < ML_MEM_SIZE ; ++i) 
    memory_[i] = 0;

  // filtering vectors
  flt_ml_ = 0;
  flt_agg_ = 0;

  // CheckPreconditioner stuff
  RateOfConvergence_ = -1.0;

  return 0;
}

// ================================================ ====== ==== ==== == =

int ML_Epetra::MultiLevelPreconditioner::
ComputePreconditioner(const bool CheckPreconditioner)
{
  
 try {

  if (List_.get("ML debug mode", false))
    ML_BreakForDebugger(*Comm_);

  if (List_.get("ML print parameter list",false))
    PrintList(0);

  // Validate Parameter List
  if(List_.get("ML validate parameter list",true)
     && !ValidateMLPParameters(List_))
  {
    if (Comm_->MyPID() == 0)
      std::cout<<"ERROR: ML's Teuchos::ParameterList contains an incorrect parameter!"<<std::endl;
#   ifdef HAVE_MPI
    MPI_Finalize();
#   endif
    exit(1);
  }

  // ============================================================== //
  // check whether the old filtering is still ok for the new matrix //
  // ============================================================== //

  if (CheckPreconditioner == true && RateOfConvergence_ != -1.0) {

    // If the previous preconditioner was computed with option
    // "reuse: enable" == true, we know the rate of convergence
    // with the previous matrix (and this preconditioner. Now, 
    // we recompute this ratio, and compare it with the previous one.
    // This requires an AztecOO object to be defined (ML must have been
    // configured with --enable-aztecoo)

    if (CheckPreconditionerKrylov() == false) {
      ML_CHK_ERR(DestroyPreconditioner());
    }
    else                                       
      return 0;
    
  } else if (CheckPreconditioner == false && 
             IsComputePreconditionerOK_ == true) {
  
    // get rid of what done before 
    ML_CHK_ERR(DestroyPreconditioner());
    
  } // nothing else if left

  // ======================== //
  // build the preconditioner //
  // ======================== //

  Epetra_Time Time(Comm());
  Epetra_Time InitialTime(Comm());
  {
    int NumCompute = OutputList_.get("number of construction phases", 0);
    OutputList_.set("number of construction phases", ++NumCompute);
  }
  
#ifdef HAVE_ML_AZTECOO
#ifdef HAVE_MPI
  const Epetra_MpiComm * MpiComm = dynamic_cast<const Epetra_MpiComm*>(&Comm());
  if (!MpiComm)
  {
     std::cerr << "dynamic_cast of Comm() failed\n";
     exit( EXIT_FAILURE );
  }
  AZ_set_proc_config(ProcConfig_,MpiComm->Comm());
#else
  AZ_set_proc_config(ProcConfig_,AZ_NOT_MPI);
#endif
#endif

  // user's defined output message
  PrintMsg_ = List_.get("output prefix",PrintMsg_);
  
  NumLevels_ = List_.get("max levels",10);  
  MaxLevels_ = NumLevels_;

  // 'ML output' replaces just 'output' but for backward compatibility
  // we still allow just 'output' if 'ML output' is not set.
  int OutputLevel = List_.get("ML output", -47);  
  if (OutputLevel == -47) OutputLevel = List_.get("output", 0);  
  ML_Set_PrintLevel(OutputLevel);

  verbose_ = ( (5 < ML_Get_PrintLevel()) && (Comm().MyPID() == 0) );
  
  if( verbose_ ) 
    ML_print_line("-",78);
  
  // check for an XML input file
  std::string XMLFileName = List_.get("XML input file","ml_ParameterList.xml");
  ReadXML(XMLFileName);

  FirstApplication_ = true;

  int call1 = 0, call2 = 0; 
#ifdef ML_MALLOC
  int call1_malloc = 0, call2_malloc = 0;
#endif

  AnalyzeMemory_ = List_.get("analyze memory", false);  

  if( AnalyzeMemory_ ) {
    memory_[ML_MEM_INITIAL] = ML_MaxMemorySize();
    call1 = memory_[ML_MEM_INITIAL];
#ifdef ML_MALLOC
    call1_malloc = ML_MaxAllocatableSize();
    memory_[ML_MEM_INITIAL_MALLOC] = call1_malloc;
    if (verbose_) std::cout << "Memory : max allocatable block = " << call1_malloc << " Mbytes" << std::endl;
#endif
  }

  if (Label_) delete[] Label_;
  
  Label_ = new char[80];
  
  // number of applications of the cycle
  CycleApplications_ = List_.get("cycle applications", 1);  

  // toggles the use of low memory
  if (List_.get("low memory usage", false))
    ML_Enable_LowMemory();
  else
    ML_Disable_LowMemory();

  // number of applications of the cycle
  ZeroStartingSolution_ = List_.get("zero starting solution", true);  

  // compute how to traverse levels (increasing of descreasing)
  // By default, use ML_INCREASING.
  
  std::string IsIncreasing = List_.get("increasing or decreasing", "increasing");

  if( SolvingMaxwell_ == true ) IsIncreasing = "decreasing";
  
  int FinestLevel;

  LevelID_.resize(NumLevels_);
  if( IsIncreasing == "increasing" ) {
    FinestLevel = 0;
    for( int i=0 ; i<NumLevels_ ; ++i ) LevelID_[i] = FinestLevel+i;
  } else {
    FinestLevel = NumLevels_-1;
    for( int i=0 ; i<NumLevels_ ; ++i ) LevelID_[i] = FinestLevel-i;
  }
  
  // check no levels are negative
  for (int i = 0 ; i < NumLevels_ ; ++i)
    if (LevelID_[i] < 0) {
      std::cerr << ErrorMsg_ << "Level " << i << " has a negative ID" << std::endl;
      ML_EXIT(EXIT_FAILURE);
    }  

  if (verbose_) {
    std::cout << PrintMsg_ << "*** " << std::endl;
    std::cout << PrintMsg_ << "*** ML_Epetra::MultiLevelPreconditioner" << std::endl;
    std::cout << PrintMsg_ << "***" << std::endl;
    std::cout << PrintMsg_ << "Matrix has " << RowMatrix_->NumGlobalRows()
     << " rows and " << RowMatrix_->NumGlobalNonzeros() 
         << " nonzeros, distributed over " << Comm().NumProc() << " process(es)" << std::endl;
    {
      const Epetra_CrsMatrix * dummy = dynamic_cast<const Epetra_CrsMatrix *>(RowMatrix_);
      if( dummy ) std::cout << "The linear system matrix is an Epetra_CrsMatrix" << std::endl;
    }    
    {
      const Epetra_VbrMatrix * dummy = dynamic_cast<const Epetra_VbrMatrix *>(RowMatrix_);
      if( dummy ) std::cout << "The linear system matrix is an Epetra_VbrMatrix" << std::endl;
    }    
    
    if( List_.isParameter("default values") ){
      std::cout << PrintMsg_ << "Default values for `" << List_.get("default values", "DD") << "'" << std::endl;
    }
    std::cout << PrintMsg_ << "Maximum number of levels = " << NumLevels_ << std::endl;
    if( IsIncreasing == "increasing" ) std::cout << PrintMsg_ << "Using increasing levels. ";
    else                               std::cout << PrintMsg_ << "Using decreasing levels. ";
    std::cout << "Finest level  = " << LevelID_[0];
    std::cout << ", coarsest level = " << LevelID_[NumLevels_-1] << std::endl;
    std::cout << "Number of applications of the ML cycle = " << CycleApplications_ << std::endl;
  }

  const Epetra_VbrMatrix* VbrMatrix;
  VbrMatrix = dynamic_cast<const Epetra_VbrMatrix *>(RowMatrix_);
  if (VbrMatrix == 0) {
    NumPDEEqns_ = List_.get("PDE equations", 1);
  }
  else {
    int NumBlockRows = VbrMatrix->RowMap().NumGlobalElements();
    int NumRows = VbrMatrix->RowMap().NumGlobalPoints();
    if( NumRows % NumBlockRows ) {
      std::cerr << "# rows must be a multiple of # block rows ("
       << NumRows << "," << NumBlockRows << ")" << std::endl;
      exit( EXIT_FAILURE );
    }
    NumPDEEqns_ = NumRows/NumBlockRows;
  }

  if( verbose_ ) std::cout << "Number of PDE equations = " << NumPDEEqns_ << std::endl;
  
  int MaxCreationLevels = NumLevels_;
  if( IsIncreasing == "decreasing" )  MaxCreationLevels = FinestLevel+1;
  
  ML_Aggregate_Create(&agg_);
  ML_Comm_Create(&ml_comm_);
#ifdef EMIN_IN_PAPER
printf("this is garbage for NSR \n");
ML_Aggregate_Set_Reuse(agg_);
agg_->keep_agg_information = 1;
agg_->keep_P_tentative = 1;
#endif


  if( SolvingMaxwell_ == false ) {

    // ====================================================================== //
    // create hierarchy for classical equations (all but Maxwell)             //
    // ====================================================================== //

    // tell ML to keep the tentative prolongator
    // in the case ReComputePreconditioner() or auxiliary matrix
    // are used.
    ML_Aggregate_Set_Reuse(agg_);
    agg_->keep_agg_information = 1;

    if (agg_->aggr_info != NULL) {
      for (int i = 0 ; i < MaxLevels_ ; ++i)
        agg_->aggr_info[i] = NULL;
    }

    ML_Create(&ml_,MaxCreationLevels);
    ml_->comm->ML_nprocs = Comm().NumProc();
    ml_->comm->ML_mypid = Comm().MyPID();
#ifdef ML_MPI
    const Epetra_MpiComm* C2 = dynamic_cast<const Epetra_MpiComm*>(&Comm());
    ml_->comm->USR_comm = C2->Comm();
#endif
    ml_->output_level = OutputLevel;

    
    

#ifdef HAVE_ML_EPETRAEXT
    RowMatrix_ = ModifyEpetraMatrixColMap(*RowMatrix_,RowMatrixColMapTrans_,
                                          "Main linear system");
#endif
    int NumMyRows;
    NumMyRows = RowMatrix_->NumMyRows();
    int N_ghost = RowMatrix_->NumMyCols() - NumMyRows;
    
    if (N_ghost < 0) N_ghost = 0;  // A->NumMyCols() = 0 for an empty matrix
    
    const Epetra_VbrMatrix *Avbr =
              dynamic_cast<const Epetra_VbrMatrix *>(RowMatrix_);
    const Epetra_CrsMatrix *Acrs =
              dynamic_cast<const Epetra_CrsMatrix *>(RowMatrix_);
    if (Avbr)
    {
      ML_Init_Amatrix(ml_,LevelID_[0],NumMyRows, NumMyRows, (void *) Avbr);
      ml_->Amat[LevelID_[0]].type = ML_TYPE_VBR_MATRIX;
    }
    else if (Acrs)
    {
      ML_Init_Amatrix(ml_,LevelID_[0],NumMyRows, NumMyRows, (void *) Acrs);
      ml_->Amat[LevelID_[0]].type = ML_TYPE_CRS_MATRIX;
    }
    else
    {
      ML_Init_Amatrix(ml_,LevelID_[0],NumMyRows, NumMyRows,(void *) RowMatrix_);
      ml_->Amat[LevelID_[0]].type = ML_TYPE_ROW_MATRIX;
    }
    // set the number of nonzeros
    ml_->Amat[LevelID_[0]].N_nonzeros = RowMatrix_->NumGlobalNonzeros();

    // ============ //
    // fix diagonal //
    // ============ //

    if (false) // ?? MS ?? what the hell I did here?
    {
      std::cout << "Setting diagonal..." << std::endl;
      double* diagonal = (double*)ML_allocate(sizeof(double) * NumMyRows);
      Epetra_Vector Diagonal(View, RowMatrix_->RowMatrixRowMap(), diagonal);
      RowMatrix_->ExtractDiagonalCopy(Diagonal);

      ML_Operator_Set_Diag(&(ml_->Amat[LevelID_[0]]), NumMyRows, diagonal);
    }

    ML_Epetra::FilterType FT;
    FT = List_.get("filter: type", ML_Epetra::ML_NO_FILTER);

    double* Mask = 0;
    Mask = List_.get("filter: mask", Mask);

    if (FT != ML_Epetra::ML_NO_FILTER) {
      if (verbose_) {
        std::cout << std::endl;
        std::cout << PrintMsg_ << "Using filtering, type = ";
        switch (FT) {
        case ML_NO_FILTER:
          // cannot be
          std::cout << "ML_NO_FILTER" << std::endl;
          break;
        case ML_EQN_FILTER:
          std::cout << "ML_EQN_FILTER" << std::endl;
          break;
        case ML_THREE_BLOCKS_FILTER:
          std::cout << "ML_THREE_BLOCKS_FILTER" << std::endl;
          break;
        case ML_TWO_BLOCKS_FILTER:
          std::cout << "ML_TWO_BLOCKS_FILTER" << std::endl;
          break;
        case ML_MASK_FILTER:
          std::cout << "ML_MASK_FILTER" << std::endl;
          break;
        }

        std::cout << PrintMsg_ << "Threshold = "
          << List_.get("filter: absolute threshold", 0.0) 
          << " (absolute), "
          << List_.get("filter: relative threshold", 1.0) 
          << " (relative)" << std::endl;

        if (FT == ML_TWO_BLOCKS_FILTER || FT == ML_THREE_BLOCKS_FILTER) {
          std::cout << PrintMsg_ << "Dividers = "
               << List_.get("filter: first divider", 0)
               << " (first), " 
               << List_.get("filter: second divider", 0)
               << " (second), " << std::endl;
        }

        if (FT == ML_MASK_FILTER) {
          std::cout << "Mask is as follows:" << std::endl;
          for (int i = 0 ; i < NumPDEEqns_ ; ++i) {
            for (int j = 0 ; j < NumPDEEqns_ ; ++j) {
              if (Mask[i * NumPDEEqns_ + j] == 1)
                std::cout << " * ";
              else
                std::cout << " . ";
            }
            std::cout << std::endl;
          }
        }
        std::cout << std::endl;

      }
      List_.set("filter: equations", NumPDEEqns_);

      ML_Set_Filter(List_);
      // FIXME // not so sure that ML_Epetra_matvec_Filter is not
      // FIXME // more appropriate
      ML_Set_Amatrix_Getrow(ml_, LevelID_[0], ML_Epetra_getrow_Filter,
                            ML_Epetra_comm_wrapper, NumMyRows+N_ghost);
    
      ML_Set_Amatrix_Matvec(ml_, LevelID_[0], ML_Epetra_matvec_Filter);

    }
    else {
      if (Avbr) {
        // check that the number of PDEs is constant
        if( Avbr->NumMyRows() % Avbr->NumMyBlockRows() != 0 ){
          std::cerr << "Error : NumPDEEqns does not seem to be constant" << std::endl;
          ML_CHK_ERR(-1);
        }
        ML_Set_Amatrix_Getrow(ml_, LevelID_[0], ML_Epetra_VbrMatrix_getrow,
                              ML_Epetra_VbrMatrix_comm_wrapper,
                              NumMyRows+N_ghost);

        ML_Set_Amatrix_Matvec(ml_, LevelID_[0], ML_Epetra_VbrMatrix_matvec);
      }
      else if (Acrs) {
        ML_Set_Amatrix_Getrow(ml_, LevelID_[0], ML_Epetra_CrsMatrix_getrow,
                              ML_Epetra_CrsMatrix_comm_wrapper,
                              NumMyRows+N_ghost);

        ML_Set_Amatrix_Matvec(ml_, LevelID_[0], ML_Epetra_CrsMatrix_matvec);
      }
      else {
        ML_Set_Amatrix_Getrow(ml_, LevelID_[0], ML_Epetra_RowMatrix_getrow,
                              ML_Epetra_comm_wrapper, NumMyRows+N_ghost);

        ML_Set_Amatrix_Matvec(ml_, LevelID_[0], ML_Epetra_matvec);
      }
    }

    // ========================================= //
    // repartition of matrices                   //
    // ========================================= //
    
    if (List_.get("repartition: enable",0))
    {
      ML_Repartition_Activate(ml_);

      std::string Repartitioner = List_.get("repartition: partitioner","Zoltan");

      double minmax = List_.get("repartition: max min ratio", 1.3);
      ML_Repartition_Set_LargestMinMaxRatio(ml_,minmax);
      int minperproc = List_.get("repartition: min per proc", 512);
      ML_Repartition_Set_MinPerProc(ml_,minperproc);

      if (Repartitioner == "Zoltan") {
        ML_Repartition_Set_Partitioner(ml_,ML_USEZOLTAN);
        int NumDimensions = List_.get("repartition: Zoltan dimensions", 0);
        ML_Aggregate_Set_Dimensions(agg_, NumDimensions);
      }
      else if (Repartitioner == "ParMETIS") {
        ML_Repartition_Set_Partitioner(ml_,ML_USEPARMETIS);
      }
    }

  } else {

    // ====================================================================== //
    // create hierarchy for Maxwell. Need to define ml_ and ml_nodes_         //
    // The only way to activate SolvingMaxwell_ == true is through the        //
    // constructor for Maxwell. I suppose that the matrices are called not    //
    // Matrix_, but moreover NodeMatrix_ and EdgeMatrix_.                     //
    // ====================================================================== //

    if( verbose_ ) std::cout << PrintMsg_ << "Solving Maxwell Equations..." << std::endl;

    int NumMyRows, N_ghost;
    
    // create hierarchy for edges

    ML_Create(&ml_,MaxCreationLevels);
    ML_Set_Label(ml_, "edges");
    int Direction;
    if (IsIncreasing == "increasing")
      Direction = ML_INCREASING;
    else
      Direction = ML_DECREASING;
    ML_Set_LevelID(ml_,Direction);

    ml_->output_level = OutputLevel;
    
    // if curl-curl and mass matrix were given separately, add them
    if (CurlCurlMatrix_) {
#ifdef HAVE_ML_EPETRAEXT
      CurlCurlMatrix_ = ModifyEpetraMatrixColMap(*CurlCurlMatrix_,
                                                CurlCurlMatrixColMapTrans_,
                                                "(curl,curl)");
      MassMatrix_ = ModifyEpetraMatrixColMap(*MassMatrix_,
                                             MassMatrixColMapTrans_,"mass");
#endif
      Epetra_RowMatrix *cc = const_cast<Epetra_RowMatrix*>(CurlCurlMatrix_);
      Epetra_RowMatrix *mm = const_cast<Epetra_RowMatrix*>(MassMatrix_);
      RowMatrix_ = Epetra_MatrixAdd(cc,mm,1.0);
      CreatedEdgeMatrix_ = true;
    }

#ifdef HAVE_ML_EPETRAEXT
    EdgeMatrix_ = ModifyEpetraMatrixColMap(*RowMatrix_,
                                           RowMatrixColMapTrans_,
                                           "edge element matrix");
    /* Handle the Nodal matrix for Hiptmair, if we're using UseNodMatrixForSmoother */
    if(UseNodeMatrixForSmoother_){
      NodeMatrix_= ModifyEpetraMatrixColMap(*NodeMatrix_,TtATMatrixColMapTrans_,"node matrix");
      Epetra_CrsMatrix * TtATCrsMatrix_ = dynamic_cast<Epetra_CrsMatrix*>((Epetra_RowMatrix*)NodeMatrix_);
      TtATMatrixML_ = ML_Operator_Create(ml_comm_);
      if(TtATCrsMatrix_) ML_Operator_WrapEpetraCrsMatrix(TtATCrsMatrix_,TtATMatrixML_,verbose_);
      else ML_Operator_WrapEpetraMatrix((Epetra_RowMatrix*)NodeMatrix_,TtATMatrixML_);
    }

    
#endif
    RowMatrix_ = EdgeMatrix_;
    NumMyRows = EdgeMatrix_->NumMyRows();
    N_ghost   = EdgeMatrix_->NumMyCols() - NumMyRows;
    
    if (N_ghost < 0) N_ghost = 0;  // A->NumMyCols() = 0 for an empty matrix
    
    const Epetra_CrsMatrix *Acrs = dynamic_cast<const Epetra_CrsMatrix*>(EdgeMatrix_);

    if (Acrs != 0) {
      ML_Init_Amatrix(ml_,LevelID_[0],NumMyRows, NumMyRows,
                      (void *) (const_cast<Epetra_CrsMatrix*>(Acrs)));
      ML_Set_Amatrix_Getrow(ml_, LevelID_[0], ML_Epetra_CrsMatrix_getrow,
              ML_Epetra_CrsMatrix_comm_wrapper, NumMyRows+N_ghost);
      ML_Set_Amatrix_Matvec(ml_, LevelID_[0], ML_Epetra_CrsMatrix_matvec);
      ml_->Amat[LevelID_[0]].type = ML_TYPE_CRS_MATRIX;
    }
    else {
      ML_Init_Amatrix(ml_,LevelID_[0],NumMyRows,NumMyRows,(void *) EdgeMatrix_);
      ML_Set_Amatrix_Getrow(ml_, LevelID_[0], ML_Epetra_getrow,
              ML_Epetra_comm_wrapper, NumMyRows+N_ghost);
      ML_Set_Amatrix_Matvec(ml_, LevelID_[0], ML_Epetra_matvec);
      ml_->Amat[LevelID_[0]].type = ML_TYPE_ROW_MATRIX;
    }

    // create hierarchy for nodes
    
    ML_Create(&ml_nodes_,MaxCreationLevels);
    ML_Set_Label(ml_nodes_, "nodes");
#ifdef HAVE_ML_EPETRAEXT
    NodeMatrix_ = ModifyEpetraMatrixColMap(*NodeMatrix_,
                                           NodeMatrixColMapTrans_,"Node");
#endif
    NumMyRows = NodeMatrix_->NumMyRows();
    N_ghost   = NodeMatrix_->NumMyCols() - NumMyRows;
    
    if (N_ghost < 0) N_ghost = 0;  // A->NumMyCols() = 0 for an empty matrix

    Acrs = dynamic_cast<const Epetra_CrsMatrix*>(NodeMatrix_);

    if (Acrs != 0) {
      ML_Init_Amatrix(ml_nodes_,LevelID_[0],NumMyRows,NumMyRows,
                      (void *) (const_cast<Epetra_CrsMatrix*>(Acrs)));
      ML_Set_Amatrix_Getrow(ml_nodes_, LevelID_[0], ML_Epetra_CrsMatrix_getrow,
                ML_Epetra_CrsMatrix_comm_wrapper, NumMyRows+N_ghost);
      ML_Set_Amatrix_Matvec(ml_nodes_, LevelID_[0], ML_Epetra_CrsMatrix_matvec);
      ml_nodes_->Amat[LevelID_[0]].type = ML_TYPE_CRS_MATRIX;
    }
    else {
      ML_Init_Amatrix(ml_nodes_,LevelID_[0],NumMyRows, NumMyRows,
                      (void *) NodeMatrix_);
      ML_Set_Amatrix_Getrow(ml_nodes_, LevelID_[0], ML_Epetra_getrow,
                ML_Epetra_comm_wrapper, NumMyRows+N_ghost);
      ML_Set_Amatrix_Matvec(ml_nodes_, LevelID_[0], ML_Epetra_matvec);
      ml_nodes_->Amat[LevelID_[0]].type = ML_TYPE_ROW_MATRIX;
    }
    ml_nodes_->Amat[LevelID_[0]].N_nonzeros = NodeMatrix_->NumMyNonzeros();

    // check whether coarse grid operators should be repartitioned among
    // processors
    int ShouldRepartition = List_.get("repartition: enable",0);

    if (ShouldRepartition) {

      std::string Repartitioner = List_.get("repartition: partitioner","Zoltan");

      ML_Repartition_Activate(ml_);
      ML_Repartition_Activate(ml_nodes_);

      double minmax = List_.get("repartition: node max min ratio", 1.3);
      ML_Repartition_Set_LargestMinMaxRatio(ml_nodes_,minmax);
      int minperproc = List_.get("repartition: node min per proc", 170);
      ML_Repartition_Set_MinPerProc(ml_nodes_,minperproc);

      minmax = List_.get("repartition: max min ratio", 1.3);
      ML_Repartition_Set_LargestMinMaxRatio(ml_,minmax);
      minperproc = List_.get("repartition: min per proc", 512);
      ML_Repartition_Set_MinPerProc(ml_,minperproc);

      if (Repartitioner == "Zoltan") {

        int NumDimensions = List_.get("repartition: Zoltan dimensions", 0);
        ML_Aggregate_Set_Dimensions(agg_, NumDimensions);

        ML_Repartition_Set_Partitioner(ml_nodes_,ML_USEZOLTAN);

        //FIXME JJH this would be a bug if increasing is ever supported
        ML_Repartition_Set_Partitioner(ml_,ML_USEZOLTAN);
      }
      else if (Repartitioner == "ParMETIS")
        ML_Repartition_Set_Partitioner(ml_,ML_USEPARMETIS);
      else if (Repartitioner == "Jostle")
        ML_Repartition_Set_Partitioner(ml_,ML_USEJOSTLE);
      else {
        if (Comm().MyPID() == 0) {
          std::cerr << ErrorMsg_ << "Unrecognized partitioner `"
               << Repartitioner << "'" << std::endl;
          std::cerr << ErrorMsg_ << "It should be: " << std::endl;
          std::cerr << ErrorMsg_ << "<Zoltan> / <ParMETIS>" << std::endl;
        }
        ML_EXIT(-1);
      }
    } //if (ShouldRepartition)
  } //if( SolvingMaxwell_ ...

  // ====================================================================== //
  // visualize aggregate shape and other statistics.                        //
  // ====================================================================== //
  
  if (List_.get("viz: enable", false) ||
      List_.get("repartition: enable",0) ||
      List_.get("aggregation: aux: enable", false))
  { 
    if (SolvingMaxwell_) {
      ML_Aggregate_VizAndStats_Setup(ml_nodes_);
    }
    ML_Aggregate_VizAndStats_Setup(ml_);
  }

  // ====================================================================== //
  // If present, fix the finest-level coordinates in the hierarchy          //
  // ====================================================================== //
  
  if (List_.get("viz: enable", false) || 
      List_.get("repartition: enable",0) ||
      List_.get("aggregation: aux: enable", false))
    ML_CHK_ERR(SetupCoordinates());

  // ====================================================================== //
  // pick up coarsening strategy. METIS and ParMETIS requires additional    //
  // lines, as we have to set the number of aggregates                      //
  // ====================================================================== //

  SetAggregation();

  // ====================================================================== //
  // minor settings                                                         //
  // ====================================================================== //

  double AggressiveFactor = .5;
  AggressiveFactor = List_.get("aggregation: aggressive", AggressiveFactor);
  ML_Aggregate_Set_Phase3AggregateCreationAggressiveness(agg_,AggressiveFactor);

  double Threshold = 0.0;
  Threshold = List_.get("aggregation: threshold", Threshold);
  ML_Aggregate_Set_Threshold(agg_,Threshold);
   
  int MaxCoarseSize = 128;
  MaxCoarseSize = List_.get("coarse: max size", MaxCoarseSize);
  ML_Aggregate_Set_MaxCoarseSize(agg_, MaxCoarseSize );

  int ReqAggrePerProc = 128;
  // FIXME: delete me???
  // compatibility with an older version
  if( List_.isParameter("aggregation: req aggregates per process") ) 
    ReqAggrePerProc = List_.get("aggregation: req aggregates per proces", ReqAggrePerProc);
  else {
    ReqAggrePerProc = List_.get("aggregation: next-level aggregates per process", ReqAggrePerProc);
  }

  if( SolvingMaxwell_ == false ) {
    ML_Aggregate_Set_ReqLocalCoarseSize( ml_, agg_, -1, ReqAggrePerProc);
  } else {
    // Jonathan, is it right ???
    ML_Aggregate_Set_ReqLocalCoarseSize( ml_, agg_, -1, ReqAggrePerProc);
    ML_Aggregate_Set_ReqLocalCoarseSize( ml_nodes_, agg_, -1, ReqAggrePerProc);
  }
  
  if( verbose_ ) {
    std::cout << PrintMsg_ << "Aggregation threshold = " << Threshold << std::endl;
    std::cout << PrintMsg_ << "Max coarse size = " << MaxCoarseSize << std::endl;
    
  }

  // ====================================================================== //
  // one can use dropping on a symmetrized matrix                           //
  // (although, this can be quite expensive for the finest levels)          //
  // ====================================================================== //
     
  if (SolvingMaxwell_ == false) {
    bool UseSymmetrize = List_.get("aggregation: symmetrize", false);
    if (UseSymmetrize == true) ML_Set_Symmetrize(ml_, ML_YES);
    else                       ML_Set_Symmetrize(ml_, ML_NO);  
  }

  // ====================================================================== //
  // Define the scheme for computing the spectral radius of the matrix.     //
  // ====================================================================== //

  std::string str = List_.get("eigen-analysis: type","power-method");
  
  if( verbose_ ) std::cout << PrintMsg_ << "Using `" << str << "' scheme for eigen-computations" << std::endl;
  
  if( str == "cg" )                ML_Set_SpectralNormScheme_Calc(ml_);
  else if( str == "Anorm" )        ML_Set_SpectralNormScheme_Anorm(ml_);
  else if( str == "Anasazi" )      ML_Set_SpectralNormScheme_Anasazi(ml_);
  else if( str == "power-method" ) ML_Set_SpectralNormScheme_PowerMethod(ml_);
  else {
    if( Comm().MyPID() == 0 ) {
      std::cerr << ErrorMsg_ << "parameter `eigen-analysis: type' has an incorrect value"
       << "(" << str << ")" << std::endl;
      std::cerr << ErrorMsg_ << "It should be: " << std::endl
       << ErrorMsg_ << "<cg> / <Anorm> / <Anasazi> / <power-method>" << std::endl;
    }
    ML_EXIT(-10); // wrong input parameter
  }
  int NumEigenIts = List_.get("eigen-analysis: iterations",10);  
  ML_Set_SpectralNorm_Iterations(ml_, NumEigenIts);


  // ====================================================================== //
  // Define scheme to determine damping parameter in prolongator and        //
  // restriction smoother. Only for non-Maxwell.                            //
  // ====================================================================== //

  if( SolvingMaxwell_ == false ) {
    if (!List_.get("energy minimization: enable", false))
      ML_CHK_ERR(SetSmoothingDamping());
  }
  else 
    // FIXME check this!!!!! 4/29/05
    ML_Aggregate_Set_DampingFactor( agg_, 0.0);

  // ====================================================================== //
  // set null space                                                         //
  // ====================================================================== //

  if( SolvingMaxwell_ == false )
    ML_CHK_ERR(SetNullSpace());
  
  OutputList_.set("time: initial phase", InitialTime.ElapsedTime() 
                  + OutputList_.get("time: initial phase", 0.0));
  InitialTime.ResetStartTime();

  // ====================================================================== //
  // Build hierarchy using smoothed aggregation.                            //
  // Then, retrieve parameters for each level. Default values are given by   //
  // entries in parameter list without (level %d)                           //
  // ====================================================================== //

  int Direction;
  if (IsIncreasing == "increasing")
    Direction = ML_INCREASING;
  else
    Direction = ML_DECREASING;

  if (SolvingMaxwell_ == false) {

    bool CreateFakeProblem = false;
    
    if (!CreateFakeProblem && List_.get("aggregation: aux: enable", false))
    {
      double Threshold   = List_.get("aggregation: aux: threshold", 0.0);
      int MaxAuxLevels   = List_.get("aggregation: aux: max levels", 10);
      ml_->Amat[LevelID_[0]].aux_data->threshold = Threshold;
      ml_->Amat[LevelID_[0]].aux_data->enable = 1;
      ml_->Amat[LevelID_[0]].aux_data->max_level = MaxAuxLevels;
      ml_->Amat[LevelID_[0]].num_PDEs = NumPDEEqns_;
    }

    Time.ResetStartTime();

    // Added on Jan-06
    if (List_.get("aggregation: use tentative restriction", false))
    {
      agg_->minimizing_energy = -1;
    }

    // Added on Feb-28
    if (List_.get("aggregation: block scaling", false) && NumPDEEqns_ != 1) // Not advertised in manual
    {
      if (verbose_) 
        std::cout << PrintMsg_ << "Using block scaling for D^{-1}A" << std::endl;
      agg_->block_scaled_SA = 1;
    }
    else
      agg_->block_scaled_SA = 0;

    // energy minimization
    if (List_.get("energy minimization: enable", false))
    {
      if (verbose_)
      {
        std::cout << std::endl;
        std::cout << "Warning: Option `energy minimization' is safer when used with" << std::endl;
        std::cout << "Warning: Uncoupled aggregation scheme. Other aggregation schemes" << std::endl;
        std::cout << "Warning: may crash the code." << std::endl;
        std::cout << "Warning: Remember also use block scaling for std::vector problem" << std::endl;
        std::cout << "Warning: by setting `aggregation: block scaling' = true" << std::endl;
        std::cout << "Warning: Parallel block scaling may crash..." << std::endl;
        std::cout << "Warning: Usually, the `2' norm gives the best results, but" << std::endl;
        std::cout << "Warning: sometimes `1' can perform nicely as well." << std::endl;
        std::cout << std::endl;
      }
      agg_->minimizing_energy = List_.get("energy minimization: type", 2);

      agg_->minimizing_energy_droptol = List_.get("energy minimization: droptol", 0.0);
      if (List_.get("energy minimization: cheap", false)) {
         agg_->cheap_minimizing_energy = 1;
         if ( (verbose_) && (agg_->minimizing_energy != 2)) {
            std::cout << std::endl;
            std::cout << "Warning: Option `energy minimization: cheap' has no effect when the type is not 2." << std::endl;
         }
      }
    }

    profileIterations_ = List_.get("profile: operator iterations", 0);
    ML_Operator_Profile_SetIterations(profileIterations_);
    NumLevels_ = 
      ML_Gen_MultiLevelHierarchy_UsingAggregation(ml_, LevelID_[0], Direction, agg_);

    if (verbose_)
      std::cout << PrintMsg_ << "Time to build the hierarchy = " 
           << Time.ElapsedTime() << " (s)" << std::endl;
    
  } // if (SolvingMaxwell_ == false)
  else {

#ifdef HAVE_ML_EPETRAEXT
    TMatrix_ = ModifyEpetraMatrixColMap(*TMatrix_,
                                        TMatrixColMapTrans_,"Gradient");
#endif
    Apply_BCsToGradient( *EdgeMatrix_, *TMatrix_);

    if (CurlCurlMatrix_ != 0) CheckNullSpace();

    if( TMatrixML_ == 0 ) {
      // convert TMatrix to ML_Operator
      TMatrixML_ = ML_Operator_Create(ml_->comm);
      ML_Operator_WrapEpetraMatrix(const_cast<Epetra_RowMatrix*>(TMatrix_),
                                   TMatrixML_);
    }

    TMatrixTransposeML_ = ML_Operator_Create(ml_->comm);
    ML_Operator_Transpose_byrow(TMatrixML_,TMatrixTransposeML_);

    double EdgeDampingFactor =
              List_.get("aggregation: damping factor",ML_DDEFAULT);
    double enrichBeta = List_.get("aggregation: enrich beta",0.0);

    double PeDropThreshold = List_.get("aggregation: edge prolongator drop threshold",ML_DDEFAULT);

    profileIterations_ = List_.get("profile: operator iterations", 0);
    ML_Operator_Profile_SetIterations(profileIterations_);

    // Set up arrays for holding the curl-curl and mass matrices separately.
    typedef ML_Operator* pML_Operator;  // to avoid compiler warning
    CurlCurlMatrix_array = new pML_Operator[NumLevels_];
    for (int i=0; i< NumLevels_; i++) CurlCurlMatrix_array[i] = NULL;
    if (CurlCurlMatrix_) {
      CurlCurlMatrix_array[LevelID_[0]] = ML_Operator_Create(ml_->comm);
      ML_Operator_WrapEpetraMatrix(
                            const_cast<Epetra_RowMatrix*>(CurlCurlMatrix_),
                            CurlCurlMatrix_array[LevelID_[0]] );
    }
    MassMatrix_array = new pML_Operator[NumLevels_];
    for (int i=0; i< NumLevels_; i++) MassMatrix_array[i] = NULL;
    if (MassMatrix_) {
      MassMatrix_array[LevelID_[0]] = ML_Operator_Create(ml_->comm);
      ML_Operator_WrapEpetraMatrix( const_cast<Epetra_RowMatrix*>(MassMatrix_),
                                  MassMatrix_array[LevelID_[0]] );
    }

    NumLevels_ = ML_Gen_MGHierarchy_UsingReitzinger(ml_,&ml_nodes_,
                            LevelID_[0], Direction,agg_,
                            CurlCurlMatrix_array,
                            MassMatrix_array,
                            TMatrixML_,TMatrixTransposeML_, 
                            &Tmat_array,&Tmat_trans_array, 
                            EdgeDampingFactor,
                            enrichBeta, PeDropThreshold);

  }

#ifdef HAVE_ML_EPETRAEXT
  // ============================================================== //
  // dump matrix in matrix market format using EpetraExt            //
  // ============================================================== //
  
  if (List_.get("dump matrix: enable", false)) {
    std::string FileName = List_.get("dump matrix: file name", "ml-matrix.mm");
    char FileName2[80];
    static int count = 0;
    char comment[80];
    sprintf(comment,"printed by MultiLevelPreconditioner, %d processors",
            Comm().NumProc());

    if (SolvingMaxwell_) {
      sprintf(FileName2,"ml-matrix-%s.mm","edge");
      EpetraExt::RowMatrixToMatrixMarketFile(FileName2, *RowMatrix_, 
                              "Maxwell -- edge finite element matrix",
                              comment);
      sprintf(FileName2,"ml-matrix-%s.mm","curlcurl");
      if (CurlCurlMatrix_)
        EpetraExt::RowMatrixToMatrixMarketFile(FileName2, *CurlCurlMatrix_, 
                                "Maxwell -- (curl,curl) matrix",
                                comment);
      sprintf(FileName2,"ml-matrix-%s.mm","mass");
      if (MassMatrix_)
        EpetraExt::RowMatrixToMatrixMarketFile(FileName2, *MassMatrix_, 
                                "Maxwell -- mass matrix",
                                comment);
      sprintf(FileName2,"ml-matrix-%s.mm","gradient");
      EpetraExt::RowMatrixToMatrixMarketFile(FileName2, *TMatrix_, 
                                "Maxwell -- gradient matrix",
                                comment);
      sprintf(FileName2,"ml-matrix-%s.mm","node");
      EpetraExt::RowMatrixToMatrixMarketFile(FileName2, *NodeMatrix_, 
                                "Maxwell -- auxiliary nodal FE matrix",
                                comment);
    }
    else {
      if (FileName == "auto-count")
        sprintf(FileName2,"ml-matrix-%d.mm",count++);
      else
        sprintf(FileName2,"%s",FileName.c_str());
  
      if (verbose_)
        std::cout << PrintMsg_ << "Print matrix on file `" << FileName2
             << "'..." << std::endl;
      EpetraExt::RowMatrixToMatrixMarketFile(FileName2, *RowMatrix_, 
                     "Epetra_RowMatrix", comment);
    }
  }
#endif

  {
    int NL2 = OutputList_.get("max number of levels", 0);
    OutputList_.set("max number of levels", NL2+NumLevels_);
  }
  
  if( AnalyzeMemory_ ) {
    call2 = ML_MaxMemorySize();
    memory_[ML_MEM_HIERARCHY] = call2-call1;
    call1 = call2;
#ifdef ML_MALLOC
    call2_malloc = ML_MaxAllocatableSize();
    if (verbose_) std::cout << "Memory : max allocatable block = " << call2_malloc << " Mbytes" << std::endl;    
    memory_[ML_MEM_HIERARCHY_MALLOC] = call1_malloc-call2_malloc;
    call1_malloc = call2_malloc;
#endif
  }
  
  if( verbose_ ) std::cout << PrintMsg_ << "Number of actual levels : " << NumLevels_ << std::endl;

  OutputList_.set("time: hierarchy", InitialTime.ElapsedTime() 
                  + OutputList_.get("time: hierarchy", 0.0));
  InitialTime.ResetStartTime();

  // ====================================================================== //
  // Now cycling over all levels                                            //
  // ====================================================================== //

  if ( NumLevels_ > 1 )
    ML_CHK_ERR(SetSmoothers());
  /* FIXME: moved in DestroyPreconditioner()
  // this below *must* to be here and not before the construction of the smoothers
  if ((agg_)->aggr_info != NULL) {
    for (int i = 0 ; i < NumLevels_ ; ++i) {
      if ((agg_)->aggr_info[i] != NULL) 
        ML_memory_free((void **)&((agg_)->aggr_info[i]));
    } 
  }
  */

  if (AnalyzeMemory_) {
    call2 = ML_MaxMemorySize();
    memory_[ML_MEM_SMOOTHER] = call2 - call1;
    call1 = call2;
#ifdef ML_MALLOC
    call2_malloc = ML_MaxAllocatableSize();
    if (verbose_) std::cout << "Memory : max allocatable block = " << call2_malloc << " Mbytes" << std::endl;        
    memory_[ML_MEM_SMOOTHER_MALLOC] = call1_malloc - call2_malloc;
    call1_malloc = call2_malloc;
#endif
  }
  
  OutputList_.set("time: smoothers setup", InitialTime.ElapsedTime() 
                  + OutputList_.get("time: smoothers setup", 0.0));
  InitialTime.ResetStartTime();

  // ====================================================================== //
  // solution of the coarse problem                                         //
  // ====================================================================== //

  ML_CHK_ERR(SetCoarse());

  ownership_ = false;

  OutputList_.set("time: coarse solver setup", InitialTime.ElapsedTime() 
                  + OutputList_.get("time: coarse solver setup", 0.0));
  InitialTime.ResetStartTime();

  // ====================================================================== //
  // Recompute complexities                                                 //
  // ====================================================================== //

  if (!SolvingMaxwell_) {

    double RowComplexity = 0.0, RowZero = 0.0;
    double NnzComplexity = 0.0, NnzZero = 0.0;

    for (int i = 0 ; i < NumLevels_ ; ++i) 
    {
      int local[2];
      int global[2];
      local[0] = ml_->Amat[LevelID_[i]].invec_leng;
      local[1] = ml_->Amat[LevelID_[i]].N_nonzeros;
      Comm().SumAll(local,global,2);
      // kludge because it appears that the nonzeros for ML
      // defined operators are only local, while for Epetra
      // are global.
      if (i == 0) {
        global[1] = local[1]; // FIXME: this is black magic
        RowZero = global[0];
        NnzZero = global[1];
      }
      RowComplexity += global[0];
      NnzComplexity += global[1];
    }
    if (verbose_) 
    {
      std::cout << PrintMsg_ << std::endl;
      std::cout << PrintMsg_ << "sum n_i   / n_finest   = " << RowComplexity / RowZero << std::endl;
      std::cout << PrintMsg_ << "sum nnz_i / nnz_finest = " << NnzComplexity / NnzZero << std::endl;
    }
  }

  // ====================================================================== //
  // Specific   preconditioners                                             //
  // NOTE: the two-level DD preconditioners are kind of experimental!       //
  // I suppose that the user knows what he/she is doing.... No real checks  //
  // are performed. Note also that the coarse solver is somehow supposed to //
  // be implemented as a post smoother (FIXME)                              //
  // ====================================================================== //

  if (AnalyzeMemory_) {
    call2 = ML_MaxMemorySize();
    memory_[ML_MEM_COARSE] = call2 - call1;
    call1 = call2;
#ifdef ML_MALLOC
    call2_malloc = ML_MaxAllocatableSize();
    if (verbose_) std::cout << "Memory : max allocatable block = " << call2_malloc << " Mbytes" << std::endl;        
    memory_[ML_MEM_COARSE_MALLOC] = call1_malloc - call2_malloc;
    call1_malloc = call2_malloc;
#endif
  }
  
  ML_Gen_Solver(ml_, ML_MGV, LevelID_[0], LevelID_[NumLevels_-1]);

  // ====================================================================== //
  // Use of filtering functions, here called `filtering: enable'            //
  // If this option is true, the code detects the non-converging modes of   //
  // I - ML^{-1}A (where ML is the preconditioner we have just built), and  //
  // creates a new V cycle to be added to the preconditioner. This part is  //
  // equivalent to the GGB files of Haim Waisman (files ml_struct.c and     //
  // ml_ggb.c, in the Main subdirectory).                                   //
  // ====================================================================== //

  ML_CHK_ERR(SetFiltering());
  
  // ====================================================================== //
  // One may decide to print out the entire hierarchy (to be analyzed in    //
  // MATLAB, for instance).                                                 //
  // ====================================================================== //

  bool PrintHierarchy = List_.get("print hierarchy", false);
  if ( PrintHierarchy == true ) Print();

  // ====================================================================== //
  // Other minor settings                                                   //
  // ====================================================================== //
  
  CreateLabel();
  
//  if( SolvingMaxwell_ == false ) rst: Not sure why this was here?
    SetPreconditioner();

  IsComputePreconditionerOK_ = true;
  
  // ====================================================================== //
  // Compute the rate of convergence (for reuse preconditioners)            //
  // ====================================================================== //

  if( List_.get("reuse: enable", false) == true ) 
    CheckPreconditionerKrylov();
  
  if (AnalyzeMemory_) {
    memory_[ML_MEM_FINAL] = ML_MaxMemorySize();
    memory_[ML_MEM_TOT1] = memory_[ML_MEM_FINAL] - memory_[ML_MEM_INITIAL];
#ifdef ML_MALLOC
    memory_[ML_MEM_FINAL_MALLOC] = ML_MaxAllocatableSize();
    memory_[ML_MEM_TOT1_MALLOC] = memory_[ML_MEM_INITIAL_MALLOC] - memory_[ML_MEM_FINAL_MALLOC];
#endif
  }
  
  // print unused parameters
  if (List_.isParameter("print unused")) {
    int ProcID = List_.get("print unused",-2);
    if ((Comm().MyPID() == ProcID || ProcID == -1) && verbose_) PrintUnused();
  }

  // ===================================================================== //
  // compute the coordinates for each level (that is, the center of        //
  // gravity, as no mesh is really available for coarser levels)           //
  // ===================================================================== //
 
  OutputList_.set("time: final setup", InitialTime.ElapsedTime() 
                  + OutputList_.get("time: final setup", 0.0));
  InitialTime.ResetStartTime();

  if (ML_Get_PrintLevel() == 10 && Comm().MyPID() == 0) {
    std::cout << std::endl;
    std::cout << "Cumulative timing for construction so far: " << std::endl;
    std::cout << PrintMsg_ << "- for initial setup   = " 
         << OutputList_.get("time: initial phase", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for hierarchy setup = " 
         << OutputList_.get("time: hierarchy", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for smoothers setup = "
         << OutputList_.get("time: smoothers setup", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for coarse setup    = "
         << OutputList_.get("time: coarse solver setup", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for final setup     = " 
         << OutputList_.get("time: final setup", 0.0) << " (s)" << std::endl;
  }

  /* ------------------- that's all folks --------------------------------- */

  if( verbose_ )
    ML_print_line("-",78);

  ConstructionTime_ += Time.ElapsedTime();
  ++NumConstructions_;

 } 
 catch(...)
 {
   if (Comm().MyPID() == 0) {
     fprintf(stderr,"\n*********************************************************\n");
     fprintf(stderr,"ML failed to compute the multigrid preconditioner. The\n");
     fprintf(stderr,"most common problem is an incorrect  data type in ML's\n");
     fprintf(stderr,"parameter list (e.g. 'int' instead of 'bool').\n\n");
     fprintf(stderr,"Note: List.set(\"ML print parameter list\",true) might help\nfigure out the bad one.\n");
     fprintf(stderr,"*********************************************************\n\n");
   }

   ML_CHK_ERR(-1);
 }
  
 return(0);
}

// ================================================ ====== ==== ==== == =

int ML_Epetra::MultiLevelPreconditioner::
ReComputePreconditioner()
{

 try{

  if (SolvingMaxwell_ == true)
    ML_CHK_ERR(-1);

  if (IsPreconditionerComputed() == false)
    ML_CHK_ERR(-2);

  IsComputePreconditionerOK_ = false;

  // =========================== //
  // re-build the preconditioner //
  // =========================== //

  int OutputLevel = List_.get("ML output", -47);  
  if (OutputLevel == -47) OutputLevel = List_.get("output", 0);  
  ML_Set_PrintLevel(OutputLevel);

  Epetra_Time Time(Comm());
  Epetra_Time InitialTime(Comm());
  InitialTime.ResetStartTime();
  Time.ResetStartTime();

  {
    int NumCompute = OutputList_.get("number of construction phases", 0);
    OutputList_.set("number of construction phases", ++NumCompute);
  }
  
  for (int i = 0; i < ml_->ML_num_levels; i++)
  {
    ML_Smoother_Clean(&(ml_->pre_smoother[i]));
    ML_Smoother_Init(&(ml_->pre_smoother[i]), &(ml_->SingleLevel[i]));
    ML_Smoother_Clean(&(ml_->post_smoother[i]));
    ML_Smoother_Init(&(ml_->post_smoother[i]), &(ml_->SingleLevel[i]));
    ML_CSolve_Clean(&(ml_->csolve[i]));
    ML_CSolve_Init(&(ml_->csolve[i]));
  }

  if( verbose_ ) 
    ML_print_line("-",78);
  
  FirstApplication_ = true;

  if (verbose_) {
    std::cout << PrintMsg_ << "Re-computing the preconditioner..." << std::endl;
  }

  profileIterations_ = List_.get("profile: operator iterations", 0);
  ML_Operator_Profile_SetIterations(profileIterations_);
  ML_Gen_MultiLevelHierarchy_UsingSmoothedAggr_ReuseExistingAgg(ml_, agg_);

  if (verbose_)
    std::cout << PrintMsg_ << "Time to re-build the hierarchy = " 
         << Time.ElapsedTime() << " (s)" << std::endl;
  Time.ResetStartTime();

  if (verbose_) 
    std::cout << PrintMsg_ << "Number of actual levels : " << NumLevels_ << std::endl;

  OutputList_.set("time: hierarchy", Time.ElapsedTime() 
                  + OutputList_.get("time: hierarchy", 0.0));
  Time.ResetStartTime();

  // ====================================================================== //
  // Now cycling over all levels                                            //
  // ====================================================================== //

  ML_CHK_ERR(SetSmoothers());

  OutputList_.set("time: smoothers setup", Time.ElapsedTime() 
                  + OutputList_.get("time: smoothers setup", 0.0));
  Time.ResetStartTime();

  // ====================================================================== //
  // solution of the coarse problem                                         //
  // ====================================================================== //

  if( NumLevels_ > 1 ) {
    ML_CHK_ERR(SetCoarse());
  }

  OutputList_.set("time: coarse solver setup", Time.ElapsedTime() 
                  + OutputList_.get("time: coarse solver setup", 0.0));
  Time.ResetStartTime();

  ML_Gen_Solver(ml_, ML_MGV, LevelID_[0], LevelID_[NumLevels_-1]);

  IsComputePreconditionerOK_ = true;

  OutputList_.set("time: final setup", Time.ElapsedTime() 
                  + OutputList_.get("time: final setup", 0.0));
  Time.ResetStartTime();

  if (ML_Get_PrintLevel() == 10 && Comm().MyPID() == 0) {
    std::cout << std::endl;
    std::cout << "Cumulative timing for construction so far: " << std::endl;
    std::cout << PrintMsg_ << "- for initial setup   = " 
      << OutputList_.get("time: initial phase", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for hierarchy setup = " 
      << OutputList_.get("time: hierarchy", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for smoothers setup = "
      << OutputList_.get("time: smoothers setup", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for coarse setup    = "
      << OutputList_.get("time: coarse solver setup", 0.0) << " (s)" << std::endl;
    std::cout << PrintMsg_ << "- for final setup     = " 
      << OutputList_.get("time: final setup", 0.0) << " (s)" << std::endl;
  }

  /* ------------------- that's all folks --------------------------------- */

  if (verbose_)
    ML_print_line("-",78);

  ConstructionTime_ += InitialTime.ElapsedTime();
  ++NumConstructions_;

 }
 catch(...)
 {
   ML_CHK_ERR(-1);
 }

 return(0);

}

// ================================================ ====== ==== ==== == =

/*! Print the individual operators in the multigrid hierarchy.

 \param whichHierarchy (In) By default, this method prints the main
                            multigrid hierarchy. If solving Maxwell's
                            equations, and this is set to anything other than
                            "main", the auxiliary node hierarchy will print.
*/

void ML_Epetra::MultiLevelPreconditioner::
Print(const char *whichHierarchy)
{
  // ====================================================================== //
  // One may decide to print out the entire hierarchy (to be analyzed in    //
  // MATLAB, for instance).                                                 //
  // ====================================================================== //

  if( Comm().NumProc() > 1) {
    if( Comm().MyPID() == 0 ) {
      std::cerr << std::endl;
      std::cerr << ErrorMsg_ << "The multigrid hierarchy can be printed only"
                        << "for serial runs." << std::endl;
      std::cerr << std::endl;
    }
  }
  
  if( Comm().NumProc() == 1 ) {
      std::cout << std::endl;
      std::cout << PrintMsg_ << "You are printing the entire hierarchy," << std::endl
       << PrintMsg_ << "from finest level (" << LevelID_[0] 
                    << ") to coarsest (" << LevelID_[NumLevels_-1] << ")." << std::endl
       << PrintMsg_ << "MATLAB can be used to load the matrices, using spconvert()" << std::endl;
      std::cout << std::endl;

    ML* mlptr;
    char auxSuffix[100];
    if ( (strcmp(whichHierarchy,"main") != 0) && SolvingMaxwell_) {
      mlptr = ml_nodes_;
      strncpy(auxSuffix,whichHierarchy,80);
    }
    else {
      mlptr = ml_;
      auxSuffix[0] = '\0';
    }

    char name[80];
    // Amat (one for each level)
    for( int i=0 ; i<NumLevels_ ; ++i ) {
      sprintf(name,"Amat_%d%s", LevelID_[i],auxSuffix);
      ML_Operator_Print_UsingGlobalOrdering(mlptr->Amat+LevelID_[i], name,
                                            NULL,NULL);
    }

    // Pmat (one for each level, except last)
    for( int i=1 ; i<NumLevels_ ; ++i ) {
      sprintf(name,"Pmat_%d%s", LevelID_[i],auxSuffix);
      ML_Operator_Print_UsingGlobalOrdering(mlptr->Pmat+LevelID_[i], name,
                                            NULL,NULL);
    }

    // Rmat (one for each level, except first)
    for( int i=0 ; i<NumLevels_-1 ; ++i ) {
      sprintf(name,"Rmat_%d%s", LevelID_[i],auxSuffix);
      //FIXME
      ML_Operator_Print(&(mlptr->Rmat[LevelID_[i]]), name);
    }

    // Tmat (one for each level)
    if (SolvingMaxwell_)
      for( int i=0 ; i<NumLevels_ ; ++i ) {
        sprintf(name,"Tmat_%d", LevelID_[i]);
        ML_Operator_Print_UsingGlobalOrdering(Tmat_array[LevelID_[i]], name,
                                              NULL,NULL);
      }

  }
} //Print() method

// ============================================================================

void ML_Epetra::MultiLevelPreconditioner::PrintUnused(const int MyPID) const
{
  if( Comm().MyPID() == MyPID ) {
    ML_print_line("-",78);
    std::cout << PrintMsg_ << "Unused parameters:" << std::endl;
    PrintUnused();
    ML_print_line("-",78);
  }
}

// ============================================================================

void ML_Epetra::MultiLevelPreconditioner::PrintList(int MyPID) 
{
  if( Comm().MyPID() == MyPID ) {
    std::cout << "+++++++++++++++++++++++++++++++++++" << std::endl;
    std::cout << "++++ start of ML parameter list +++" << std::endl;
    std::cout << "+++++++++++++++++++++++++++++++++++" << std::endl;
    List_.print(std::cout);
    std::cout << "-----------------------------------" << std::endl;
    std::cout << "---- end of ML parameter list -----" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
  }
}

// ============================================================================

int ML_Epetra::MultiLevelPreconditioner::
SetParameterList(const ParameterList & List) 
{
  if( IsComputePreconditionerOK_ == true ) DestroyPreconditioner();
  List_ = List;
  return 0;
  
}

// ============================================================================

int ML_Epetra::MultiLevelPreconditioner::CreateLabel()
{  

  char finest[80];
  char coarsest[80];
  finest[0] = '\0';
  coarsest[0] = '\0';
  char * label;

  int i = ml_->ML_finest_level;
     
  if (ml_->pre_smoother[i].smoother->func_ptr != NULL) {
    label = ml_->pre_smoother[i].label;
    if( strncmp(label,"PreS_",4) == 0 ) sprintf(finest, "%s", "~");
    else                                sprintf(finest, "%s", label);
  } else                                sprintf(finest, "%s", "~"); 

  if (ml_->post_smoother[i].smoother->func_ptr != NULL) {
    label = ml_->post_smoother[i].label;
    if( strncmp(label,"PostS_", 5) == 0 ) sprintf(finest, "%s/~", finest);
    else                                  sprintf(finest, "%s/%s", finest, label);
  } else                                  sprintf(finest, "%s/~", finest);  

  if (i != ml_->ML_coarsest_level) {
    i = ml_->ML_coarsest_level;
    if ( ML_CSolve_Check( &(ml_->csolve[i]) ) == 1 ) {
    sprintf(coarsest, "%s", ml_->csolve[i].label);
    }
    
    else {
      if (ml_->pre_smoother[i].smoother->func_ptr != NULL) {
    label = ml_->pre_smoother[i].label;
    if( strncmp(label,"PreS_",4) == 0 ) sprintf(coarsest, "%s", "~");
    else                                sprintf(coarsest, "%s", label);
      } else                                sprintf(coarsest, "%s", "~"); 
      if (ml_->post_smoother[i].smoother->func_ptr != NULL) {
    label = ml_->post_smoother[i].label;
    if( strncmp(label,"PostS_", 5) == 0 ) sprintf(coarsest, "%s/~", coarsest); 
    else                                  sprintf(coarsest, "%s/%s",coarsest, label);
      } else                                  sprintf(coarsest, "%s/~", coarsest); 
    }
  }

  if( SolvingMaxwell_ == false ) 
    sprintf(Label_, "ML (L=%d, %s, %s)", 
     ml_->ML_num_actual_levels, finest, coarsest);
  else
    sprintf(Label_, "ML (Maxwell, L=%d, %s, %s)", 
     ml_->ML_num_actual_levels, finest, coarsest);
  
  return 0;
    
}

// ============================================================================

int ML_Epetra::MultiLevelPreconditioner::
ApplyInverse(const Epetra_MultiVector& X,
         Epetra_MultiVector& Y) const
{

  int before = 0, after = 0;
#ifdef ML_MALLOC
  int before_malloc = 0, after_malloc = 0;
#endif

  if (AnalyzeMemory_) {
#ifdef ML_MALLOC
    before_malloc = ML_MaxAllocatableSize();
#endif
    before = ML_MaxMemorySize();
  }
    
  Epetra_Time Time(Comm());
  
  // ========================================================= //
  // I do not perfom checks on maps, like the following,       //
  // if (!X.Map().SameAs(OperatorDomainMap())) ML_CHK_ERR(-1)  //
  // if (!Y.Map().SameAs(OperatorRangeMap())) ML_CHK_ERR(-1)   //
  // because they can be expensive                             //
  //                                                           //
  // We allocate xtmp, needed in case X is scaled in solver or //
  // X = Y. We cannot pre-allocate xtmp, since the user might  //
  // give in input different number of vectors.                //
  //                                                           //
  // Finally, since does not handle multivectors, we need to   //
  // extract and iterate one at a time.                        //
  //                                                           //
  // FIXME: the Y.PutScalar(0.0) can probably be skipped.      //
  // ========================================================= //
  
  if (Y.NumVectors() != X.NumVectors()) 
    ML_CHK_ERR(-3);
  if( !IsPreconditionerComputed() ) 
    ML_CHK_ERR(-10);

  Epetra_MultiVector xtmp(X); 

  if (ZeroStartingSolution_) Y.PutScalar(0.0); 

  double** xvectors;
  double** yvectors;
  ML_CHK_ERR(xtmp.ExtractView(&xvectors));
  ML_CHK_ERR(Y.ExtractView(&yvectors));

  for (int i = 0; i < X.NumVectors(); ++i) {

    // ================================================== //
    // Added support for multiple cycles on 08-Mar-05     //
    // Tested for ML_Cycle_MG only                        //
    //                                                    //
    // note that all the DD stuff is "delicate"           //
    // in the sense that the coarse solver must be Amesos //
    //                                                    //
    // I am not sure if ML_NONZERO applies to something   //
    // else than ML_Cycle_MG().                           //
    //                                                    //
    // The flt_ml_ is the filtering, which requires a     //
    // suitable setup phase. Note that in the current     //
    // implementation the                                 //
    // resulting preconditioner is always non-symmetric   //
    // ================================================== //

    for (int ia = 0 ; ia < CycleApplications_ ; ++ia) {

      int StartingSolution;
      if (ia || !ZeroStartingSolution_)
        StartingSolution = ML_NONZERO;
      else
        StartingSolution = ML_ZERO;

      switch(ml_->ML_scheme) {
      case(ML_MGFULLV):
        ML_Solve_MGFull(ml_, xvectors[i], yvectors[i]); 
        break;
      case(ML_SAAMG): //Marian Brezina's solver
        ML_Solve_AMGV(ml_, xvectors[i], yvectors[i]); 
        break;
      case(ML_PAMGV): //V-cycle, where modes are projected out before & after
        ML_Solve_ProjectedAMGV(ml_, xvectors[i], yvectors[i]); 
        break;
      case(ML_ONE_LEVEL_DD): 
        ML_DD_OneLevel(&(ml_->SingleLevel[ml_->ML_finest_level]),
                       yvectors[i], xvectors[i],
                       ML_ZERO, ml_->comm, ML_NO_RES_NORM, ml_);
        break;
      case(ML_TWO_LEVEL_DD_ADD):
        ML_DD_Additive(&(ml_->SingleLevel[ml_->ML_finest_level]),
                       yvectors[i], xvectors[i],
                       ML_ZERO, ml_->comm, ML_NO_RES_NORM, ml_);
        break;
      case(ML_TWO_LEVEL_DD_HYBRID): 
        ML_DD_Hybrid(&(ml_->SingleLevel[ml_->ML_finest_level]),
                     yvectors[i], xvectors[i],
                     ML_ZERO, ml_->comm, ML_NO_RES_NORM, ml_);    
        break;
    case(ML_TWO_LEVEL_DD_HYBRID_2):
      ML_DD_Hybrid_2(&(ml_->SingleLevel[ml_->ML_finest_level]),
             yvectors[i], xvectors[i],
             ML_ZERO, ml_->comm, ML_NO_RES_NORM, ml_);    
      break;
    default: 
      ML_Cycle_MG(&(ml_->SingleLevel[ml_->ML_finest_level]), 
                  yvectors[i], xvectors[i], StartingSolution,
                  ml_->comm, ML_NO_RES_NORM, ml_);
      }
    }

    if (flt_ml_) {
      ML_Cycle_MG(&(flt_ml_->SingleLevel[flt_ml_->ML_finest_level]),
          yvectors[i], xvectors[i],
          ML_NONZERO, flt_ml_->comm, ML_NO_RES_NORM, flt_ml_);
    }
  }

  // ====== //
  // timing //
  // ====== //

  MultiLevelPreconditioner * This = const_cast<MultiLevelPreconditioner *>(this);

  if (AnalyzeMemory_) {
#ifdef ML_MALLOC
    after_malloc = ML_MaxAllocatableSize();
#endif
    after = ML_MaxMemorySize();
  }
  
  double t = Time.ElapsedTime();
  if (FirstApplication_) {
    This->FirstApplication_ = false;
    This->FirstApplicationTime_ += t;
    This->memory_[ML_MEM_PREC_FIRST] = after - before;
#ifdef ML_MALLOC
    This->memory_[ML_MEM_PREC_FIRST_MALLOC] = before_malloc - after_malloc;
#endif
  } 
  else {
    This->memory_[ML_MEM_PREC_OTHER] = after - before;
#ifdef ML_MALLOC
    This->memory_[ML_MEM_PREC_OTHER_MALLOC] = before_malloc - after_malloc;
#endif
    This->ApplicationTime_ += t;
  }
  
  ++(This->NumApplications_);

  return 0;
}

// ============================================================================
/*! Values for \c "coarse: type"
 * - \c Jacobi
 * - \c Gauss-Seidel
 * - \c symmetric Gauss-Seidel
 * - \c MLS
 * - \c Hiptmair (Maxwell only)
 * - \c SuperLU (deprecated)
 * - \c Amesos-KLU
 * - \c Amesos-UMFPACK
 * - \c Amesos-Superludist
 * - \c Amesos-Superlu
 * - \c Amesos-MUMPS
 * - \c Amesos-ScALAPACK (under development in Amesos)
 * - \c do-nothing
 */
int ML_Epetra::MultiLevelPreconditioner::SetCoarse() 
{

  std::string CoarseSolution = List_.get("coarse: type", "Amesos-KLU");
  int NumSmootherSteps = List_.get("coarse: sweeps", 2);
  double Omega = List_.get("coarse: damping factor", 1.0);
  double AddToDiag = List_.get("coarse: add to diag", 1e-12);
  std::string PreOrPostSmoother = List_.get("coarse: pre or post","post");
  int pre_or_post;
  if( PreOrPostSmoother == "pre" ) pre_or_post = ML_PRESMOOTHER;
  else if( PreOrPostSmoother == "both" ) pre_or_post = ML_BOTH;
  else pre_or_post = ML_POSTSMOOTHER;

  // rst: Changing polynomial interface:
  //    1) polynomial degree is set from "coarse: sweeps"
  //    2) "coarse: Chebyshev" also calls ML's Chebyshev
  //    3) "coarse: Chebshev alpha" also sets alpha
  //    4) "coarse: node sweeps" and "coarse: edge sweeps" now set polynomial
  //                degree within Hiptmair 
  //
  // For backward compatiblity, we still take the degree from 
  // "coarse: MLS polynomial order" if set, still recognize MLS 

  double ChebyshevAlpha = List_.get("coarse: MLS alpha",-2.0);
  if ( ChebyshevAlpha == -2.) ChebyshevAlpha = List_.get("coarse: Chebyshev alpha", 30.);

  int ChebyshevPolyOrder = List_.get("coarse: MLS polynomial order",-7);
  if (ChebyshevPolyOrder == -7) ChebyshevPolyOrder = NumSmootherSteps;

  char msg[80];
  sprintf(msg, "Coarse solve (level %d) : ", LevelID_[NumLevels_-1]);
    
  int MaxProcs = List_.get("coarse: max processes", -1);

  if( CoarseSolution == "Jacobi" ) {
    if( verbose_ ) std::cout << msg << "Jacobi (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << ")" << std::endl;
    ML_Gen_Smoother_Jacobi(ml_, LevelID_[NumLevels_-1], pre_or_post,
               NumSmootherSteps, Omega);

  } else if( CoarseSolution == "Gauss-Seidel" ) {
    if( verbose_ ) std::cout << msg << "Gauss-Seidel (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << ")" << std::endl;
#ifdef HAVE_ML_IFPACK
    if (ml_->Amat[LevelID_[NumLevels_-1]].type == ML_TYPE_CRS_MATRIX) {
      if (verbose_)
        std::cout << msg << "Epetra_CrsMatrix detected, using "
             << "Ifpack implementation" << std::endl;
      std::string IfpackType = "point relaxation stand-alone";
      ParameterList& IfpackList = List_.sublist("smoother: ifpack list");;
      IfpackList.set("relaxation: type", "Gauss-Seidel");
      IfpackList.set("relaxation: sweeps", NumSmootherSteps);
      IfpackList.set("relaxation: damping factor", Omega);
      ML_Gen_Smoother_Ifpack(ml_, IfpackType.c_str(),
                             0, LevelID_[NumLevels_-1], pre_or_post,
                             (void*)(&IfpackList),(void *) Comm_);
    }
    else
#endif
      ML_Gen_Smoother_GaussSeidel(ml_, LevelID_[NumLevels_-1], pre_or_post,
               NumSmootherSteps, Omega);

  } else if( CoarseSolution == "ML Gauss-Seidel" ) {
    if( verbose_ ) std::cout << msg << "Gauss-Seidel (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << ")" << std::endl;
    ML_Gen_Smoother_GaussSeidel(ml_, LevelID_[NumLevels_-1], pre_or_post,
                                NumSmootherSteps, Omega);

  } else if( CoarseSolution == "symmetric Gauss-Seidel" ) {
    if( verbose_ ) std::cout << msg << "symmetric Gauss-Seidel (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << ")" << std::endl;
#ifdef HAVE_ML_IFPACK
    if (ml_->Amat[LevelID_[NumLevels_-1]].type == ML_TYPE_CRS_MATRIX) {
      if (verbose_)
        std::cout << msg << "Epetra_CrsMatrix detected, using "
             << "Ifpack implementation" << std::endl;
      std::string IfpackType = "point relaxation stand-alone";
      ParameterList& IfpackList = List_.sublist("smoother: ifpack list");;
      IfpackList.set("relaxation: type", "symmetric Gauss-Seidel");
      IfpackList.set("relaxation: sweeps", NumSmootherSteps);
      IfpackList.set("relaxation: damping factor", Omega);
      ML_Gen_Smoother_Ifpack(ml_, IfpackType.c_str(),
                             0, LevelID_[NumLevels_-1], pre_or_post,
                             //IfpackList,*Comm_);
                             (void*)&IfpackList,(void*)Comm_);
    }
    else
#endif
      ML_Gen_Smoother_SymGaussSeidel(ml_, LevelID_[NumLevels_-1],
                     pre_or_post, NumSmootherSteps, Omega);

  } else if( CoarseSolution == "ML symmetric Gauss-Seidel" ) {
    if( verbose_ ) std::cout << msg << "symmetric Gauss-Seidel (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << ")" << std::endl;
    ML_Gen_Smoother_SymGaussSeidel(ml_, LevelID_[NumLevels_-1],
                                   pre_or_post, NumSmootherSteps, Omega);

  } else if(( CoarseSolution == "MLS" ) || (CoarseSolution == "Chebyshev")) {
  if( verbose_ ) std::cout << msg << "Chebyshev (degree="
                        << ChebyshevPolyOrder << ",alpha=" << ChebyshevAlpha
                        << "," << PreOrPostSmoother << ")" << std::endl;
     ML_Gen_Smoother_Cheby(ml_, LevelID_[NumLevels_-1], pre_or_post,
                         ChebyshevAlpha, ChebyshevPolyOrder);

  } else if( CoarseSolution == "Hiptmair" ) {
                                                                                
      if (SolvingMaxwell_ == false) {
        if (Comm().MyPID() == 0) {
          std::cerr << ErrorMsg_ << "Hiptmair smoothing is only supported" << std::endl;
          std::cerr << ErrorMsg_ << "for solving eddy current equations." << std::endl;
          std::cerr << ErrorMsg_ << "Choose another smoother." << std::endl;
        }
        ML_EXIT(EXIT_FAILURE);
      }
                                                                                
      std::string SubSmootherType = List_.get("coarse: subsmoother type","MLS");
      int nodal_its = List_.get("coarse: node sweeps", 2);
      int edge_its = List_.get("coarse: edge sweeps", 2);
                                                                                
      int logical_level = LevelID_[NumLevels_-1];
      void *edge_smoother = 0, *nodal_smoother = 0;
                                                                                
      double omega;
      char subsmDetails[80];
      // only MLS and SGS are currently supported
      if ( (SubSmootherType == "MLS") || (SubSmootherType == "Chebyshev"))
      {
        nodal_smoother=(void *) ML_Gen_Smoother_Cheby;
        nodal_args_ = ML_Smoother_Arglist_Create(2);
        edge_args_ = ML_Smoother_Arglist_Create(2);
        // set polynomial degree
        ML_Smoother_Arglist_Set(nodal_args_, 0, &nodal_its);
        edge_smoother=(void *) ML_Gen_Smoother_Cheby;
        ML_Smoother_Arglist_Set(edge_args_, 0, &edge_its);
                                                                                
        // set alpha (lower bound of smoothing interval (alpha,beta) )
                                                                                
        ML_Smoother_Arglist_Set(edge_args_, 1, &ChebyshevAlpha);
        ML_Smoother_Arglist_Set(nodal_args_, 1, &ChebyshevAlpha);
        sprintf(subsmDetails,"edge sweeps=%d, node sweeps=%d,alpha=%3.1e",
                edge_its,nodal_its,ChebyshevAlpha);
      }
      else if (SubSmootherType == "symmetric Gauss-Seidel") {
        omega = List_.get("coarse: damping factor",1.0);
        nodal_smoother=(void *) ML_Gen_Smoother_SymGaussSeidel;
        nodal_args_ = ML_Smoother_Arglist_Create(2);
        edge_args_ = ML_Smoother_Arglist_Create(2);
        ML_Smoother_Arglist_Set(nodal_args_, 0, &nodal_its);
        ML_Smoother_Arglist_Set(nodal_args_, 1, &omega);
        edge_smoother=(void *) ML_Gen_Smoother_SymGaussSeidel;
        ML_Smoother_Arglist_Set(edge_args_, 0, &edge_its);
        ML_Smoother_Arglist_Set(edge_args_, 1, &omega);
        sprintf(subsmDetails,"edge sweeps=%d,node sweeps=%d,omega=%3.1e",
                edge_its,nodal_its,omega);
      }
      else if (Comm().MyPID() == 0)
        std::cerr << ErrorMsg_ << "Only Chebyshev(or MLS) and SGS are supported as "
             << "Hiptmair subsmoothers ... not " << SubSmootherType << std::endl;

      int hiptmair_type = (int)
             List_.get("smoother: Hiptmair efficient symmetric", true);

     if( verbose_ ) std::cout << msg << "Hiptmair " << "(outer sweeps="
                          << NumSmootherSteps << ","
                          << PreOrPostSmoother << "," << SubSmootherType
                          << "," << subsmDetails << ")" << std::endl;
     ML_Gen_Smoother_Hiptmair2(ml_, logical_level,pre_or_post,
                                NumSmootherSteps, Tmat_array, Tmat_trans_array, NULL,
                                MassMatrix_array,TtATMatrixML_,
                                edge_smoother, edge_args_, nodal_smoother, nodal_args_,
                                hiptmair_type);

      ML_Smoother_Arglist_Delete(&nodal_args_);
      ML_Smoother_Arglist_Delete(&edge_args_);


  } else if( CoarseSolution == "SuperLU" ) 
    ML_Gen_CoarseSolverSuperLU( ml_, LevelID_[NumLevels_-1]);
  else if( CoarseSolution == "Amesos-LAPACK" ) {
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_LAPACK, MaxProcs, AddToDiag);
  }
  else if( CoarseSolution == "Amesos-KLU" ) {
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_KLU, MaxProcs, AddToDiag);
  } else if( CoarseSolution == "Amesos-UMFPACK" )
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_UMFPACK, MaxProcs, AddToDiag);
  else if(  CoarseSolution == "Amesos-Superludist" )
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_SUPERLUDIST, MaxProcs, AddToDiag);
  else if(  CoarseSolution == "Amesos-Superlu" )
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_SUPERLU, MaxProcs, AddToDiag);
  else if( CoarseSolution == "Amesos-MUMPS" )
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_MUMPS, MaxProcs, AddToDiag);
  else if( CoarseSolution == "Amesos-ScaLAPACK" )
    ML_Gen_Smoother_Amesos(ml_, LevelID_[NumLevels_-1], 
                           ML_AMESOS_SCALAPACK, MaxProcs, AddToDiag);

 else if( CoarseSolution == "block Gauss-Seidel" ) {
    if( verbose_ ) std::cout << msg << "block Gauss-Seidel (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << "numPDEs="
                        << NumPDEEqns_ << ")" << std::endl;
    ML_Gen_Smoother_BlockGaussSeidel(ml_, LevelID_[NumLevels_-1], pre_or_post,
                                        NumSmootherSteps, Omega, NumPDEEqns_);

  } else if( CoarseSolution == "symmetric block Gauss-Seidel" ) {
    if( verbose_ ) std::cout << msg << "symmetric block Gauss-Seidel (sweeps="
                        << NumSmootherSteps << ",omega=" << Omega
                        << "," << PreOrPostSmoother << "numPDEs="
                        << NumPDEEqns_ << ")" << std::endl;
    ML_Gen_Smoother_SymBlockGaussSeidel(ml_, LevelID_[NumLevels_-1],pre_or_post,
                                        NumSmootherSteps, Omega, NumPDEEqns_);

  } else if(CoarseSolution == "user-defined"||CoarseSolution == "user defined"){
    // ============ //
    // user-defined //
    // ============ //

    int (*userSmootherPtr)(ML_Smoother *, int, double *, int, double *);
    userSmootherPtr = NULL;
    userSmootherPtr = List_.get("coarse: user-defined function",
                                userSmootherPtr);
    std::string userSmootherName;
    userSmootherName = List_.get("coarse: user-defined name", "User-defined");

    if( verbose_ ) std::cout << msg << userSmootherName << " (sweeps=" 
			 << NumSmootherSteps << ", " << PreOrPostSmoother << ")" << std::endl;

    if (userSmootherPtr == NULL) {
      if (Comm().MyPID() == 0)
        std::cerr << ErrorMsg_
             << "No pointer to user-defined smoother function found." << std::endl;
      ML_EXIT(EXIT_FAILURE);
    }
    ML_Operator *data;
    ML_Get_Amatrix(ml_, LevelID_[NumLevels_-1], &data);
    ML_Set_Smoother(ml_, LevelID_[NumLevels_-1], pre_or_post, data,
                    userSmootherPtr,
                    const_cast<char *>(userSmootherName.c_str()));

  } else if( CoarseSolution == "do-nothing" ) {
    if( verbose_ ) std::cout << msg << "No Coarse Solve" << std::endl;

    // do nothing, ML will not use any coarse solver 
  } else {
    if( Comm().MyPID() == 0 ) {
      std::cout << ErrorMsg_ << "specified options for coarse solver ("
           << CoarseSolution << ") not valid. Should be:" << std::endl;
      std::cout << ErrorMsg_ << "<Jacobi> / <Gauss-Seidel> / <symmetric Gauss-Seidel /" << std::endl;
      std::cout << ErrorMsg_ << "<MLS> / <Hiptmair> / <Amesos-LAPACK> / <Amesos-KLU> /" << std::endl;
      std::cout << ErrorMsg_ << "<Amesos-UMFPACK> / <Amesos-Superludist> / <Amesos-Superlu> /" << std::endl;
      std::cout << ErrorMsg_ << "<Amesos-MUMPS> / <block Gauss-Seidel> /" << std::endl;
      std::cout << ErrorMsg_ << "<symmetric block Gauss-Seidel> / <user-defined>"
           << std::endl;
    }

    ML_EXIT(-1);
  }
    
  return 0;
}

// ============================================================================
/*! Values for \c "aggregation: type"
 * - \c Uncoupled-MIS
 * - \c METIS
 * - \c ParMETIS
 * - \c Uncoupled
 * - \c Coupled (deprecated)
 * - \c MIS
 * - \c user
 * - \c greedy
 */
#include "ml_agg_user.h"
int ML_Epetra::MultiLevelPreconditioner::SetAggregation() 
{

  char parameter[80];
  
  int value = -777; /* pagina 777 di televideo */
  std::string CoarsenScheme = List_.get("aggregation: type","Uncoupled");

  if (CoarsenScheme == "Uncoupled-MIS")
      ML_Aggregate_Set_CoarsenScheme_UncoupledMIS(agg_);
  else if (CoarsenScheme == "VBMETIS")   // Not advertised in manual
  {
     ML_Aggregate_Set_CoarsenScheme_VBMETIS(agg_);
     int  nblocks   = List_.get("aggregation: nblocks",0);
     int  blockdim  = List_.get("aggregation: length blocks",0);
     int* blocks    = List_.get("aggregation: blocks", (int*)NULL);
     int* block_pde = List_.get("aggregation: block_pde", (int*)NULL);
     ML_Aggregate_Set_Vblocks_CoarsenScheme_VBMETIS(agg_,0,NumLevels_,nblocks,
                                                    blocks,block_pde,blockdim);
     int nodesperagg = List_.get("aggregation: nodes per aggregate",0);
     if (!nodesperagg) {
       std::cout << "*ML*WRN* aggregation: nodes per aggregate no set, using 9\n";
       nodesperagg = 9;
     }
     for (int i=0; i<NumLevels_; ++i)
       ML_Aggregate_Set_NodesPerAggr(ml_,agg_,i,nodesperagg);
  }
  else {
     for( int level=0 ; level<NumLevels_-1 ; ++level ) {  
   
       sprintf(parameter, "aggregation: type (level %d)",
           LevelID_[level]);
       CoarsenScheme = List_.get(parameter,CoarsenScheme);

       if (CoarsenScheme == "METIS")
         ML_Aggregate_Set_CoarsenSchemeLevel_METIS(level,NumLevels_,agg_);
       else if (CoarsenScheme == "ParMETIS") 
         ML_Aggregate_Set_CoarsenSchemeLevel_ParMETIS(level,NumLevels_,agg_);
       else if (CoarsenScheme == "Zoltan")
         ML_Aggregate_Set_CoarsenSchemeLevel_Zoltan(level,NumLevels_,agg_);
       else if (CoarsenScheme == "MIS")
         ML_Aggregate_Set_CoarsenSchemeLevel_MIS(level,NumLevels_,agg_);
       else if (CoarsenScheme == "Uncoupled") 
         ML_Aggregate_Set_CoarsenSchemeLevel_Uncoupled(level,NumLevels_,agg_);
       else if (CoarsenScheme == "Coupled") 
         ML_Aggregate_Set_CoarsenSchemeLevel_Coupled(level,NumLevels_,agg_);
       else if (CoarsenScheme == "user") 
         ML_Aggregate_Set_CoarsenSchemeLevel_User(level,NumLevels_,agg_);
#ifdef MARZIO
       else if (CoarsenScheme == "greedy") {
         // MS // this is delicate as it burns the "user" aggregation,
         // MS // should we fix it? 
         // MS // 05-Dec-04
         ML_SetUserLabel(ML_Aggregate_GreedyLabel);
         ML_SetUserPartitions(ML_Aggregate_Greedy);
         ML_Aggregate_Set_CoarsenSchemeLevel_User(level,NumLevels_,agg_);
       }
#endif
       else {
         if( Comm().MyPID() == 0 ) {
           std::cout << ErrorMsg_ << "specified options ("
                << CoarsenScheme << ") not valid. Should be:" << std::endl;
           std::cout << ErrorMsg_ << "<METIS> / <ParMETIS> / <Zoltan> /<MIS> / <Uncoupled> / <Coupled> / <user>" << std::endl;
         }
         exit( EXIT_FAILURE );
       }

       /* FIXME then DELETEME: check that user still works,
        * then delete this part
       if (CoarsenScheme == "Zoltan" || CoarsenScheme == "user") {
         // This copies the coordinates if the aggregation scheme
         // of at least one level is Zoltan. Coordinates will be
         // projected for ALL levels independently of the 
         // aggregation scheme.

         double * coord = List_.get("aggregation: coordinates", (double *)0);
         int NumDimensions = List_.get("aggregation: dimensions", 0);

         ML_Aggregate_Set_NodalCoordinates(ml_, agg_, coord);
         ML_Aggregate_Set_Dimensions(agg_, NumDimensions);

       }
       */

       if( CoarsenScheme == "METIS" || CoarsenScheme == "ParMETIS" ||
           CoarsenScheme == "Zoltan" ) {
 
         bool isSet = false;

         // first look for parameters without any level specification

         sprintf(parameter, "%s","aggregation: global aggregates"); 
         if( List_.isParameter(parameter) ){
           value = -777; // simply means not set
           value = List_.get(parameter,value);
           if( value != -777 ) {
             ML_Aggregate_Set_GlobalNumber(ml_,agg_,LevelID_[level],value );
             isSet = true;
           }
         }

         sprintf(parameter, "%s","aggregation: local aggregates");
         if( List_.isParameter(parameter) ){
           value = -777;
           value = List_.get(parameter,value);
           if( value != -777 ) {
             ML_Aggregate_Set_LocalNumber(ml_,agg_,LevelID_[level],value );
             isSet = true;
           }
             }

         sprintf(parameter, "%s","aggregation: nodes per aggregate");
         if( List_.isParameter(parameter) ){
           value = -777;
           value = List_.get(parameter,value);
           if( value != -777 ) {
             ML_Aggregate_Set_NodesPerAggr(ml_,agg_,LevelID_[level],value );
             isSet = true;
           }
         }

         // now for level-specific data

         sprintf(parameter, "aggregation: global aggregates (level %d)", 
                 LevelID_[level]);
         if( List_.isParameter(parameter) ){
           value = -777; // simply means not set
           value = List_.get(parameter,value);
           if( value != -777 ) {
             ML_Aggregate_Set_GlobalNumber(ml_,agg_,LevelID_[level],value );
             isSet = true;
           }
         }

         sprintf(parameter, "aggregation: local aggregates (level %d)", 
                 LevelID_[level]);
         if( List_.isParameter(parameter) ){
           value = -777;
           value = List_.get(parameter,value);
           if( value != -777 ) {
             ML_Aggregate_Set_LocalNumber(ml_,agg_,LevelID_[level],value );
             isSet = true;
           }
         }

         sprintf(parameter, "aggregation: nodes per aggregate (level %d)", 
                 LevelID_[level]);
         if( List_.isParameter(parameter) ){
           value = -777;
           value = List_.get(parameter,value);
           if( value != -777 ) {
             ML_Aggregate_Set_NodesPerAggr(ml_,agg_,LevelID_[level],value );
             isSet = true;
           }
         }

         if( isSet == false ) {
           // put default values
           sprintf(parameter, "aggregation: local aggregates (level %d)", 
                   LevelID_[level]);
           value = List_.get(parameter,1);
           ML_Aggregate_Set_LocalNumber(ml_,agg_,LevelID_[level],value);
         }

       } // if( CoarsenScheme == "METIS" || CoarsenScheme == "ParMETIS" )

     } /* for */
  } /* else */

  return 0;
}

// ================================================ ====== ==== ==== == =

int ML_Epetra::MultiLevelPreconditioner::SetPreconditioner() 
{
  std::string str = List_.get("prec type","MGV");

  if( str == "one-level-postsmoothing" ) {
    
    sprintf(Label_,  "%s","1-level postsmoothing only");
    ml_->ML_scheme = ML_ONE_LEVEL_DD;

  } else if( str == "two-level-additive" ) {

    sprintf(Label_,  "%s","two-level additive DD");
    ml_->ML_scheme = ML_TWO_LEVEL_DD_ADD;
    if( NumLevels_ != 2 ) {
      if( Comm().MyPID() == 0 ) {
    std::cerr << ErrorMsg_ << "You asked for `two-level additive DD' but you don't have" << std::endl
         << ErrorMsg_ << "exacty two levels. Now continue, but check you input..." << std::endl;
      }
    }

  } else if( str == "two-level-hybrid") {

    sprintf(Label_,  "%s","two-level hybrid DD");
    ml_->ML_scheme = ML_TWO_LEVEL_DD_HYBRID;
    if( NumLevels_ != 2 ) {
      if( Comm().MyPID() == 0 ) {
    std::cerr << ErrorMsg_ << "You asked for `two-level hybrid DD' but you don't have" << std::endl
         << ErrorMsg_ << "exacty two levels. Now continue, but check you input..." << std::endl;
      }
    }

  } else if( str == "two-level-hybrid2") {

    sprintf(Label_,  "%s","two-level hybrid DD (2)");
    ml_->ML_scheme = ML_TWO_LEVEL_DD_HYBRID_2;
    if( NumLevels_ != 2 ) {
      if( Comm().MyPID() == 0 ) {
    std::cerr << ErrorMsg_ << "You asked for `two-level hybrid DD (2)' but you don't have" << std::endl
         << ErrorMsg_ << "exacty two levels. Now continue, but check you input..." << std::endl;
      }
    }

  } else if( str == "full-MGV" ) {
    ml_->ML_scheme = ML_MGFULLV;

  } else if( str == "MGW" ) {
    ml_->ML_scheme = ML_MGW;

  } else if( str == "projected MGV" ) {
    sprintf(Label_,  "%s","projected MGV");
    ml_->ML_scheme = ML_PAMGV;

    int numModes = List_.get("number of projected modes",0);
    double **periodicModes = List_.get("projected modes", (double **) 0);

    // Check that the number of modes is 1-3, and that the modes are there. 
    if (numModes < 1 || numModes > 3) {
      std::cerr << ErrorMsg_ <<
        "You have chosen `projected MGV', but `number of projected modes'"
        << std::endl << ErrorMsg_ <<
        " has an incorrect value.  It should be 1, 2 or 3." << std::endl;
      exit( EXIT_FAILURE );
    }
    if (periodicModes == 0) {
      std::cerr << ErrorMsg_ <<
        "You have chosen `projected MGV', but `projected modes' is NULL."
        << std::endl; 
      exit( EXIT_FAILURE );
    }
    for (int i=0; i<numModes; i++) {
      if (periodicModes[i] == 0) {
        std::cerr << ErrorMsg_ <<
          "You have chosen `projected MGV', but mode " << i+1 << " is NULL."
          << std::endl; 
        exit( EXIT_FAILURE );
      }
    }

    //JJH 7-22-05  I think NumMyRows is the right value...
    ML_Operator_SetSubspace(ml_, periodicModes, numModes,
                            RowMatrix_->NumMyRows());

  } else if( str == "MGV" ) {
    // it is the default
    ml_->ML_scheme = ML_MGV;
  } else {

    if( Comm().MyPID() == 0 ) {
      std::cerr << ErrorMsg_ << "`prec type' has an incorrect value of '"
       << str << "'. It should be" << std::endl
       << ErrorMsg_ << "<one-level-postsmoothing> / <two-level-additive>" << std::endl
       << ErrorMsg_ << "<two-level-hybrid> / <two-level-hybrid2>" << std::endl;
    }
    exit( EXIT_FAILURE );
    
  }
  
  return 0;
}

// ================================================ ====== ==== ==== == =

int ML_Epetra::MultiLevelPreconditioner::SetSmoothingDamping() 
{

  Epetra_Time Time(Comm());
  
  // Check for additional smoothing of prolongator.  This should be used in
  // conjunction with more aggressive coarsening.
  int prolongatorSmSweeps = List_.get("aggregation: smoothing sweeps", 1);
  ML_Aggregate_Set_DampingSweeps(agg_,prolongatorSmSweeps);
  //ML_Aggregate_Set_BlockDiagScaling(agg_);

  /* ********************************************************************** */
  /* Strategies to determine the field-of-values.                           */
  /* almost everything here is experimental ;)                              */
  /* ********************************************************************** */

  std::string RandPSmoothing = List_.get("R and P smoothing: type", "classic");

  /* start looping over different options */

  if( RandPSmoothing == "classic" ) {

    /* ********************************************************************** */
    /* For "classical" approach to determine lambda_max only.                 */
    /* ********************************************************************** */

    ML_CHK_ERR(SetSmoothingDampingClassic());
    
  } else if( RandPSmoothing == "advanced" ) {

    /* ********************************************************************** */
    /* This is the new way, based on Anasazi to compute eigen-widgets         */
    /* ********************************************************************** */

    //    if( verbose_ )
    //      std::cout << PrintMsg_ << "Use A to smooth restriction operator" << std::endl;
    agg_->Restriction_smoothagg_transpose = ML_TRUE;

    // fix default values in List_
    List_.set("eigen-analysis: use symmetric algorithm",false);
    List_.set("eigen-analysis: tolerance", 1e-2);
    List_.set("eigen-analysis: use diagonal scaling", true);
    List_.set("eigen-analysis: restart", 100);
    List_.set("eigen-analysis: length", 20);
    List_.set("field-of-values: tolerance", 1e-2);
    List_.set("field-of-values: use diagonal scaling", true);
    List_.set("field-of-values: restart", 100);
    List_.set("field-of-values: ", 20);
    List_.set("field-of-values: print current status", false);
    List_.set("ML output",0);

    struct ML_Field_Of_Values* field_of_values;

    // stick default values (undefined)
    field_of_values = (struct ML_Field_Of_Values *) ML_allocate( sizeof(struct ML_Field_Of_Values) );
    field_of_values->eta     = 0.0;
    field_of_values->real_max= -1.0;
    field_of_values->imag_max= -1.0;
    field_of_values->poly_order = 0;

    if( List_.get("aggregation: compute field of values",true) )
      field_of_values->compute_field_of_values = ML_YES;
    else
      field_of_values->compute_field_of_values = ML_NO;
    
    if( List_.get("aggreation: compute field of values for non-scaled",false) )
      field_of_values->compute_field_of_values_non_scaled = ML_YES;
    else
      field_of_values->compute_field_of_values_non_scaled = ML_NO;

    // following values for choice:
    // -1 : undefined
    //  0 : do nothing, put eta = 0 (used in non-smoothed aggregation)
    //  1 : compute the box of the field of values (eta = imag_max/real_max)
    //  2 : compute ||lambda_max|| (eta = std::sqrt(imag_max^2 + real_max^2))
    field_of_values->choice     = -1;
    // and this is a pointer for the object's interal ParameterList
    field_of_values->EigenList = (void *) &List_;

    // still to set up polynomial coeffiecients
    std::string DampingType =  List_.get("R and P smoothing: damping", "default");
  
    if( DampingType == "non-smoothed" ) {

      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : non-smoothed aggregation" << std::endl;

      field_of_values->choice     =  0;
      field_of_values->poly_order =  0;
      // I don't really need them, smoothing will be set to zero
      field_of_values->R_coeff[0] =  0.0;
      field_of_values->R_coeff[1] =  0.0;
      field_of_values->R_coeff[2] =  0.0;
      
      field_of_values->P_coeff[0] =  0.0;
      field_of_values->P_coeff[1] =  0.0;
      field_of_values->P_coeff[2] =  0.0;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);

      agg_->Restriction_smoothagg_transpose = ML_FALSE;      
      
    } else if( DampingType == "almost-non-smoothed" ) {

      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : almost non-smoothed aggregation" << std::endl;

      field_of_values->choice     =  1;
      field_of_values->poly_order =  0;
      // I don't really need them, smoothing will be set to zero
      field_of_values->R_coeff[0] =  0.000000001;
      field_of_values->R_coeff[1] =  0.0;
      field_of_values->R_coeff[2] =  0.0;
      
      field_of_values->P_coeff[0] =  0.000000001;
      field_of_values->P_coeff[1] =  0.0;
      field_of_values->P_coeff[2] =  0.0;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "fov-1" ) {

      // those are the coefficients proposed by Ray
      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : Using `fov-1' values" << std::endl;
      
      field_of_values->choice     =  1;
      field_of_values->poly_order =  2;
      
      field_of_values->R_coeff[0] =  1.107;
      field_of_values->R_coeff[1] =  0.285;
      field_of_values->R_coeff[2] =  0.718;
      
      field_of_values->P_coeff[0] =  1.878;
      field_of_values->P_coeff[1] = -2.515;
      field_of_values->P_coeff[2] =  0.942;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "fov-2" ) {

      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : Using `fov-2' values" << std::endl;

      field_of_values->choice     =  1;
      field_of_values->poly_order =  2;
      
      field_of_values->R_coeff[0] =  1.138;
      field_of_values->R_coeff[1] =  1.162;
      field_of_values->R_coeff[2] = -2.384;
      
      field_of_values->P_coeff[0] =  2.143;
      field_of_values->P_coeff[1] = -2.179;
      field_of_values->P_coeff[2] =  0.101;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "fov-5" ) {

      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : Using `fov-5' values" << std::endl;
   
      field_of_values->choice     =  1;
      field_of_values->poly_order =  2;

      field_of_values->R_coeff[0] = 1.631;
      field_of_values->R_coeff[1] = -3.9015;
      field_of_values->R_coeff[2] = 2.5957;
      
      field_of_values->P_coeff[0] = 1.00145;
      field_of_values->P_coeff[1] = -1.4252;
      field_of_values->P_coeff[2] = 0.6627;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "fov-10" ) {

      // those are Marzio's best values ;^)
      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : Using `fov-10' values" << std::endl;

      field_of_values->choice     =  1;
      field_of_values->poly_order =  2;


      field_of_values->R_coeff[0] = 1.768909e+00;
      field_of_values->R_coeff[1] = -4.132227e+00;
      field_of_values->R_coeff[2] = 2.669318e+00;
      field_of_values->P_coeff[0] = 1.619455e+00;
      field_of_values->P_coeff[1] = -2.347773e+00;
      field_of_values->P_coeff[2] = 8.652273e-01;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "random" ) {

      // only to play with
      if( verbose_ )
        std::cout << PrintMsg_ << "R and P smoothing : Using `random' values" <<std::endl;

      field_of_values->choice     =  1;
      field_of_values->poly_order =  2;
        
      // initialize seed
      unsigned int s = (int)(Time.ElapsedTime()*10000);
      srand(s);
      
      // put random values
      field_of_values->R_coeff[0] =  (double)rand()/RAND_MAX;
      field_of_values->R_coeff[1] =  (double)rand()/RAND_MAX;
      field_of_values->R_coeff[2] =  (double)rand()/RAND_MAX;
      
      field_of_values->P_coeff[0] =  (double)rand()/RAND_MAX;
      field_of_values->P_coeff[1] =  (double)rand()/RAND_MAX;
      field_of_values->P_coeff[2] =  (double)rand()/RAND_MAX;

      if( verbose_ ) {
        std::cout << PrintMsg_ << "Random coefficients for R and P:" << std::endl
             << PrintMsg_ << field_of_values->R_coeff[0] << "   "
             << field_of_values->R_coeff[1] << "   "
             << field_of_values->R_coeff[2] << std::endl
             << PrintMsg_ << field_of_values->P_coeff[0] << "   "
             << field_of_values->P_coeff[1] << "   "
             << field_of_values->P_coeff[2] << "   (seed = "
             << s << ")" << std::endl;
      }

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "classic" ) {

      // This must be as with classic ML approach.
      // It can be used to estimate the field of value, tough.
      
      if( verbose_ )
        std::cout << PrintMsg_ << "R and P smoothing : Using `classic'" << std::endl;

      // First set damping as usual
      SetSmoothingDampingClassic();

      agg_->Restriction_smoothagg_transpose = ML_FALSE;      

    }  else if( DampingType == "classic-use-A" ) {

      if( verbose_ )
        std::cout << PrintMsg_ << "R and P smoothing : Using `classic-use-A'" <<std::endl;

      field_of_values->choice     =  2;
      field_of_values->poly_order =  2;
      
      field_of_values->R_coeff[0] =  1.333;
      field_of_values->R_coeff[1] =  0.0;
      field_of_values->R_coeff[2] =  0.0;
      
      field_of_values->P_coeff[0] =  1.333;
      field_of_values->P_coeff[1] =  0.0;
      field_of_values->P_coeff[2] =  0.0;

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else if( DampingType == "user-defined") {
      
      if( verbose_ )
    std::cout << PrintMsg_ << "R and P smoothing : Using `user-defined'" << std::endl;

      // user may specify each coefficient. Default values as for fov-1
      field_of_values->choice     =  1;
      field_of_values->poly_order =  2;
      
      // get them from parameters' list.
      field_of_values->R_coeff[0] =  List_.get("R and P smoothing: c_0",  1.107);
      field_of_values->R_coeff[1] =  List_.get("R and P smoothing: c_1",  0.285);
      field_of_values->R_coeff[2] =  List_.get("R and P smoothing: c_2",  0.718);

      field_of_values->P_coeff[0] =  List_.get("R and P smoothing: g_0",  1.878);
      field_of_values->P_coeff[1] =  List_.get("R and P smoothing: g_1", -2.515);
      field_of_values->P_coeff[2] =  List_.get("R and P smoothing: g_2",  0.942);

      ML_Aggregate_Set_DampingFactor(agg_,0.0);
      
    } else {

      if( Comm().MyPID() == 0 ) {
    std::cerr << std::endl;
    std::cerr << ErrorMsg_ << "Parameter for `R and P smoothing : damping' not recognized" << std::endl
         << ErrorMsg_ << "It is: `" << DampingType << "'. It should be one of:" << std::endl
         << ErrorMsg_ << "<fov-1> / <fov-2> / <fov-5> / <fov-10> / <user-defined>" << std::endl;
      }

      ML_EXIT(-10); // wrong input parameter
    }

    agg_->field_of_values = (void*) field_of_values;  

  } else {

    if( Comm().MyPID() == 0 ) {
      std::cerr << std::endl;
      std::cerr << ErrorMsg_ << "Parameter for `R and P smoothing : type' not recognized" << std::endl
       << ErrorMsg_ << "It is: `" << RandPSmoothing << "'. It should be one of:" << std::endl
       << ErrorMsg_ << "<classic> / <advanced>" << std::endl;
    }
    
    ML_EXIT(-11); // wrong input parameter
    
  }

  return 0;
}

// ============================================================================

int ML_Epetra::MultiLevelPreconditioner::SetSmoothingDampingClassic()
{
  
  double DampingFactor = 1.333;
  if( SolvingMaxwell_ ) DampingFactor = 0.0;

  DampingFactor = List_.get("aggregation: damping factor", 
                DampingFactor);
  ML_Aggregate_Set_DampingFactor( agg_, DampingFactor );
  
  if( verbose_ ) {
    std::cout << PrintMsg_ << "R and P smoothing : P = (I-\\omega A) P_t, R = P^T" << std::endl;
    std::cout << PrintMsg_ << "R and P smoothing : \\omega = " << DampingFactor << "/lambda_max" <<std::endl;
  }
    
#ifdef no_longer_done_here
  /*********************************************************************/
  /* Changed the code so that the eigen-analysis type is no longer     */
  /* connected to aggregates but is instead connected to matrices. This*/
  /* means that the eigen-analysis stuff is parsed and set elsewhere.  */
  /*********************************************************************/

  std::string str = List_.get("eigen-analysis: type","cg");
  
  if( verbose_ ) std::cout << PrintMsg_ << "Using `" << str << "' scheme for eigen-computations" << std::endl;
  
  if( str == "cg" )                ML_Aggregate_Set_SpectralNormScheme_Calc(agg_);
  else if( str == "Anorm" )        ML_Aggregate_Set_SpectralNormScheme_Anorm(agg_);
  else if( str == "Anasazi" )      ML_Aggregate_Set_SpectralNormScheme_Anasazi(agg_);
  else if( str == "power-method" ) ML_Aggregate_Set_SpectralNormScheme_PowerMethod(agg_);
  else {
    if( Comm().MyPID() == 0 ) {
      std::cerr << ErrorMsg_ << "parameter `eigen-analysis: type' has an incorrect value"
       << "(" << str << ")" << std::endl;
      std::cerr << ErrorMsg_ << "It should be: " << std::endl
       << ErrorMsg_ << "<cg> / <Anorm> / <Anasazi> / <power-method>" << std::endl;
    }
    ML_EXIT(-10); // wrong input parameter
  }
#endif
    
  return 0;
}
 
// ============================================================================

int ML_Epetra::MultiLevelPreconditioner::
PrintStencil2D(const int nx, const int ny, 
           int NodeID,
           const int EquationID)
{

  if (nx <= 0)
    ML_CHK_ERR(-1); // need nodes along the X-axis
 
  if (ny <= 0)
    ML_CHK_ERR(-2); // need nodes along Y-axis


  if (RowMatrix_ == 0) 
    ML_CHK_ERR(-3); // matrix still not set

  // automatically compute NodeID, somewhere in the middle of the grid
  if (NodeID == -1) {
    if (ny == 1) 
      NodeID = (int)(nx/2);
    else 
      NodeID = (int)(ny*(nx/2) + nx/2);
  }
  
  // need to convert from NodeID (BlockRowID) to PointRowID
  int GID = NodeID * NumPDEEqns_;
  
  int LID = RowMatrix_->RowMatrixRowMap().LID(GID);

  // only processor having this node will go on
  if (LID == -1)
    return(0);

  int MaxPerRow = RowMatrix_->MaxNumEntries();
  int NumEntriesRow;   // local entries on each row
  std::vector<double> Values;  Values.resize(MaxPerRow);
  std::vector<int>    Indices; Indices.resize(MaxPerRow);
  
  int ierr = RowMatrix_->ExtractMyRowCopy(LID, MaxPerRow, NumEntriesRow,
                      &Values[0], &Indices[0]);

  if (ierr) 
    ML_CHK_ERR(-4);

  // cycle over nonzero elements, look for elements in positions that we
  // can understand
  
  Epetra_IntSerialDenseMatrix StencilInd(3,3);
  Epetra_SerialDenseMatrix StencilVal(3,3);
  for( int i=0 ; i<3 ; ++i ) 
    for( int j=0 ; j<3 ; ++j ) {
      StencilVal(i,j) = 0.0;
    }
  
  // look for the following positions
  StencilInd(0,0) = RowMatrix_->RowMatrixColMap().LID(NodeID-1-nx);
  StencilInd(1,0) = RowMatrix_->RowMatrixColMap().LID(NodeID-nx);
  StencilInd(2,0) = RowMatrix_->RowMatrixColMap().LID(NodeID+1-nx);
  StencilInd(0,1) = RowMatrix_->RowMatrixColMap().LID(NodeID-1);
  StencilInd(1,1) = RowMatrix_->RowMatrixColMap().LID(NodeID);
  StencilInd(2,1) = RowMatrix_->RowMatrixColMap().LID(NodeID+1);
  StencilInd(0,2) = RowMatrix_->RowMatrixColMap().LID(NodeID-1+nx);
  StencilInd(1,2) = RowMatrix_->RowMatrixColMap().LID(NodeID+nx);
  StencilInd(2,2) = RowMatrix_->RowMatrixColMap().LID(NodeID+1+nx);

  for( int i=0 ; i<NumEntriesRow ; ++i ) {
    // get only the required equation
    if( Indices[i]%NumPDEEqns_ ) continue;
    // convert into block row
    int LocalColID = Indices[i]/NumPDEEqns_;
    // look for known positions
    for( int ix=0 ; ix<3 ; ++ix ) {
      for( int iy=0 ; iy<3 ; ++iy ) {
    if( StencilInd(ix,iy) == LocalColID ) {
      StencilVal(ix,iy) = Values[i];
    }
      }
    }
  }
  
  std::cout << "2D computational stencil for equation " << EquationID << " at node " << NodeID
       << " (grid is " << nx << " x " << ny << ")" << std::endl;
  std::cout << std::endl;
  for( int iy=0 ; iy<3 ; ++iy ) {
    std::cout << "\t";
    for( int ix=0 ; ix<3 ; ++ix ) {
      std::cout << " " << std::setw(15) << StencilVal(ix,iy);
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  
  return 0;
}

// ============================================================================

void ML_Epetra::MultiLevelPreconditioner::Apply_BCsToGradient(
             const Epetra_RowMatrix & iEdgeMatrix,
             const Epetra_RowMatrix & iGrad)
{
  /* This function zeros out *rows* of T that correspond to Dirichlet rows in
     the curl-curl matrix.  It mimics what was done previously in
     ML_Tmat_applyDirichletBC().
     Input:
         EdgeMatrix         curl-curl matrix
         Grad               gradient matrix
     Output:
         Grad               gradient matrix with *rows* zeroed out

     Comments: The graph of Grad is unchanged.
  */

  const Epetra_CrsMatrix *EdgeMatrix = dynamic_cast<const Epetra_CrsMatrix*>
                                           (&iEdgeMatrix );
  const Epetra_CrsMatrix *Grad = dynamic_cast<const Epetra_CrsMatrix*>(&iGrad );

  if (EdgeMatrix == 0 || Grad == 0) {
    if (verbose_)
      std::cout << "Not applying Dirichlet boundary conditions to gradient "
           << "because cast failed." << std::endl;
    return;
  }

  // locate Dirichlet edges
  int *dirichletEdges = new int[EdgeMatrix->NumMyRows()];
  int numBCEdges = 0;
  for (int i=0; i<EdgeMatrix->NumMyRows(); i++) {
    int numEntries, *cols;
    double *vals;
    int ierr = EdgeMatrix->ExtractMyRowView(i,numEntries,vals,cols);
    if (ierr == 0) {
      int nz=0;
      for (int j=0; j<numEntries; j++) if (vals[j] != 0.0) nz++;
      if (nz == 1) {
        dirichletEdges[numBCEdges++] = i;
      }
    }
  }
  // -------------------------
  // now zero out the rows
  // -------------------------
  for (int i=0; i < numBCEdges; i++) {
    int numEntries;
    double *vals;
    int *cols;
    Grad->ExtractMyRowView(dirichletEdges[i],numEntries,vals,cols);
    for (int j=0; j < numEntries; j++)
      vals[j] = 0.0;
  }
  delete [] dirichletEdges;
} //Apply_BCsToGradient

#ifdef ML_VERSION_THAT_ZEROS_COLUMNS
// ============================================================================

void ML_Epetra::MultiLevelPreconditioner::Apply_BCsToGradient(
             const Epetra_RowMatrix & iEdgeMatrix,
             const Epetra_RowMatrix & iGrad)
{
  /* This function zeros out columns of T that are no longer in the null space
     of the curl-curl matrix because of Dirichlet boundary conditions.
     Input:
         EdgeMatrix         curl-curl matrix
         Grad               gradient matrix
     Output:
         Grad               gradient matrix with columns zeroed out

     Comments: The graph of Grad is unchanged.
  */

  const Epetra_CrsMatrix *EdgeMatrix = dynamic_cast<const Epetra_CrsMatrix*>
                                           (&iEdgeMatrix );
  const Epetra_CrsMatrix *Grad = dynamic_cast<const Epetra_CrsMatrix*>(&iGrad );

  if (EdgeMatrix == 0 || Grad == 0) {
    if (verbose_)
      std::cout << "Not applying Dirichlet boundary conditions to gradient "
           << "because cast failed." << std::endl;
    return;
  }

  // locate Dirichlet edges
  int *dirichletEdges = new int[EdgeMatrix->NumMyRows()];
  int numBCEdges = 0;
  for (int i=0; i<EdgeMatrix->NumMyRows(); i++) {
    int numEntries, *cols;
    double *vals;
    int ierr = EdgeMatrix->ExtractMyRowView(i,numEntries,vals,cols);
    if (ierr == 0) {
      int nz=0;
      for (int j=0; j<numEntries; j++) if (vals[j] != 0.0) nz++;
      if (nz == 1) {
        dirichletEdges[numBCEdges++] = i;
      }
    }
  }

  const Epetra_Map & ColMap = Grad->ColMap();
  int indexBase = ColMap.IndexBase();
  Epetra_Map globalMap(Grad->NumGlobalCols(),indexBase,EdgeMatrix->Comm());

  // create the exporter from this proc's column map to global 1-1 column map
  Epetra_Export Exporter(ColMap,globalMap);

  // create a std::vector of global column indices that we will export to
  Epetra_Vector globColsToZero(globalMap);
  // create a std::vector of local column indices that we will export from
  Epetra_Vector myColsToZero(ColMap);
  myColsToZero.PutScalar(0);

  // for each local column j in a local dirichlet row, set myColsToZero[j]=1
  for (int i=0; i < numBCEdges; i++) {
    int numEntries;
    double *vals;
    int *cols;
    Grad->ExtractMyRowView(dirichletEdges[i],numEntries,vals,cols);
    for (int j=0; j < numEntries; j++)
      myColsToZero[ cols[j] ] = 1;
  }

  // export to the global column map
  globColsToZero.Export(myColsToZero,Exporter,Add);
  // now import from the global column map to the local column map
  myColsToZero.Import(globColsToZero,Exporter,Insert);

  // -------------------------
  // now zero out the columns
  // -------------------------

  for (int i=0; i < Grad->NumMyRows(); i++) {
    int numEntries;
    double *vals;
    int *cols;
    Grad->ExtractMyRowView(i,numEntries,vals,cols);
    for (int j=0; j < numEntries; j++) {
      if (myColsToZero[ cols[j] ] > 0) {
        vals[j] = 0.0;
      }
    }
  }
  delete [] dirichletEdges;
} //Apply_BCsToGradient
#endif //ifdef ML_VERSION_THAT_ZEROS_COLUMNS

// ============================================================================

void ML_Epetra::MultiLevelPreconditioner::
CheckNullSpace()
{
  // Check that the null space is correct.
  Epetra_Vector XXX(TMatrix_->OperatorDomainMap());
  Epetra_Vector YYY(TMatrix_->OperatorRangeMap());
  Epetra_Vector ZZZ(TMatrix_->OperatorRangeMap());
  XXX.Random();
  double norm;
  XXX.Norm2(&norm);
  XXX.Scale(1.0/norm);
  TMatrix_->Multiply(false,XXX,YYY);
  CurlCurlMatrix_->Multiply(false,YYY,ZZZ);
  ZZZ.Norm2(&norm);
  int mypid = CurlCurlMatrix_->Comm().MyPID();
  if (mypid ==0 && ML_Get_PrintLevel() > 0)
    std::cout << "Checking curl/gradient relationship" << std::endl;
  double norminf = CurlCurlMatrix_->NormInf();
  if (mypid ==0 && ML_Get_PrintLevel() > 0) {
    if (norm > (1e-12 * norminf))
      std::cout << std::endl
           << "**WARNING** ||curlcurl * grad * vrand||_2 = " << norm << std::endl
           << "**WARNING** Either the curl-curl or the null space may be wrong."
           << std::endl << std::endl;
    else {
      std::cout << "||curlcurl||_inf              = " << norminf << std::endl;
      std::cout << "||curlcurl * grad * vrand||_2 = " << norm << std::endl;
    }
  }
} //CheckNullSpace()

// ============================================================================

#ifdef HAVE_ML_EPETRAEXT
Epetra_RowMatrix* ML_Epetra::MultiLevelPreconditioner::
ModifyEpetraMatrixColMap(const Epetra_RowMatrix &A,
                         EpetraExt::CrsMatrix_SolverMap &transform,
                         const char *matrixLabel)
{
    Epetra_RowMatrix *B;
    Epetra_CrsMatrix *Acrs;

    const Epetra_CrsMatrix *Atmp = dynamic_cast<const Epetra_CrsMatrix*>(&A);
    if (Atmp != 0) {
      Acrs = const_cast<Epetra_CrsMatrix*>(Atmp);
      B = &(transform(*Acrs));
    }
    else
      B = const_cast<Epetra_RowMatrix *>(&A);

    if (verbose_) {
      if (B != &A)
        printf("** Transforming column map of %s matrix\n", matrixLabel);
      else
        printf("** Leaving column map of %s matrix unchanged\n",matrixLabel);
    }

    return B;
} //ModifyEpetraMatrixColMap()
#endif


#endif /*ifdef HAVE_ML_EPETRA && HAVE_ML_TEUCHOS */
