// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
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
//
//  This test is for the BlockDavidson solver
//
#include "AnasaziConfigDefs.hpp"
#include "AnasaziTypes.hpp"

#include "AnasaziEpetraAdapter.hpp"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"

#include "AnasaziBlockDavidson.hpp"

#include "AnasaziBasicEigenproblem.hpp"
#include "AnasaziBasicOutputManager.hpp"
#include "AnasaziSVQBOrthoManager.hpp"
#include "AnasaziBasicSort.hpp"
#include "AnasaziStatusTestMaxIters.hpp"
#include "AnasaziModalSolverUtils.hpp"
#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_SerialDenseMatrix.hpp"

#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#include <mpi.h>
#else
#include "Epetra_SerialComm.h"
#endif

#include "ModeLaplace1DQ1.h"

using namespace Teuchos;
using namespace Anasazi;

typedef double                              ScalarType;
typedef ScalarTraits<ScalarType>                   SCT;
typedef SCT::magnitudeType               MagnitudeType;
typedef Epetra_MultiVector                 MV;
typedef Epetra_Operator                    OP;
typedef MultiVecTraits<ScalarType,MV>     MVT;
typedef OperatorTraits<ScalarType,MV,OP>  OPT;

class get_out : public std::logic_error {
  public: get_out(const std::string &whatarg) : std::logic_error(whatarg) {}
};

void checks( RefCountPtr<BlockDavidson<ScalarType,MV,OP> > solver, int blocksize, int numblocks, 
             RefCountPtr<Eigenproblem<ScalarType,MV,OP> > problem,
             RefCountPtr<MatOrthoManager<ScalarType,MV,OP> > ortho,
             ModalSolverUtils<ScalarType,MV,OP> &msutils) {
  BlockDavidsonState<ScalarType,MV> state = solver->getState();
  
  TEST_FOR_EXCEPTION(MVT::GetNumberVecs(*state.V)  != solver->getMaxSubspaceDim(),get_out,     "getMaxSubspaceDim() does not match allocated size for V");
  TEST_FOR_EXCEPTION(MVT::GetNumberVecs(*state.X)  != solver->getBlockSize(),get_out,"blockSize() does not match allocated size for X");
  TEST_FOR_EXCEPTION(MVT::GetNumberVecs(*state.KX) != solver->getBlockSize(),get_out,"blockSize() does not match allocated size for KX");
  if (solver->getProblem().getM() != null) {
    TEST_FOR_EXCEPTION(MVT::GetNumberVecs(*state.MX) != solver->getBlockSize(),get_out,"blockSize() does not match allocated size for MX");
  }
  else {
    TEST_FOR_EXCEPTION(state.MX != null,get_out,"MX should null; problem has no M matrix");
  }
  TEST_FOR_EXCEPTION(MVT::GetNumberVecs(*state.R)  != solver->getBlockSize(),get_out,"blockSize() does not match allocated size for R");
  TEST_FOR_EXCEPTION(solver->getBlockSize() != blocksize, get_out,"Solver block size does not match specified block size.");  
  TEST_FOR_EXCEPTION(solver->getMaxSubspaceDim()/solver->getBlockSize() != numblocks, get_out, "Solver num blaocks does not match specified num blocks.");
  TEST_FOR_EXCEPTION(&solver->getProblem() != problem.get(),get_out,"getProblem() did not return the submitted problem.");
  TEST_FOR_EXCEPTION(solver->getMaxSubspaceDim() != numblocks*blocksize,get_out,"BlockDavidson::getMaxSubspaceDim() does not match numblocks*blocksize.");

  if (solver->isInitialized()) 
  {
    // check residuals
    RefCountPtr<const MV> evecs = state.X;
    RefCountPtr<MV> Kevecs, Mevecs;
    Kevecs = MVT::Clone(*evecs,blocksize);
    OPT::Apply(*problem->getOperator(),*evecs,*Kevecs);
    if (problem->getM() == null) {
      Mevecs = MVT::CloneCopy(*evecs);
    }
    else {
      Mevecs = MVT::Clone(*evecs,blocksize);
      OPT::Apply(*problem->getM(),*evecs,*Mevecs);
    }
    vector<MagnitudeType> theta = solver->getRitzValues();
    TEST_FOR_EXCEPTION(theta.size() != (unsigned int)solver->getCurSubspaceDim(),get_out,"getRitzValues() has incorrect size.");
    SerialDenseMatrix<int,ScalarType> T(blocksize,blocksize);
    for (int i=0; i<blocksize; i++) T(i,i) = theta[i];
    // BlockDavidson computes residuals like R = K*X - M*X*T 
    MVT::MvTimesMatAddMv(-1.0,*Mevecs,T,1.0,*Kevecs);
    MagnitudeType error = msutils.errorEquality(Kevecs.get(),state.R.get());
    // residuals from BlockDavidson should be exact; we will cut a little slack
    TEST_FOR_EXCEPTION(error > 1e-14,get_out,"Residuals from solver did not match eigenvectors.");

    // check eigenvalues
    // X should be ritz vectors; they should diagonalize K to produce the current eigenvalues
    MagnitudeType ninf = T.normInf();
    OPT::Apply(*problem->getOperator(),*evecs,*Kevecs);
    MVT::MvTransMv(1.0,*evecs,*Kevecs,T);
    for (int i=0; i<blocksize; i++) T(i,i) -= theta[i];
    error = T.normFrobenius() / ninf;
    TEST_FOR_EXCEPTION(error > 1e-14,get_out,"Ritz values don't match eigenvectors.");
  }
  else {
    // not initialized
    TEST_FOR_EXCEPTION(solver->getCurSubspaceDim() != 0,get_out,"In unitialized state, getCurSubspaceDim() should be 0.");
  }
}

