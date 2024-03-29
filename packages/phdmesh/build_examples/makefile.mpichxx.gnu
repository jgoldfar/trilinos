#-----------------------------------------------------------------------
#      phdMesh : Parallel Heterogneous Dynamic unstructured Mesh
#                Copyright (2004) Sandia Corporation
#  
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
# 
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#  
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#  
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 
# USA
#-----------------------------------------------------------------------
# Paths for source code and compile locations:

PHDMESH_PATH = ${HOME}/Trilinos/packages/phdmesh
INCLUDE_PATH = -I. -I$(PHDMESH_PATH)/include -I$(PHDMESH_PATH)/src

#-----------------------------------------------------------------------
# Compilers and linkers:

CC      = mpicc
CXX     = mpicxx
LDCXX   = $(CXX)
AR      = ar
ARFLAGS = rc
RANLIB  = ranlib

#-----------------------------------------------------------------------
# Configuration options:

HAVE_MPI = 1
MPICH_SKIP_MPICXX = 1
# NO_PTHREADS = 1
# TPI_NO_SCHED = 1
# HAVE_SNL_EXODUSII = 1

# Libraries for configuration options:

C_LIB_EXT   = -lpthread
CXX_LIB_EXT = -lpthread

#-----------------------------------------------------------------------
# Compiler options:

# OPTIMIZED:            -O3
# DEBUG:                -g
# STATIC COMPILE/LINK:  -static
# 32BIT EXECUTABLE:     -Wa,--32 -m32

COMPILE_OPTIONS = -O3 -static

# Diagnostics:

CFLAGS_DIAG   = -Wall -Wextra -Werror
CXXFLAGS_DIAG = -Wall -Wextra -Werror -Woverloaded-virtual

#-----------------------------------------------------------------------

CFLAGS   = -std=c99   $(CFLAGS_DIAG)   $(COMPILE_OPTIONS) $(INCLUDE_PATH)
CXXFLAGS = -std=c++98 $(CXXFLAGS_DIAG) $(COMPILE_OPTIONS) $(INCLUDE_PATH)
   
#-----------------------------------------------------------------------

include ${PHDMESH_PATH}/makefile_targets

