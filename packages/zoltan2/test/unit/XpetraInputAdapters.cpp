// @HEADER
// ***********************************************************************
//
//         Zoltan2: Sandia Partitioning Ordering & Coloring Library
//
//                Copyright message goes here.   TODO
//
// ***********************************************************************
//
// Basic testing of xpetra and tpetra matrix and graph input adapters
//
// TODO test other input adapters as well.

#include <TestAdapters.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_Comm.hpp>
#include <Teuchos_OrdinalTraits.hpp>
#include <string>

using namespace std;
using Teuchos::RCP;
using Teuchos::Comm;
using Teuchos::DefaultComm;
using Teuchos::OrdinalTraits;
using Teuchos::ScalarTraits;

template <typename GraphAdapter, typename Graph>
  void testGraphAdapter(RCP<GraphAdapter> input, RCP<const Graph> g, 
    RCP<const Comm<int> > comm)
{
  typedef typename GraphAdapter::lid_t LID;
  typedef typename GraphAdapter::gid_t GID;
  typedef typename GraphAdapter::lno_t LNO;
  typedef typename GraphAdapter::gno_t GNO;
  typedef typename GraphAdapter::node_t Node;

  typedef Tpetra::CrsGraph<LNO, GNO, Node> crsGraph_t;

  typedef Zoltan2::GraphInput<crsGraph_t> graphInput_t; 

  // TODO do we need to cast?

  const crsGraph_t *tpetraGraph = static_cast<const crsGraph_t *>(g.get());

  size_t nRows = tpetraGraph->getNodeNumRows();
  size_t ngRows = tpetraGraph->getGlobalNumRows();
  size_t nEntries = tpetraGraph->getNodeNumEntries();
  size_t ngEntries = tpetraGraph->getGlobalNumEntries();

  size_t num = input->getLocalNumVertices();
  TEST_FAIL_AND_EXIT(*comm, num==nRows, "input->getLocalNumVertices", 1);
  num = input->getGlobalNumVertices();
  TEST_FAIL_AND_EXIT(*comm, num==ngRows, "input->getGlobalNumVertices", 1);
  num = input->getLocalNumEdges();
  TEST_FAIL_AND_EXIT(*comm, num==nEntries, "input->getLocalNumEdges", 1);
  num = input->getGlobalNumEdges();
  TEST_FAIL_AND_EXIT(*comm, num==ngEntries, "input->getGlobalNumEdges", 1);
}

template <typename MatrixAdapter, typename Matrix>
  void testMatrixAdapter(RCP<MatrixAdapter> input, RCP<Matrix> m, RCP<const Comm<int> >&comm)
{
  typedef typename MatrixAdapter::scalar_t Scalar;
  typedef typename MatrixAdapter::lid_t LID;
  typedef typename MatrixAdapter::gid_t GID;
  typedef typename MatrixAdapter::lno_t LNO;
  typedef typename MatrixAdapter::gno_t GNO;
  typedef typename MatrixAdapter::node_t Node;

  typedef Tpetra::CrsMatrix<Scalar, LNO, GNO, Node> crsMatrix_t;

  typedef Zoltan2::MatrixInput<crsMatrix_t> matrixInput_t; 

  // TODO do we need to cast?

  matrixInput_t *m_in = static_cast<matrixInput_t *>(input.get());

  crsMatrix_t *tpetraMatrix = static_cast<crsMatrix_t *>(m.get());

  size_t nRows = tpetraMatrix->getNodeNumRows();
  size_t ngRows = tpetraMatrix->getGlobalNumRows();
  size_t nColumns = tpetraMatrix->getNodeNumCols();
  size_t ngColumns = tpetraMatrix->getGlobalNumCols();

  size_t num = m_in->getLocalNumRows();
  TEST_FAIL_AND_EXIT(*comm, num==nRows, "m_in.getLocalNumRows", 1);
  num = m_in->getGlobalNumRows();
  TEST_FAIL_AND_EXIT(*comm, num==ngRows, "m_in.getGlobalNumRows", 1);
  num = m_in->getLocalNumColumns();
  TEST_FAIL_AND_EXIT(*comm, num==nColumns, "m_in.getLocalNumColumns", 1);
  num = m_in->getGlobalNumColumns();
  TEST_FAIL_AND_EXIT(*comm, num==ngColumns, "m_in.getGlobalNumColumns", 1);
}



