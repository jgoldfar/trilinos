
INCLUDE(TribitsAddExecutableAndTest)

ASSERT_DEFINED(${PACKAGE_NAME}_ENABLE_Galeri)

IF (${PACKAGE_NAME}_ENABLE_Galeri)

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  OverlappingRowMatrix_LL
  SOURCES cxx_main.cpp
  ARGS "-v"
  NUM_MPI_PROCS 1-4
  COMM serial mpi
  )

ENDIF (${PACKAGE_NAME}_ENABLE_Galeri)
