/*
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
*/

/** \file CrsMatrix_DefaultMultiplyTests.hpp
    \brief A file unit-testing and demonstrating the Kokkos default sparse kernel provider.
 */

  TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL( DefaultSparseOps, NodeTest, Ordinal, Scalar, Node )
  {
    RCP<Node> node = getNode<Node>();
    typedef typename DefaultKernels<Scalar,Ordinal,Node>::SparseOps DSM;
    typedef CrsGraph<Ordinal,Node,DSM>                             GRPH;
    typedef CrsMatrix<Scalar,Ordinal,Node,DSM>                      MAT;
    typedef MultiVector<Scalar,Node>                                 MV;
    typedef Teuchos::ScalarTraits<Scalar>                            ST;
    typename DSM::template rebind<Scalar>::other dsm(node);
    TEST_EQUALITY(dsm.getNode(), node);
  }

  TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL( DefaultSparseOps, ResubmitMatrix, Ordinal, Scalar, Node )
  {
    RCP<Node> node = getNode<Node>();
    typedef typename DefaultKernels<Scalar,Ordinal,Node>::SparseOps DSM;
    typedef CrsGraph<Ordinal,Node,DSM>                             GRPH;
    typedef CrsMatrix<Scalar,Ordinal,Node,DSM>                      MAT;
    typedef MultiVector<Scalar,Node>                                 MV;
    typedef Teuchos::ScalarTraits<Scalar>                            ST;
    const size_t numRows = 5;
    GRPH G(numRows,node);
    MAT  A(G);
    A.finalize(true);
    out << "\n**\n** Can't submit the values before the structure\n**\n";
    {
      typename DSM::template rebind<Scalar>::other dsm(node);
      TEST_THROW( dsm.initializeValues(A), std::runtime_error );
    }
    out << "\n**\n** Can't submit the graph twice\n**\n";
    {
      typename DSM::template rebind<Scalar>::other dsm(node);
      dsm.initializeStructure(G);
      TEST_THROW( dsm.initializeStructure(G), std::runtime_error );
    }
    out << "\n**\n** Can submit the graph again after calling clear\n**\n";
    {
      typename DSM::template rebind<Scalar>::other dsm(node);
      dsm.initializeStructure(G);
      dsm.clear();
      TEST_NOTHROW( dsm.initializeStructure(G) );
    }
    out << "\n**\n** Can submit the values twice\n**\n";
    {
      typename DSM::template rebind<Scalar>::other dsm(node);
      dsm.initializeStructure(G);
      dsm.initializeValues(A);
      TEST_NOTHROW( dsm.initializeValues(A) );
    }
  }