template <typename Scalar, typename LNO, typename GNO>
  void testInputAdapters(GNO xdim, GNO ydim, GNO zdim,
    std::string &fname, RCP<const Comm<int> > &comm, int rank)
{
  bool include64BitIds = false;

  std::string ScalarName(ScalarTraits<Scalar>::name());
  std::string LNOName(OrdinalTraits<LNO>::name());
  std::string GNOName(OrdinalTraits<GNO>::name());

//
//  64-bit global IDs need to wait for Tpetra problem to be resolved.
//  if (sizeof(GNO) >= 8)
//    include64BitIds = true;
//

  typedef Zoltan2::default_node_t Node;

  typedef Tpetra::CrsMatrix<Scalar, LNO, GNO, Node> crsM_t;
  typedef Tpetra::CrsGraph<LNO, GNO, Node> crsG_t;
  typedef Zoltan2::TpetraCrsMatrixInput<crsM_t> tpetraM_t;
  typedef Zoltan2::TpetraCrsGraphInput<crsG_t>  tpetraG_t;
  typedef Zoltan2::XpetraCrsMatrixInput<crsM_t> xpetraM_t;
  typedef Zoltan2::XpetraCrsGraphInput<crsG_t>  xpetraG_t;

  // Create some input adapters for xpetra objects

  TestAdapters<Scalar,LNO,GNO> *input = NULL;

  if (fname.size() > 0)
    input = new TestAdapters<Scalar,LNO,GNO>(fname);
  else
    input = new TestAdapters<Scalar,LNO,GNO>(xdim, ydim, zdim);

#ifdef HAVE_MPI
  input->setMpiCommunicator(MPI_COMM_WORLD);
#endif

  // Get the original matrix in order to compare answers to it.
  Teuchos::RCP<crsM_t > M = input->getMatrix();
  Teuchos::RCP<const crsG_t> G = M->getCrsGraph();

  RCP<tpetraM_t> tmi = input->getTpetraCrsMatrixInputAdapter();
  testMatrixAdapter<tpetraM_t, crsM_t>(tmi, M, comm);

  RCP<tpetraG_t> tgi = input->getTpetraCrsGraphInputAdapter();
  testGraphAdapter<tpetraG_t, crsG_t>(tgi, G, comm);

  RCP<xpetraM_t> xmi = input->getXpetraCrsMatrixInputAdapter();
  testMatrixAdapter<xpetraM_t, crsM_t>(xmi, M, comm);

  RCP<xpetraG_t> xgi = input->getXpetraCrsGraphInputAdapter();
  testGraphAdapter<xpetraG_t, crsG_t>(xgi, G, comm);

  if (include64BitIds){
    // This matrix has ids that use the high order 4 bytes of
    // of the 8 byte ID.  true: delete the original M after making a new one.
    //RCP<tpetraM_t> tmi64 = input->getTpetraCrsMatrixInputAdapter64(true);
    //testMatrixAdapter<tpetraM_t, crsM_t>(tmi64, M, comm);
  }

  delete input;

  if (!rank){
    if (fname.size())
      std::cout << "Processed " << fname << ": ";
    else{
      std::cout << "Created input adapter for matrix/graph of size ";
      std::cout << xdim << " x " << ydim << " x " << zdim << std::endl;
    }
    std::cout << "Scalar = " << ScalarName;
    std::cout << ", LNO = " << LNOName;
    std::cout << ", GNO = " << GNOName << std::endl;
    if (include64BitIds)
      std::cout << "Including a Tpetra::CrsMatrix with 64-bit Ids." << std::endl;
    std::cout << std::endl;
  }
}

int main(int argc, char *argv[])
{
  Teuchos::GlobalMPISession session(&argc, &argv);
  RCP<const Comm<int> > comm = DefaultComm<int>::getComm();
  int rank = session.getRank();

  std::vector<std::string> mtxFiles;
  
  mtxFiles.push_back("../data/simple.mtx");
  mtxFiles.push_back("../data/cage10.mtx");

  // To use this matrix we would need to pass a domain map
  // to FillComplete.  So we skip it for now.  TODO
  // mtxFiles.push_back("../data/diag500_4.mtx");

  for (unsigned int fileNum=0; fileNum < mtxFiles.size(); fileNum++){
    testInputAdapters<double, int, int>(0,0,0,mtxFiles[fileNum], comm, rank);
    testInputAdapters<double, int, long>(0,0,0,mtxFiles[fileNum],comm, rank);
  }

  std::string nullString;

  testInputAdapters<double, int, long>(10, 20, 10, nullString, comm, rank);

  if (rank == 0)
    std::cout << "PASS" << std::endl;

  return 0;
}
