
INCLUDE(TribitsAddExecutableAndTest)


ASSERT_DEFINED(${PACKAGE_NAME}_ENABLE_SuperLU)
IF (${PACKAGE_NAME}_ENABLE_SuperLU)

  TRIBITS_ADD_EXECUTABLE_AND_TEST(
    Test_SuperLU
    SOURCES cxx_main.cpp
    NUM_MPI_PROCS 4
    COMM serial mpi
    )

ENDIF()
