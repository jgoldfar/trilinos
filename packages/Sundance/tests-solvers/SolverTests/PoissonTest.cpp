//@HEADER
// ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
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
//@HEADER

#include "Teuchos_GlobalMPISession.hpp"
#include "TSFVectorDecl.hpp"
#include "TSFInverseOperatorDecl.hpp"
#include "TSFLoadableMatrix.hpp"
#include "TSFGlobalAnd.hpp"
#include "TSFVectorType.hpp"
#include "TSFEpetraVectorType.hpp"
#include "TSFSerialVectorType.hpp"
#include "Teuchos_Time.hpp"
#include "Teuchos_MPIComm.hpp"
#include "TSFLinearSolverDecl.hpp"
#include "TSFLinearSolverBuilder.hpp"
#include "TSFDenseLUSolver.hpp"
#include "TSFMatrixLaplacian1D.hpp"
#include "TSFLinearSolverBuilder.hpp"
#include "SundancePathUtils.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"
#include "TSFLinearCombinationImpl.hpp"

#ifndef HAVE_TEUCHOS_EXPLICIT_INSTANTIATION
#include "TSFLinearOperatorImpl.hpp"
#include "TSFLinearSolverImpl.hpp"
#include "TSFInverseOperatorImpl.hpp"
#endif


using namespace Teuchos;
using namespace TSFExtended;
using namespace TSFExtendedOps;


bool runit(const VectorType<double>& vecType,
  const LinearSolver<double>& solver)
{
  typedef Teuchos::ScalarTraits<double> ST;

  /* create the range space  */
  int nLocalRows = 10;
  
  MatrixLaplacian1D builder(nLocalRows, vecType, false);

  LinearOperator<double> A = builder.getOp();

  Out::root() << "matrix is " << endl;
  Out::os() << A << endl;

  Vector<double> x = A.domain().createMember();

  Thyra::randomize(-ST::one(),+ST::one(),x.ptr().ptr());

  Out::root() << "input is " << endl;
  Out::os() << x << endl;
  Vector<double> y = A*x;

  Out::root() << "rhs is " << endl;
  Out::os() << y << endl;

  Vector<double> ans = A.range().createMember();
  
  Out::root() << "slot for solution is " << endl;
  Out::os() << ans << endl;

  LinearOperator<double> AInv = inverse(A, solver);

  ans = AInv * y;

  Out::root() << "answer is " << endl;
  Out::os() << ans << endl;
      
  double err = (x-ans).norm2();
  Out::root() << "error norm = " << err << endl;

  double tol = 1.0e-10;
    
  if (err <= tol)
  {
    Out::root() << "Poisson solve test PASSED" << endl;
    return true;
  }
  else
  {
    Out::root() << "Poisson solve test FAILED" << endl;
    return false;
  }
}


int main(int argc, char *argv[]) 
{
  bool status = 0;

  try
  {
    GlobalMPISession session(&argc, &argv);

    int nProc = session.getNProc();
    int rank = session.getRank();

    VectorType<double> epetra = new EpetraVectorType();
    VectorType<double> serial = new SerialVectorType();

    LinearSolver<double> denseLU = new DenseLUSolver();
    LinearSolver<double> amesos = LinearSolverBuilder::createSolver("amesos.xml");
    LinearSolver<double> belos_ml = LinearSolverBuilder::createSolver("belos-ml.xml");
    LinearSolver<double> belos_ifpack = LinearSolverBuilder::createSolver("belos-ifpack.xml");
    LinearSolver<double> aztec_ml = LinearSolverBuilder::createSolver("aztec-ml.xml");
    LinearSolver<double> aztec_ifpack = LinearSolverBuilder::createSolver("aztec-ifpack.xml");

    bool allOK = true;
    Out::root() << "Running Belos/ML" << endl;
    allOK = runit(epetra, belos_ml) && allOK;

    Out::root() << "Running Belos/Ifpack" << endl;
    allOK = runit(epetra, belos_ifpack) && allOK;

    Out::root() << "Running Aztec/ML" << endl;
    allOK = runit(epetra, aztec_ml) && allOK;

    Out::root() << "Running Aztec/Ifpack" << endl;
    allOK = runit(epetra, aztec_ifpack) && allOK;

    if (nProc == 1)
    {
      Out::root() << "Running Amesos (serial)" << endl;
      allOK = runit(epetra, amesos) && allOK;
    }

    if (rank==0)
    {
      Out::root() << "Running dense LU (serial)" << endl;
      allOK = runit(serial, denseLU) && allOK;
    }

    allOK = globalAnd(allOK);

    if (allOK) 
    {
      Out::root() << "all Poisson solve tests PASSED!" << endl;
    }
    else
    {
      status = -1;
      Out::root() << "some Poisson solve tests FAILED!" << endl;
    }
  }
  catch(std::exception& e)
  {
    cout << "Caught exception: " << e.what() << endl;
    return -1;
  }
  return status;
}

