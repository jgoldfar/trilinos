// @HEADER
// ***********************************************************************
// 
//              Meros: Segregated Preconditioning Package
//                 Copyright (2004) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#include "diag.h"
#include "Epetra2TSFutils.h"
#include "TSFHashtable.h"

using namespace TSF;
using namespace SPP;

int main(int argc, void** argv)
{
  
  // Set up communication stuff for parallel

  TSFMPI::init(&argc, &argv);

  Epetra_Comm *comm;
#ifdef EPETRA_MPI
  Epetra_MpiComm mpicomm(MPI_COMM_WORLD);
  comm = (Epetra_Comm *) (&mpicomm);
#else
  comm = (Epetra_Comm *) new Epetra_SerialComm();
#endif
  
  cerr << "starting saddle_diagschur" << endl;

  // Build epetra pressure and velocity maps for a simple example

  int Nvelocity = 480;
  int Npressure = 256;
  Epetra_Map *VelocityMap = new Epetra_Map(Nvelocity, 0, *comm);
  Epetra_Map *PressureMap = new Epetra_Map(Npressure, 0, *comm);
  Epetra_Map *BlockMap    = new Epetra_Map(Nvelocity + Npressure, 0, *comm);
  
  //      |    F    Bt   | 
  //      |    B    C    |
  // Read F,B, and B^T from files and store as epetra crs matrices
  
  Epetra_CrsMatrix *F_crs, *Bt_crs, *B_crs;
  Epetra_Vector *x_epet   = new Epetra_Vector(*BlockMap),*rhs_epet = new Epetra_Vector(*BlockMap);
  ReadPetraMatrix(VelocityMap, VelocityMap,  &F_crs, "../data/mac/F.serial");
  ReadPetraMatrix(VelocityMap, PressureMap, &Bt_crs, "../data/mac/Bt.serial");
  ReadPetraMatrix(PressureMap, VelocityMap,  &B_crs, "../data/mac/B.serial");
  ReadPetraVector(x_epet  , "../data/mac/init_guess");
  ReadPetraVector(rhs_epet, "../data/mac/rhs");
  // Build TSF vector spaces. 
  
  TSFVectorSpace velocitySpace = EpetraCRS2TSFVspace(F_crs);
  TSFVectorSpace pressureSpace = EpetraCRS2TSFVspace(Bt_crs);

   // Convert block matrices to TSF matrices. 
  TSFLinearOperator F_tsf  = EpetraCRS2TSF(velocitySpace,velocitySpace,F_crs);
  TSFLinearOperator Bt_tsf = EpetraCRS2TSF(pressureSpace,velocitySpace,Bt_crs);
  TSFLinearOperator B_tsf  = EpetraCRS2TSF(velocitySpace,pressureSpace,B_crs);

  // Set up 2x2 block TSF operator
  
  TSFVectorSpace  rangeBlockSpace = new TSFProductSpace(velocitySpace, pressureSpace);
  TSFVectorSpace domainBlockSpace = new TSFProductSpace(velocitySpace, pressureSpace);
  TSFLinearOperator   saddleA_tsf = new TSFBlockLinearOperator(rangeBlockSpace, domainBlockSpace);
  
  saddleA_tsf.setBlock(0,0,F_tsf);
  saddleA_tsf.setBlock(0,1,Bt_tsf);
  saddleA_tsf.setBlock(1,0,B_tsf);

  // Build a block preconditioner using X = diag(F)
  // 
  //      |  inv(F)   0   | |   I    -Bt  | |   I      0     |
  //      |    0      I   | |   0     I   | |   0   -inv(X)  |
  //
  
  // We'll do this in 4 steps.
  // 1) Build a solver for inv(F)
  // 2) Build a SchurFactory that can make the inv(X) approximation
  // 3) Build a block preconditioner factory with the F solver and Schur factory
  // 4) Make the preconditioner and get a TSFLinearOperator representing the prec. 
  
  
  // 1) Build solver for inv(F) so that it corresponds to using GMRES with ML.

  //  TSFLinearSolver FSolver;
  TSFHashtable<int, int> azOptionsF;
  TSFHashtable<int, double> azParamsF;
  azOptionsF.put(AZ_solver, AZ_gmres);
  azOptionsF.put(AZ_ml, 1);
  azOptionsF.put(AZ_ml_sym, 0);
  azOptionsF.put(AZ_ml_levels, 4);
  azOptionsF.put(AZ_precond, AZ_dom_decomp);
  azOptionsF.put(AZ_subdomain_solve, AZ_ilu);
  azParamsF.put(AZ_tol, 1e-6);
  azOptionsF.put(AZ_max_iter, 200);
  azOptionsF.put(AZ_recursive_iterate, 1);
  TSFLinearSolver FSolver = new AZTECSolver(azOptionsF,azParamsF);
  FSolver.setVerbosityLevel(4);

  TSFLinearOperator F_inv = F_tsf.inverse(FSolver);
  
  // 2) Build a Schur complement factory for getting inv(X) approximation.
  //   Using a X = C - B * Dinv * Bt, where Dinv = inv(diag(F))

  // 2 a) Build solver for inv(schurOp) so that it corresponds to using GMRES with ML.
  //      Using same ML solver as for F.
  //  TSFLinearSolver schurSolver;
  //   ML_solverData   schurSolver_data;
  //   symmetric = true;
  //   ML_TSF_defaults(schurSolver, &schurSolver_data, symmetric, X_crs);
//   int fill = 0;
//   int overlap = 0; 
//   double tol = 1.0e-8;
//   int maxiter = 100;
//   int kspace = 100;
//   // unpreconditioned GMRES
//   TSFLinearSolver schurSolver = new GMRESSolver(tol, maxiter, kspace);

  TSFHashtable<int, int> azOptionsSchur;
  TSFHashtable<int, double> azParamsSchur;
  azOptionsSchur.put(AZ_solver, AZ_gmres);
  azOptionsSchur.put(AZ_ml, 1);
  azOptionsSchur.put(AZ_ml_levels, 4);
  azOptionsSchur.put(AZ_precond, AZ_dom_decomp);
  azOptionsSchur.put(AZ_subdomain_solve, AZ_ilu);
  azParamsSchur.put(AZ_tol, 1e-6);
  azOptionsSchur.put(AZ_max_iter, 200);
  azOptionsSchur.put(AZ_recursive_iterate, 1);
  azOptionsSchur.put(AZ_output, 1);
  TSFLinearSolver schurSolver = new AZTECSolver(azOptionsSchur,azParamsSchur);

  schurSolver.setVerbosityLevel(4);
  
  // 2 b) Build a Schur complement factory of type DiagSchurFactory.
  SchurFactory sfac = new DiagSchurFactory(schurSolver);
 
  // 3) Build a preconditioner factory with the F solver and the Schur factory.
  TSFPreconditionerFactory pfac = new NSBlockPreconditionerFactory(FSolver, sfac);

  // 4) Give the preconditioner factory data (the linear operators) 
  //    and build the preconditioner.

  // 4 a) Build a DiagSchur type operator source.
  //      The operator source contains all of the linear ops needed for the preconditioner.
  //      For a DiagSchur preconditioner we just need saddleA.
  //      The operator source can build Dinv from A (or we can pass it in).
  
  TSFOperatorSource opSrc = new DiagRightOperatorSource(saddleA_tsf);

  // 4 b) Create the DiagSchur style preconditioner as a TSFLinearOperator
  TSFPreconditioner P = pfac.createPreconditioner(opSrc);
  TSFLinearOperator saddleM_tsf = P.right();
  
  // Build a map. This map is intended to go between vbr-style vectors and TSF-style vectors. 
  // Since this example does not contain VBR data, the map is pointless but needed
  // for the proper creation of an epetra wrapper around a TSF matrix.
  // Specifically, map[i] != 1 indicates that the ith variable is in the
  //                           first block.
  //               map[i] == 1 indicates that the ith variable is in the
  //                           second block.
  
  int *imap;
  imap = Vbr2TSF(Nvelocity, Npressure);
  
  // Build the epetra matrix wrapper around the TSF block matrix
  
  TSFLinearOperator2EpetraRowMatrix *saddleA_epet = 
    new TSFLinearOperator2EpetraRowMatrix(saddleA_tsf, comm, BlockMap, imap,
                                          EPETRA_MATRIX);
  
  // Build the epetra matrix wrapper around the TSF block PRECONDITIONER
  
  TSFLinearOperator2EpetraRowMatrix *saddlePrec_epet 
    = new TSFLinearOperator2EpetraRowMatrix(saddleM_tsf, comm, BlockMap,
                                            (dynamic_cast<TSFLinearOperator2EpetraRowMatrix*>
                                             (saddleA_epet))->getBlockAssignments(), 
                                            EPETRA_INVERSE);
  
  // Set up the outer iteration
  
  Epetra_LinearProblem problem(saddleA_epet, x_epet, rhs_epet);
  AztecOO solver(problem);
  solver.SetPrecOperator(saddlePrec_epet);
  solver.SetAztecOption(AZ_solver,AZ_GMRESR);
  solver.SetAztecOption(AZ_kspace, 500);
  
  cerr << "Got here in example" << endl;

  solver.Iterate(1000, 1.0E-8);
 
  cerr << "Got here (2) in example" << endl;

  delete saddlePrec_epet;
  delete x_epet;
  delete rhs_epet;
  
#ifndef EPETRA_MPI
  delete comm;
#endif
  
  TSFMPI::finalize(); 
}



