// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#include <Xpetra_Matrix.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>
#include <Xpetra_BlockedCrsMatrix.hpp>
#include <Xpetra_EpetraMultiVector.hpp>

#include "MueLu_EpetraOperator.hpp"
#include "MueLu_Level.hpp"
#include "MueLu_Utilities.hpp"

namespace MueLu {

int EpetraOperator::ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const {
  try {
    // There is no rcpFromRef(const T&), so we need to do const_cast
    const Xpetra::EpetraMultiVector eX(rcpFromRef(const_cast<Epetra_MultiVector&>(X)));
    Xpetra::EpetraMultiVector       eY(rcpFromRef(Y));

    // Generally, we assume two different vectors, but AztecOO uses a single vector
    if (X.Values() == Y.Values()) {
      // X and Y point to the same memory, use an additional vector
      RCP<Xpetra::EpetraMultiVector> tmpY = Teuchos::rcp(new Xpetra::EpetraMultiVector(eY.getMap(), eY.getNumVectors()));

      // InitialGuessIsZero in MueLu::Hierarchy.Iterate() does not zero out components, it
      // only assumes that user provided an already zeroed out vector
      bool initialGuessZero = true;
      tmpY->putScalar(0.0);

      // apply one V-cycle as preconditioner
      Hierarchy_->Iterate(eX, 1, *tmpY, initialGuessZero);

      // deep copy solution from MueLu
      eY.update(1.0, *tmpY, 0.0);

    } else {
      // X and Y point to different memory, pass the vectors through

      // InitialGuessIsZero in MueLu::Hierarchy.Iterate() does not zero out components, it
      // only assumes that user provided an already zeroed out vector
      bool initialGuessZero = true;
      eY.putScalar(0.0);

      Hierarchy_->Iterate(eX, 1, eY, initialGuessZero);
    }

  } catch (std::exception& e) {
    //TODO: error msg directly on std::cerr?
    std::cerr << "Caught an exception in MueLu::EpetraOperator::ApplyInverse():" << std::endl
        << e.what() << std::endl;
    return -1;
  }

  return 0;
}

const Epetra_Comm& EpetraOperator::Comm() const {
  RCP<Matrix> A = Hierarchy_->GetLevel(0)->Get<RCP<Matrix> >("A");

  //TODO: This code is not pretty
  RCP<Xpetra::BlockedCrsMatrix<double, int, int> > epbA = Teuchos::rcp_dynamic_cast<Xpetra::BlockedCrsMatrix<double, int, int> >(A);
  if (epbA != Teuchos::null) {
    RCP<const Xpetra::EpetraCrsMatrix> tmp_ECrsMtx = rcp_dynamic_cast<Xpetra::EpetraCrsMatrix >(epbA->getMatrix(0,0));
    if (tmp_ECrsMtx == Teuchos::null)
      throw Exceptions::BadCast("Cast from Xpetra::CrsMatrix to Xpetra::EpetraCrsMatrix failed");

    RCP<Epetra_CrsMatrix> epA = tmp_ECrsMtx->getEpetra_CrsMatrixNonConst();
    return epA->Comm();
  }

  RCP<Epetra_CrsMatrix> epA = Utils::Op2NonConstEpetraCrs(A);
  return epA->Comm();
}

const Epetra_Map& EpetraOperator::OperatorDomainMap() const {
  RCP<Matrix> A = Hierarchy_->GetLevel(0)->Get<RCP<Matrix> >("A");

  RCP<Xpetra::BlockedCrsMatrix<double, int, int> > epbA = Teuchos::rcp_dynamic_cast<Xpetra::BlockedCrsMatrix<double, int, int> >(A);
  if (epbA != Teuchos::null)
    return Xpetra::toEpetra(epbA->getDomainMap());

  RCP<Epetra_CrsMatrix> epA = Utils::Op2NonConstEpetraCrs(A);
  return epA->DomainMap();
}

const Epetra_Map & EpetraOperator::OperatorRangeMap() const {
  RCP<Matrix> A = Hierarchy_->GetLevel(0)->Get<RCP<Matrix> >("A");

  RCP<Xpetra::BlockedCrsMatrix<double, int, int> > epbA = Teuchos::rcp_dynamic_cast<Xpetra::BlockedCrsMatrix<double, int, int> >(A);
  if (epbA != Teuchos::null)
    return Xpetra::toEpetra(epbA->getRangeMap());

  RCP<Epetra_CrsMatrix> epA = Utils::Op2NonConstEpetraCrs(A);
  return epA->RangeMap();
}

} // namespace
