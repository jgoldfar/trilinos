// Epetra_BlockMap Test routine

#ifdef EPETRA_MPI
#include "Epetra_MpiComm.h"
#include <mpi.h>
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Time.h"
#include "Epetra_BlockMap.h"
#include "Epetra_MultiVector.h"
#include "BuildTestProblems.h"
#include "ExecuteTestProblems.h"

int main(int argc, char *argv[]) {

  int ierr = 0, i, j;

#ifdef EPETRA_MPI

  // Initialize MPI

  MPI_Init(&argc,&argv);
  int size, rank; // Number of MPI processes, My process ID

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);

#else

  int size = 1; // Serial case (not using MPI)
  int rank = 0;
  Epetra_SerialComm Comm;

#endif

  bool verbose = false;

  // Check if we should print results to standard out
  if (argc>1) if (argv[1][0]=='-' && argv[1][1]=='v') verbose = true;

  //  char tmp;
  //  if (rank==0) cout << "Press any key to continue..."<< endl;
  //  if (rank==0) cin >> tmp;
  //  Comm.Barrier();

  int MyPID = Comm.MyPID();
  int NumProc = Comm.NumProc();
  if (verbose) cout << Comm <<endl;

  bool verbose1 = verbose;

  // Redefine verbose to only print on PE 0
  if (verbose && rank!=0) verbose = false;

  int NumMyElements = 10000;
  int NumMyElements1 = NumMyElements; // Needed for localmap
  int NumGlobalElements = NumMyElements*NumProc+EPETRA_MIN(NumProc,3);
  if (MyPID < 3) NumMyElements++;
  int IndexBase = 0;
  int ElementSize = 7;
  bool DistributedGlobal = (NumGlobalElements>NumMyElements);
  int NumVectors = 4;
  
  // Test LocalMap constructor

  if (verbose) cout << "Checking Epetra_LocalMap(NumMyElements1, IndexBase, Comm)" << endl;

  Epetra_LocalMap *LocalMap = new Epetra_LocalMap(NumMyElements1, IndexBase,
                              Comm);
  // Test Petra-defined uniform linear distribution constructor
  if (verbose) cout << "Checking Epetra_BlockMap(NumGlobalElements, ElementSize, IndexBase, Comm)" << endl;

  Epetra_BlockMap * BlockMap = new Epetra_BlockMap(NumGlobalElements, ElementSize, IndexBase, Comm);
  ierr = MultiVectorTests(*BlockMap, NumVectors, verbose);
  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete BlockMap;

  // Test User-defined linear distribution constructor

  if (verbose) cout << "Checking Epetra_BlockMap(NumGlobalElements, NumMyElements, ElementSize, IndexBase, Comm)" << endl;

  BlockMap = new Epetra_BlockMap(NumGlobalElements, NumMyElements, ElementSize, IndexBase, Comm);

  ierr = MultiVectorTests(*BlockMap, NumVectors, verbose);
  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete BlockMap;

  // Test User-defined arbitrary distribution constructor
  // Generate Global Element List.  Do in reverse for fun!

  int * MyGlobalElements = new int[NumMyElements];
  int MaxMyGID = (Comm.MyPID()+1)*NumMyElements-1+IndexBase;
  if (Comm.MyPID()>2) MaxMyGID+=3;
  for (i = 0; i<NumMyElements; i++) MyGlobalElements[i] = MaxMyGID-i;

  if (verbose) cout << "Checking Epetra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements,  ElementSize, IndexBase, Comm)" << endl;

  BlockMap = new Epetra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements, ElementSize,
		      IndexBase, Comm);
  ierr = MultiVectorTests(*BlockMap, NumVectors, verbose);
  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete BlockMap;

  int * ElementSizeList = new int[NumMyElements];
  int NumMyEquations = 0;
  int NumGlobalEquations = 0;
  for (i = 0; i<NumMyElements; i++) 
    {
      ElementSizeList[i] = i%6+2; // blocksizes go from 2 to 7
      NumMyEquations += ElementSizeList[i];
    }
  ElementSize = 7; // Set to maximum for use in checkmap
  NumGlobalEquations = Comm.NumProc()*NumMyEquations;

  // Adjust NumGlobalEquations based on processor ID
  if (Comm.NumProc() > 3)
    {
      if (Comm.MyPID()>2)
	NumGlobalEquations += 3*((NumMyElements)%6+2);
      else 
	NumGlobalEquations -= (Comm.NumProc()-3)*((NumMyElements-1)%6+2);
    }

  if (verbose) cout << "Checking Epetra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements,  ElementSizeList, IndexBase, Comm)" << endl;

  BlockMap = new Epetra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements, ElementSizeList,
		      IndexBase, Comm);
  ierr = MultiVectorTests(*BlockMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  // Test Copy constructor

  if (verbose) cout << "Checking Epetra_BlockMap(*BlockMap)" << endl;

  Epetra_BlockMap * BlockMap1 = new Epetra_BlockMap(*BlockMap);

  ierr = MultiVectorTests(*BlockMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete [] ElementSizeList;
  delete [] MyGlobalElements;
  delete BlockMap;
  delete BlockMap1;


  // Test Petra-defined uniform linear distribution constructor

  if (verbose) cout << "Checking Epetra_Map(NumGlobalElements, IndexBase, Comm)" << endl;

  Epetra_Map * Map = new Epetra_Map(NumGlobalElements, IndexBase, Comm);
  ierr = MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete Map;

  // Test User-defined linear distribution constructor

  if (verbose) cout << "Checking Epetra_Map(NumGlobalElements, NumMyElements, IndexBase, Comm)" << endl;

  Map = new Epetra_Map(NumGlobalElements, NumMyElements, IndexBase, Comm);

  ierr = MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete Map;

  // Test User-defined arbitrary distribution constructor
  // Generate Global Element List.  Do in reverse for fun!

  MyGlobalElements = new int[NumMyElements];
  MaxMyGID = (Comm.MyPID()+1)*NumMyElements-1+IndexBase;
  if (Comm.MyPID()>2) MaxMyGID+=3;
  for (i = 0; i<NumMyElements; i++) MyGlobalElements[i] = MaxMyGID-i;

  if (verbose) cout << "Checking Epetra_Map(NumGlobalElements, NumMyElements, MyGlobalElements,  IndexBase, Comm)" << endl;

  Map = new Epetra_Map(NumGlobalElements, NumMyElements, MyGlobalElements, 
		      IndexBase, Comm);
  ierr = MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  // Test Copy constructor

  if (verbose) cout << "Checking Epetra_Map(*Map)" << endl;
 
  Epetra_Map * Map1 = new Epetra_Map(*Map);

  ierr = MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete [] MyGlobalElements;
  delete Map;
  delete Map1;
  delete LocalMap;

  if (verbose1)
    {
      // Test MultiVector MFLOPS for 2D Dot Product
      int M = 27;
      int N = 27;
      int K = 10000;
      Epetra_Map * Map2 = new Epetra_Map(-1, K, IndexBase, Comm);
      Epetra_LocalMap * Map3 = new Epetra_LocalMap(M, IndexBase, Comm);
      
      Epetra_MultiVector & A = *new Epetra_MultiVector(*Map2,N);A.Random();
      Epetra_MultiVector & B = *new Epetra_MultiVector(*Map2,N);B.Random();
      Epetra_MultiVector & C = *new Epetra_MultiVector(*Map3,N);C.Random();

      if (verbose) cout << "Testing Assignment operator" << endl;

      double tmp1 = 1.00001* (double) (MyPID+1);
      double tmp2 = tmp1;
      A[1][1] = tmp1;
      tmp2 = A[1][1];
      cout << "On PE "<< MyPID << "  A[1][1] should equal = " << tmp1;
      if (tmp1==tmp2) cout << " and it does!" << endl;
      else cout << " but it equals " << tmp2;
 
      Comm.Barrier();
	  
      if (verbose) cout << "Testing MFLOPs" << endl;
      Epetra_Time & mytimer = *new Epetra_Time(Comm);
      C.Multiply('T', 'N', 0.5, A, B, 0.0);
      double Multiply_time = mytimer.ElapsedTime();
      double Multiply_flops = C.Flops();
      if (verbose) cout << "\n\nTotal FLOPs = " << Multiply_flops << endl;
      if (verbose) cout << "Total Time  = " << Multiply_time << endl;
      if (verbose) cout << "MFLOPs      = " << Multiply_flops/Multiply_time/1000000.0 << endl;

      delete &mytimer;
      delete &A;
      delete &B;
      delete &C;
      delete Map2;
      delete Map3;

      Comm.Barrier();
	  
      // Test MultiVector ostream operator with Petra-defined uniform linear distribution constructor
      // and a small vector
      
      Epetra_Map * Map4 = new Epetra_Map(100, IndexBase, Comm);
      double * Dp = new double[200];
      for (j=0; j<2; j++)
	for (i=0; i<100; i++)
	  Dp[i+j*100] = i+j*100;
      Epetra_MultiVector & D = *new Epetra_MultiVector(View, *Map4,Dp, 100, 2);
	  
      if (verbose) cout << "\n\nTesting ostream operator:  Multivector  should be 100-by-2 and print i,j indices" 
	   << endl << endl;
      cout << D << endl;
      delete &D;
      delete [] Dp;
      delete Map4;
    }

#ifdef EPETRA_MPI
  MPI_Finalize();
#endif

  return 0;
}