void testsolver( RefCountPtr<BasicEigenproblem<ScalarType,MV,OP> > problem,
                 RefCountPtr< OutputManager<ScalarType> > printer,
                 RefCountPtr< MatOrthoManager<ScalarType,MV,OP> > ortho,
                 RefCountPtr< SortManager<ScalarType,MV,OP> > sorter,
                 ParameterList &pls)
{
  // create a status tester to run for two iterations
  RefCountPtr< StatusTest<ScalarType,MV,OP> > tester = rcp( new StatusTestMaxIters<ScalarType,MV,OP>(2) );

  const int blocksize = pls.get<int>("Block Size");
  const int numblocks = pls.get<int>("Num Blocks");

  // create the solver
  RefCountPtr< BlockDavidson<ScalarType,MV,OP> > solver;
  try {
    solver = rcp( new BlockDavidson<ScalarType,MV,OP>(problem,sorter,printer,tester,ortho,pls) );
    TEST_FOR_EXCEPTION(numblocks < 2 || blocksize < 1, get_out, "Initializing with invalid parameters failed to throw exception.")
  }
  catch (std::invalid_argument ia) {
    TEST_FOR_EXCEPTION(numblocks >= 2 && blocksize >= 1, get_out, "Initializing with valid parameters unexpectadly threw exception.");
    // caught expected exception
    return;
  }

  ModalSolverUtils<ScalarType,MV,OP> msutils(printer);

  // solver should be uninitialized
  TEST_FOR_EXCEPTION(solver->isInitialized() != false,get_out,"Solver should be un-initialized after instantiation.");  
  TEST_FOR_EXCEPTION(solver->getNumIters() != 0,get_out,"Number of iterations after initialization should be zero after init.")
  TEST_FOR_EXCEPTION(solver->getAuxVecs().size() != 0,get_out,"getAuxVecs() should return empty.");
  checks(solver,blocksize,numblocks,problem,ortho,msutils);

  int cursize  = MVT::GetNumberVecs(*problem->getInitVec());
  if (cursize < blocksize) cursize = blocksize;

  // initialize solver and perform checks
  solver->initialize();
  TEST_FOR_EXCEPTION(solver->isInitialized() != true,get_out,"Solver should be initialized after call to initialize().");  
  TEST_FOR_EXCEPTION(solver->getNumIters() != 0,get_out,"Number of iterations should be zero.")
  TEST_FOR_EXCEPTION(solver->getAuxVecs().size() != 0,get_out,"getAuxVecs() should return empty.");
  TEST_FOR_EXCEPTION(solver->getCurSubspaceDim() != cursize,get_out,"after init, getCurSubspaceDim() should be size of problem->getInitVec().");
  checks(solver,blocksize,numblocks,problem,ortho,msutils);

  while ( solver->getCurSubspaceDim() < solver->getMaxSubspaceDim() ) {

    // call iterate(); solver should perform at most two iterations and return; status test should be consistent
    solver->iterate();
    TEST_FOR_EXCEPTION(solver->isInitialized() != true,get_out,"Solver should be initialized after return from iterate().");  
    TEST_FOR_EXCEPTION(solver->getNumIters() != 2 && tester->getStatus() == Passed,get_out,"Number of iterations not consistent with StatusTest return.");
    TEST_FOR_EXCEPTION(solver->getNumIters() == 2 && tester->getStatus() != Passed,get_out,"Number of iterations not consistent with StatusTest return.");
    TEST_FOR_EXCEPTION(solver->getCurSubspaceDim() != solver->getMaxSubspaceDim() && tester->getStatus() != Passed,get_out,"solver should not have returned from iterate().");
    TEST_FOR_EXCEPTION(solver->getAuxVecs().size() != 0,get_out,"getAuxVecs() should return empty.");
    TEST_FOR_EXCEPTION(solver->getCurSubspaceDim() != cursize+solver->getNumIters()*blocksize,get_out,"getCurSubspaceDim() did not grow as expected.");
    TEST_FOR_EXCEPTION(solver->getCurSubspaceDim() > solver->getMaxSubspaceDim(),get_out,"impossibly large basis.");
    checks(solver,blocksize,numblocks,problem,ortho,msutils);
    // record current size
    cursize = solver->getCurSubspaceDim();

    // reset numiters and check
    solver->resetNumIters();
    TEST_FOR_EXCEPTION(solver->getNumIters() != 0,get_out,"Number of iterations should be zero after resetNumIters().")
  }
}

