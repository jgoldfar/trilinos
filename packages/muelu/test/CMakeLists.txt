ASSERT_DEFINED(
  ${PACKAGE_NAME}_ENABLE_Tpetra
  )

TRIBITS_ADD_TEST_DIRECTORIES(blockedtransfer)
TRIBITS_ADD_TEST_DIRECTORIES(longlong)
TRIBITS_ADD_TEST_DIRECTORIES(navierstokes)
if (NOT MUELU_VALGRIND_TESTING)
  # JJH 4Feb2014:  This is failing the nightly MPI valgrind test.  It runs fine
  # interactively.  If/when I figure out the cause, I'll reenable this during
  # memory testing.
  TRIBITS_ADD_TEST_DIRECTORIES(paramlist)
ENDIF()
TRIBITS_ADD_TEST_DIRECTORIES(permutation)
TRIBITS_ADD_TEST_DIRECTORIES(profiling)
TRIBITS_ADD_TEST_DIRECTORIES(scaling)
TRIBITS_ADD_TEST_DIRECTORIES(simple1D)
TRIBITS_ADD_TEST_DIRECTORIES(structure2D)
TRIBITS_ADD_TEST_DIRECTORIES(unit_tests)

if (NOT MUELU_VALGRIND_TESTING)
  TRIBITS_ADD_TEST_DIRECTORIES(interface)
  IF (${PACKAGE_NAME}_ENABLE_Tpetra)
    TRIBITS_ADD_TEST_DIRECTORIES(helmholtz)
  ENDIF()
ELSE()
  MESSAGE("Skipping EasyParameterListInterpreter and Helmholtz due to valgrind testing.")
ENDIF()

#TRIBITS_ADD_TEST_DIRECTORIES(mhdvanka)

INCLUDE(TribitsAddExecutableAndTest)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)

SET(SOURCES "")
