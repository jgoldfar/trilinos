#! /usr/bin/env python

# @HEADER
# ************************************************************************
# 
#                PyTrilinos: Python Interface to Trilinos
#                   Copyright (2005) Sandia Corporation
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? Contact Bill Spotz (wfspotz@sandia.gov) 
# 
# ************************************************************************
# @HEADER

######################################################################
#
# Example of how to define an Epetra.BasicRowMatrix derived class in python
# 
# This example shows how to derive from the class Epetra.BasicRowMatrix in python.
# The main procedure is as follows: - create a python class derived from
# Epetra.BasicRowMatrix -- in this case a 1D Laplacian matrix.  Define all the
# methods as done in this file.  The most important methods are probably
# Apply(), Multiply(), and ExtractMyRowCopy() (note that the return signature
# for NumMyRowEntries() and ExtractMyRowCopy() are different for python than for
# C++).  Some methods do not need to be implemented; in this case they simply
# return -1.  You may now create an instance of your derived class and supply it
# to any Trilinos solver that can use it (in this case AztecOO).
#
# Based on a script originally written by Marzio Sala.  Updated by Bill Spotz.
#
######################################################################

try:
    import setpath
    import Epetra
    import AztecOO
except:
    from PyTrilinos import Epetra, AztecOO
    print "Using installed version of Epetra, AztecOO"

import numpy

################################################################################

class Laplace1D_Matrix(Epetra.BasicRowMatrix):

    def __init__(self, n, comm=None):
        """
        __init__(self, n) -> Laplace1D_Matrix (with and Epetra.PyComm() communicator)
        __init__(self, n, comm) -> Laplace1D_Matrix (with given communicator)
        """
        # Determine the communicator
        if comm is None:
            comm = Epetra.PyComm()
        # Initialize the base class.  This is REQUIRED
        Epetra.BasicRowMatrix.__init__(self, comm)
        # Store the problem size
        self.__size = n
        # Default indexes
        self.__y0 =  1
        self.__y1 = -1
        # Create and store the row and column maps
        rowMap = Epetra.Map(n, 0, self.Comm())
        myIndexes = list(rowMap.MyGlobalElements())
        if self.Comm().MyPID() > 0:
            self.__y0 = None    # Equivalent to index 0
            myIndexes.insert(0,myIndexes[0]-1)
        if self.Comm().MyPID() < self.Comm().NumProc()-1:
            self.__y1 = None    # Equivalent to last index
            myIndexes.append(myIndexes[-1]+1)
        colMap = Epetra.Map(-1, myIndexes, 0, self.Comm())
        self.SetMaps(rowMap, colMap)
        # Store a label for the row matrix
        self.__label = "1D Laplace Basic Row Matrix"

    def __str__(self):
        "Return the row matrix label"
        return self.__label

    def Label(self):
        "Optional implementation of Epetra.Operator class"
        return self.__label

    def NumMyRowEntries(self, MyRow, NumEntries):
        """
        Required implementation of Epetra.BasicRowMatrix class.  In C++,
        NumEntries is an int& argument intended as output.  When called via
        callbacks from C++, this int& is converted to a numpy array of length 1
        so that it can be altered in-place with 'NumEntries[0] = ...'.
        """
        try:
            globalRow = self.RowMatrixRowMap().GID(MyRow)
            if globalRow == 0 or globalRow == self.__size-1:
                NumEntries[0] = 1
            else:
                NumEntries[0] = 3
            return 0
        except Exception, e:
            print e
            return -1

    def ExtractMyEntryView(self, CurIndex, Value, RowIndex, ColIndex):
        """
        Required implementation of Epetra.BasicRowMatrix class.  In C++, Value,
        RowIndex and ColIndex are all scalar output arguments.  When called via
        callbacks from C++, these arguments are converted to numpy arrays of
        length 1 so that we can manipulate the data in-place.  For example, use
        'Value[0] = ...', etc.
        """
        try:
            if CurIndex < 0 or CurIndex >= self.NumMyNonzeros():
                return -1
            comm = self.Comm()
            if comm.MyPID() == 0:
                if CurIndex == 0:
                    Value[0]    = 1.0
                    RowIndex[0] = 0
                    ColIndex[0] = 0
                    return 0
                CurIndex += 2
            if comm.MyPID() == comm.NumProc()-1:
                if CurIndex == self.NumMyNonzeros()-1:
                    Value[0]    = 1.0
                    RowIndex[0] = self.RowMatrixRowMap().NumMyElements()-1
                    ColIndex[0] = RowIndex[0]
                    return 0
            row, col = divmod(CurIndex,3)
            if col == 1: Values[0] =  2.0
            else:        Values[0] = -1.0
            RowIndex[0] = row
            ColIndex[0] = col + row - 1
            return 0
        except Exception, e:
            print e
            return -1

    def ExtractMyRowCopy(self, MyRow, Length, NumEntries, Values, Indices):
        """
        Required implementation of Epetra.BasicRowMatrix class.  In C++,
        NumEntries, Values, and Indices are all output arguments.  When called
        via callbacks from C++, these arguments are converted to numpy arrays so
        that we can manipulate the data in-place.  NumEntries is a scalar in
        C++, but must be accessed as NumEntries[0] in python.
        """
        try:
            globalRow = self.RowMatrixRowMap().GID(MyRow)
            if globalRow == 0 or globalRow == self.__size-1:
                if (Length < 1):
                    return -1
                NumEntries[0] = 1
                Values[0]     = 1.0
                Indices[0]    = MyRow
            else:
                if (Length < 3):
                    return -1
                NumEntries[0] = 3
                Values[:3]    = [   -1.0,   2.0,    -1.0]
                Indices[:3]   = [MyRow-1, MyRow, MyRow+1]
            return 0
        except Exception, e:
            print e
            return -1

################################################################################

def main():

    # Problem initialization
    n     = 100
    bc0   = 0.0
    bc1   = 1.0
    tol   = 1.0e-5
    comm  = Epetra.PyComm()
    lap1d = Laplace1D_Matrix(n, comm)

    # Create solution and RHS vectors
    x = Epetra.Vector(lap1d.RowMatrixColMap())
    b = Epetra.Vector(lap1d.RowMatrixRowMap())

    # Initialize vectors: x will be a straight line between its boundary values,
    # and b=1, with its boundary values equal to x on the boundaries
    x[:] = bc0 + (bc1-bc0) * (x.Map().MyGlobalElements() / (n-1.0))
    b.PutScalar(1.0)
    if comm.MyPID() == 0:
        b[0] = bc0
    if comm.MyPID() == comm.NumProc()-1:
        b[-1] = bc1

    # Build the linear system solver
    problem = Epetra.LinearProblem(lap1d, x, b)
    solver  = AztecOO.AztecOO(problem)
    solver.SetParameters({"Solver"  : "GMRES",
                          "Precond" : "Neumann",
                          "Output"  : 16      })

    # Solve the problem
    solver.Iterate(n, tol)
    if comm.MyPID() == 0:
        if solver.ScaledResidual() < tol: print "End Result: TEST PASSED"
        else:                             print "End Result: TEST FAILED"

################################################################################

if __name__ == "__main__":
    main()
