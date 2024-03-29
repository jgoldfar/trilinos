//@HEADER
// ************************************************************************
//
//          Kokkos: Node API and Parallel Node Kernels
//              Copyright (2008) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
//@HEADER

#ifndef __Kokkos_Raw_SparseTriangularSolve_def_hpp
#define __Kokkos_Raw_SparseTriangularSolve_def_hpp

/// \file Kokkos_Raw_SparseTriangularSolve_def.hpp
/// \brief Definitions of "raw" sequential sparse triangular solve routines.
/// \warning (mfh 12 Feb 2013) Normally, this code would be generated
///   by the SparseTriSolve.py script.  As a result, this header
///   should not normally be edited by hand, since those edits would
///   disappear each time the script were run.  However, there is a
///   bug in the script.  Please use the hand-edited version of this
///   file for now until I have a chance to fix the script.

namespace KokkosClassic {
namespace Raw {

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrColMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    // This assumes that the diagonal entry is last in the row.
    // This will break if the row is empty, but that's bad for
    // non-unit-diagonal triangular solve anyway.
    const MatrixScalar A_rr = val[ptr[r+1]-1];
    for (size_t k = ptr[r]; k < ptr[r+1] - 1; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] /= A_rr;
    }
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrRowMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = val[ptr[r]];
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] /= A_rr;
    }
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrColMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrRowMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscColMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrColMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = val[ptr[r]];
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] /= A_rr;
    }
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrRowMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = val[ptr[r]];
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] /= A_rr;
    }
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrColMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrRowMajorUnitDiag (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = val[k];
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorUnitDiagInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorUnitDiagInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorUnitDiagInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorUnitDiagInPlace (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = val[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrColMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = STS::conjugate (val[ptr[r]]);
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] /= A_rr;
    }
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrRowMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = STS::conjugate (val[ptr[r]]);
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] /= A_rr;
    }
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrColMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
lowerTriSolveCsrRowMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = 0; r < numRows; ++r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrColMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = STS::conjugate (val[ptr[r]]);
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] /= A_rr;
    }
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc of the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrRowMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    // We assume the diagonal entry is first in the row.
    const MatrixScalar A_rr = STS::conjugate (val[ptr[r]]);
    for (size_t k = ptr[r]+1; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] /= A_rr;
    }
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal j = 0; j < numVecs; ++j) {
    for (Ordinal i = 0; i < numRows; ++i) {
      X[i + j*colStrideX] = Y[i + j*colStrideY];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrColMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal colStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r + j*colStrideX] = Y[r + j*colStrideY];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal i = 0; i < numRows; ++i) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[i*rowStrideX + j] = Y[i*rowStrideY + j];
    }
  }

  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
upperTriSolveCsrRowMajorUnitDiagConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val,
  const DomainScalar* const Y,
  const Ordinal rowStrideY)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal r = numRows-1; r >= 0; --r) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[r*rowStrideX + j] = Y[r*rowStrideY + j];
    }
    for (size_t k = ptr[r]; k < ptr[r+1]; ++k) {
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      const Ordinal c = ind[k];
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current row r
  } // for each row r
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = 0; c < numCols; ++c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscColMajorUnitDiagInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
lowerTriSolveCscRowMajorUnitDiagInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = 0; c < numCols; ++c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c + j*colStrideX] /= A_cc;
    }
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = numCols-1; c >= 0; --c) {
    MatrixScalar A_cc = STS::zero ();
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      if (r == c) {
        A_cc += A_rc;
      } else {
        for (Ordinal j = 0; j < numVecs; ++j) {
          X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
        }
      }
    } // for each entry A_rc in the current column c
    for (Ordinal j = 0; j < numVecs; ++j) {
      X[c*rowStrideX + j] /= A_cc;
    }
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscColMajorUnitDiagInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal colStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r + j*colStrideX] -= A_rc * X[c + j*colStrideX];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#ifndef KOKKOSCLASSIC_HAVE_FAST_COMPILE

template<class Ordinal,
         class MatrixScalar,
         class RangeScalar>
void
upperTriSolveCscRowMajorUnitDiagInPlaceConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  RangeScalar* const X,
  const Ordinal rowStrideX,
  const  size_t* const ptr,
  const Ordinal* const ind,
  const MatrixScalar* const val)
{
  typedef Teuchos::ScalarTraits<MatrixScalar> STS;

  for (Ordinal c = numCols-1; c >= 0; --c) {
    for (size_t k = ptr[c]; k < ptr[c+1]; ++k) {
      const Ordinal r = ind[k];
      const MatrixScalar A_rc = STS::conjugate (val[k]);
      for (Ordinal j = 0; j < numVecs; ++j) {
        X[r*rowStrideX + j] -= A_rc * X[c*rowStrideX + j];
      }
    } // for each entry A_rc in the current column c
  } // for each column c
}

#endif // ! KOKKOSCLASSIC_HAVE_FAST_COMPILE

} // namespace Raw
} // namespace KokkosClassic

#endif // #ifndef __Kokkos_Raw_SparseTriangularSolve_def_hpp