int main(int argc, char *argv[]) 
{

#ifdef HAVE_MPI
  // Initialize MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  bool testFailed;
  bool verbose = false;

  CommandLineProcessor cmdp(false,true);
  cmdp.setOption("verbose","quiet",&verbose,"Print messages and results.");
  if (cmdp.parse(argc,argv) != CommandLineProcessor::PARSE_SUCCESSFUL) {
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return -1;
  }

  // create the output manager
  RefCountPtr< OutputManager<ScalarType> > printer = rcp( new BasicOutputManager<ScalarType>() );

  if (verbose) {
    printer->stream(Errors) << Anasazi_Version() << endl << endl;
  }

  //  Problem information
  int space_dim = 1;
  std::vector<double> brick_dim( space_dim );
  brick_dim[0] = 1.0;
  std::vector<int> elements( space_dim );
  elements[0] = 100;

  // Create problem
  RefCountPtr<ModalProblem> testCase = rcp( new ModeLaplace1DQ1(Comm, brick_dim[0], elements[0]) );
  //
  // Get the stiffness and mass matrices
  RefCountPtr<Epetra_CrsMatrix> K = rcp( const_cast<Epetra_CrsMatrix *>(testCase->getStiffness()), false );
  RefCountPtr<Epetra_CrsMatrix> M = rcp( const_cast<Epetra_CrsMatrix *>(testCase->getMass()), false );
  //
  // Create the initial vectors
  // For BlockDavidson, this will dictate the initial size of the basis after a call to initialize()
  const int nev = 4;
  RefCountPtr<Epetra_MultiVector> ivec = rcp( new Epetra_MultiVector(K->OperatorDomainMap(), nev) );
  ivec->Random();
  //
  // Create eigenproblem: one standard and one generalized
  RefCountPtr<BasicEigenproblem<ScalarType,MV,OP> > probstd = rcp( new BasicEigenproblem<ScalarType, MV, OP>(K, ivec) );
  RefCountPtr<BasicEigenproblem<ScalarType,MV,OP> > probgen = rcp( new BasicEigenproblem<ScalarType, MV, OP>(K, M, ivec) );
  //
  // Inform the eigenproblem that the operator A is symmetric
  probstd->setHermitian(true);
  probgen->setHermitian(true);
  //
  // Set the number of eigenvalues requested
  probstd->setNEV( nev );
  probgen->setNEV( nev );
  //
  // Inform the eigenproblem that you are finishing passing it information
  if ( probstd->setProblem() != true || probgen->setProblem() != true ) {
    if (verbose) {
      printer->stream(Errors) << "Anasazi::BasicEigenproblem::SetProblem() returned with error." << endl
                              << "End Result: TEST FAILED" << endl;	
    }
#ifdef HAVE_MPI
    MPI_Finalize() ;
#endif
    return -1;
  }

  // create the orthogonalization managers: one standard and one M-based
  RefCountPtr< MatOrthoManager<ScalarType,MV,OP> > orthostd = rcp( new SVQBOrthoManager<ScalarType,MV,OP>() );
  RefCountPtr< MatOrthoManager<ScalarType,MV,OP> > orthogen = rcp( new SVQBOrthoManager<ScalarType,MV,OP>(M) );
  // create the sort manager
  RefCountPtr< SortManager<ScalarType,MV,OP> > sorter = rcp( new BasicSort<ScalarType,MV,OP>("LR") );
  // create the parameter list specifying blocksize > nev and full orthogonalization
  ParameterList pls;

  // begin testing 
  testFailed = false;

  pls.set<int>("Num Blocks",4);

  try 
  {

    // try with blocksize == getInitVec() size
    pls.set<int>("Block Size",nev);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev,4) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev,4) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

    // try with getInitVec() too small
    pls.set<int>("Block Size",2*nev);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(2*nev,4) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(2*nev,4) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

    // try with getInitVec() == two blocks
    pls.set<int>("Block Size",nev/2);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev/2,4) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev/2,4) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

    // try with a larger number of blocks
    pls.set<int>("Block Size",nev);
    pls.set<int>("Num Blocks",20);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev,20) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev,20) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

    // try with a larger number of blocks+1
    pls.set<int>("Block Size",nev);
    pls.set<int>("Num Blocks",21);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev,21) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(nev,21) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

    // try with an invalid blocksize
    pls.set<int>("Block Size",-1);
    pls.set<int>("Num Blocks",4);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(-1,4) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(-1,4) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

    // try with an invalid numblocks
    pls.set<int>("Block Size",4);
    pls.set<int>("Num Blocks",1);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(4,1) with standard eigenproblem..." << endl;
    }
    testsolver(probstd,printer,orthostd,sorter,pls);
    if (verbose) {
      printer->stream(Errors) << "Testing solver(4,1) with generalized eigenproblem..." << endl;
    }
    testsolver(probgen,printer,orthogen,sorter,pls);

  }
  catch (get_out go) {
    printer->stream(Errors) << "Test failed: " << go.what() << endl;
    testFailed = true;
  }
  catch (std::exception e) {
    printer->stream(Errors) << "Caught unexpected exception: " << e.what() << endl;
    testFailed = true;
  }

  
#ifdef HAVE_MPI
  MPI_Finalize() ;
#endif

  if (testFailed) {
    if (verbose) {
      printer->stream(Errors) << endl << "End Result: TEST FAILED" << endl;	
    }
    return -1;
  }
  //
  // Default return value
  //
  if (verbose) {
    printer->stream(Errors) << endl << "End Result: TEST PASSED" << endl;
  }
  return 0;

}	
