#! /usr/bin/env python

# @HEADER
# ************************************************************************
#
#              PyTrilinos.Epetra: Python Interface to Epetra
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
# Questions? Contact Michael A. Heroux (maherou@sandia.gov)
#
# ************************************************************************
# @HEADER

# Imports.  Users importing an installed version of PyTrilinos should use the
# "from PyTrilinos import ..." syntax.  Here, the setpath module adds the build
# directory, including "PyTrilinos", to the front of the search path.  We thus
# use "import ..." for Trilinos modules.  This prevents us from accidentally
# picking up a system-installed version and ensures that we are testing the
# build module.
import sys

try:
    import setpath
    import Epetra
except ImportError:
    from PyTrilinos import Epetra
    print >>sys.stderr, "Using system-installed Epetra"

import unittest
from   numpy    import *

##########################################################################

class EpetraMultiVectorTestCase(unittest.TestCase):
    "TestCase class for MultiVector objects"

    def setUp(self):
        self.length      = 9
        self.scale       = 1.0 / (self.length-1)
        self.comm        = Epetra.PyComm()
        self.map         = Epetra.Map(self.length*self.comm.NumProc(),0,self.comm)
        self.numPyArray1 = arange(self.length) * self.scale
        self.numPyArray2 = array([0,-1,2.17,-3.14,4,-5,6,-7,6.28])
        
    def tearDown(self):
        self.comm.Barrier()

    def testConstructor01(self):
        "Test Epetra.MultiVector (BlockMap,int,bool) constructor"
        emv = Epetra.MultiVector(self.map,2,True)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())

    def testConstructor02(self):
        "Test Epetra.MultiVector (BlockMap,int) constructor"
        emv = Epetra.MultiVector(self.map,3)
        self.assertEquals(emv.NumVectors(),3)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())

    def testConstructor03(self):
        "Test Epetra.MultiVector (BlockMap,bad-list) constructor"
        list = [0, 1.0, "e", "pi"]
        self.assertRaises(TypeError, Epetra.MultiVector, self.map, list)

    def testConstructor04(self):
        "Test Epetra.MultiVector (BlockMap,1D-small-list) constructor"
        list = [0, 1.0, 2, 3.14]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),1)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv[0,:len(list)] == list).all())

    def testConstructor05(self):
        "Test Epetra.MultiVector (BlockMap,1D-correct-list) constructor"
        list = [0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),1)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv[0,:] == list).all())

    def testConstructor06(self):
        "Test Epetra.MultiVector (BlockMap,1D-big-list) constructor"
        list = [0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0, -1, -2]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),1)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv[0,:] == list[:emv.shape[1]]).all())

    def testConstructor07(self):
        "Test Epetra.MultiVector (BlockMap,2D-small-list) constructor"
        list = [[0,    1.0, 2, 3.14],
                [2.17, 1.5, 1, 0.5 ]]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv[:,:len(list[0])] == list).all())

    def testConstructor08(self):
        "Test Epetra.MultiVector (BlockMap,2D-correct-list) constructor"
        list = [[0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0],
                [0.0, 0.5, 1, 1.5, 2.17, 3.14, 2, 1.0, 0]]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv == list).all())

    def testConstructor09(self):
        "Test Epetra.MultiVector (BlockMap,2D-big-list) constructor"
        list = [[0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0, -1, -2],
                [0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0, -1, -2]]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        for i in range(emv.shape[0]):
            self.failUnless((emv[i,:] == list[i][:emv.shape[1]]).all())

    def testConstructor10(self):
        "Test Epetra.MultiVector (BlockMap,3D-small-list) constructor"
        list = [[[0   , 1.0], [2, 3.14]],
                [[2.17, 1.5], [1, 0.5 ]]]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        for i in range(len(list)):
            for j in range(len(list[i])):
                for k in range(len(list[i][j])):
                    self.assertEquals(emv[i,j*2+k], list[i][j][k])

    def testConstructor11(self):
        "Test Epetra.MultiVector (BlockMap,3D-correct-list) constructor"
        list = [[[0  , 1.0, 2], [3.14, 2.17, 1.5 ], [1, 0.5, 0.0]],
                [[0.0, 0.5, 1], [1.5 , 2.17, 3.14], [2, 1.0, 0  ]]]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv == list).all())

    def testConstructor12(self):
        "Test Epetra.MultiVector (BlockMap,3D-big-list) constructor"
        list = [[[0, 1.0, 2, 3.14, 2.17], [1.5, 1, 0.5, 0.0, -1]],
                [[0, 1.0, 2, 3.14, 2.17], [1.5, 1, 0.5, 0.0, -1]],
                [[0, 1.0, 2, 3.14, 2.17], [1.5, 1, 0.5, 0.0, -1]]]
        emv = Epetra.MultiVector(self.map,list)
        self.assertEquals(emv.NumVectors(),3)
        self.assertEquals(emv.MyLength(), self.length)
        self.assertEquals(emv.GlobalLength(), self.length*comm.NumProc())
        for i in range(emv.shape[0]):
            for j in range(len(list[i])):
                self.assertEquals(emv[i,j], list[i][0][j])

    def testConstructor13(self):
        "Test Epetra.MultiVector (BlockMap) constructor"
        self.assertRaises(TypeError, Epetra.MultiVector, self.map)

    def testConstructor14(self):
        "Test Epetra.MultiVector (1D-list) constructor"
        list = [0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0]
        emv = Epetra.MultiVector(list)
        self.assertEquals(emv.NumVectors(),1)
        self.assertEquals(emv.MyLength(), len(list))
        self.assertEquals(emv.GlobalLength(), len(list))
        self.failUnless((emv[0,:] == list).all())

    def testConstructor15(self):
        "Test Epetra.MultiVector (2D-list) constructor"
        list = [[0, 1.0, 2, 3.14, 2.17, 1.5, 1, 0.5, 0.0],
                [0.0, 0.5, 1, 1.5, 2.17, 3.14, 2, 1.0, 0]]
        emv = Epetra.MultiVector(list)
        self.assertEquals(emv.NumVectors(),2)
        self.assertEquals(emv.MyLength(), 2*len(list[0]))
        self.assertEquals(emv.GlobalLength(), 2*len(list[0]))
        self.failUnless((emv == list).all())

    def testConstructor16(self):
        "Test Epetra.MultiVector (3D-list) constructor"
        list = [[[0   ,1.0,2  ], [3.14,2.17,1.5 ], [1,0.5,0.0 ]],
                [[0.0 ,0.5,1  ], [1.5 ,2.17,3.14], [2,1.0,0   ]],
                [[3.14,2  ,1.0], [0   ,0.0 ,0.5 ], [1,1.5,2.17]]]
        emv = Epetra.MultiVector(list)
        self.assertEquals(emv.NumVectors(),3)
        self.assertEquals(emv.MyLength(), 3*3*len(list[0][0]))
        self.assertEquals(emv.GlobalLength(), 3*3*len(list[0][0]))
        for i in range(emv.shape[0]):
            for j in range(len(list[i])):
                self.assertEquals(emv[i,j], list[i][j])

    def testConstructor17(self):
        "Test Epetra.MultiVector (bad-list) constructor"
        list = [0, 1.0, "e", "pi"]
        self.assertRaises(TypeError, Epetra.MultiVector, list)

    def testConstructor18(self):
        "Test Epetra.MultiVector (Copy,MultiVector,range-of-1) constructor"
        list = [[0, 1, 2, 3, 4, 5, 5, 4, 3],
                [2, 1, 0, 0, 1, 2, 3, 4, 5]]
        emv1  = Epetra.MultiVector(self.map,list)
        emv2  = Epetra.MultiVector(Epetra.Copy,emv1,(1,))
        self.assertEquals(emv2.NumVectors(),1)
        self.assertEquals(emv2.MyLength(),self.length)
        self.assertEquals(emv2.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv2[0,:] == emv1[1,:]).all())

    def testConstructor19(self):
        "Test Epetra.MultiVector (Copy,MultiVector,range-of-4) constructor"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        emv2  = Epetra.MultiVector(Epetra.Copy,emv1,range(4))
        self.assertEquals(emv2.NumVectors(),4)
        self.assertEquals(emv2.MyLength(),self.length)
        self.assertEquals(emv2.GlobalLength(), self.length*comm.NumProc())
        for i in range(emv2.shape[0]):
            self.assertEquals(emv2[i], emv1[i])

    def testConstructor20(self):
        "Test Epetra.MultiVector (Copy,MultiVector,discontinuous-range) constructor"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        indexes = (0,2,3,7)
        emv2  = Epetra.MultiVector(Epetra.Copy,emv1,indexes)
        self.assertEquals(emv2.NumVectors(),4)
        self.assertEquals(emv2.MyLength(),self.length)
        self.assertEquals(emv2.GlobalLength(), self.length*comm.NumProc())
        for i in range(emv2.shape[0]):
            self.assertEquals(emv2[i], emv1[indexes[i]])

    def testConstructor21(self):
        "Test Epetra.MultiVector (Copy,MultiVector,bad-list) constructor"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        indexes = [0, 1.0, "e", "pi"]
        self.assertRaises(TypeError, Epetra.MultiVector, Epetra.Copy, emv1, indexes)

    def testConstructor22(self):
        "Test Epetra.MultiVector (View,MultiVector,range-of-1) constructor"
        list = [[0, 1, 2, 3, 4, 5, 5, 4, 3],
                [2, 1, 0, 0, 1, 2, 3, 4, 5]]
        emv1  = Epetra.MultiVector(self.map,list)
        emv2  = Epetra.MultiVector(Epetra.View,emv1,(1,))
        self.assertEquals(emv2.NumVectors(),1)
        self.assertEquals(emv2.MyLength(),self.length)
        self.assertEquals(emv2.GlobalLength(), self.length*comm.NumProc())
        self.failUnless((emv2[0,:] == emv1[1,:]).all())

    def testConstructor23(self):
        "Test Epetra.MultiVector (View,MultiVector,range-of-4) constructor"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        emv2  = Epetra.MultiVector(Epetra.View,emv1,range(4))
        self.assertEquals(emv2.NumVectors(),4)
        self.assertEquals(emv2.MyLength(),self.length)
        self.assertEquals(emv2.GlobalLength(), self.length*comm.NumProc())
        for i in range(emv2.shape[0]):
            self.assertEquals(emv2[i], emv1[i])

    def testConstructor24(self):
        "Test Epetra.MultiVector (View,MultiVector,discontinuous-range) constructor"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        indexes = (0,2,3,7)
        emv2  = Epetra.MultiVector(Epetra.View,emv1,indexes)
        self.assertEquals(emv2.NumVectors(),4)
        self.assertEquals(emv2.MyLength(),self.length)
        self.assertEquals(emv2.GlobalLength(), self.length*comm.NumProc())
        for i in range(emv2.shape[0]):
            self.assertEquals(emv2[i], emv1[indexes[i]])

    def testConstructor25(self):
        "Test Epetra.MultiVector (View,MultiVector,bad-list) constructor"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        indexes = [0, 1.0, "e", "pi"]
        self.assertRaises(TypeError, Epetra.MultiVector, Epetra.View, emv1, indexes)

    def testConstructor26(self):
        "Test Epetra.MultiVector copy constructor"
        emv1 = Epetra.MultiVector(self.map,4)
        emv2 = Epetra.MultiVector(emv1)
        self.assertEquals(emv2.NumVectors(),   emv1.NumVectors()  )
        self.assertEquals(emv2.MyLength(),     emv1.MyLength()    )
        self.assertEquals(emv2.GlobalLength(), emv1.GlobalLength())

    def testReplaceMap1(self):
        "Test Epetra.MultiVector ReplaceMap method with good map"
        blockMap = Epetra.BlockMap(3*self.comm.NumProc(),3,0,self.comm)
        emv = Epetra.MultiVector(self.map,3)
        result = emv.ReplaceMap(blockMap)
        self.assertEquals(result, 0)
        newMap = emv.Map()
        self.assertEquals(newMap.ElementSize(), blockMap.ElementSize())

    def testReplaceMap2(self):
        "Test Epetra.MultiVector ReplaceMap method with bad map"
        blockMap = Epetra.BlockMap(2*self.comm.NumProc(),5,0,self.comm)
        emv = Epetra.MultiVector(self.map,4)
        result = emv.ReplaceMap(blockMap)
        self.assertEquals(result, -1)
        newMap = emv.Map()
        self.assertEquals(newMap.ElementSize(), self.map.ElementSize())

    def testReplaceGlobalValue1(self):
        "Test Epetra.MultiVector ReplaceGlobalValue method"
        emv = Epetra.MultiVector(self.map,self.numPyArray1)
        gid = 4
        lid = self.map.LID(gid)
        self.assertEquals(emv[0,gid], 0.5)
        result = emv.ReplaceGlobalValue(gid,0,5.0)
        if lid >= 0:
            self.assertEquals(result, 0)
            self.assertEquals(emv[0,lid], 5.0)
        else:
            self.assertEquals(result, 1)

    def testReplaceGlobalValue2(self):
        "Test Epetra.MultiVector ReplaceGlobalValue method for BlockMaps"
        map = Epetra.BlockMap(3*self.comm.NumProc(),3,0,self.comm)
        self.numPyArray1.shape = (1,3,3)  # 1 vector, 3 elements, 3 points per element
        emv = Epetra.MultiVector(map,self.numPyArray1)
        gid = 1
        lid = self.map.LID(gid)
        self.assertEquals(emv[0,gid,1], 0.5)
        result = emv.ReplaceGlobalValue(gid,1,0,5.0)
        if lid >= 0:
            self.assertEquals(result, 0)
            self.assertEquals(emv[0,lid,1], 5.0)
        else:
            self.assertEquals(result, 1)

    def testSumIntoGlobalValue1(self):
        "Test Epetra.MultiVector SumIntoGlobalValue method"
        emv = Epetra.MultiVector(self.map,self.numPyArray1)
        gid = 4
        lid = self.map.LID(gid)
        self.assertEquals(emv[0,gid], 0.5)
        result = emv.SumIntoGlobalValue(gid,0,0.5)
        if lid >= 0:
            self.assertEquals(result, 0)
            self.assertEquals(emv[0,lid], 1.0)
        else:
            self.assertEquals(result, 1)

    def testSumIntoGlobalValue2(self):
        "Test Epetra.MultiVector SumIntoGlobalValue method for BlockMaps"
        map = Epetra.BlockMap(3*self.comm.NumProc(),3,0,self.comm)
        self.numPyArray1.shape = (1,3,3)  # 1 vector, 3 elements, 3 points per element
        emv = Epetra.MultiVector(map,self.numPyArray1)
        gid = 1
        lid = self.map.LID(gid)
        self.assertEquals(emv[0,gid,1], 0.5)
        result = emv.SumIntoGlobalValue(gid,1,0,0.5)
        if lid >= 0:
            self.assertEquals(result, 0)
            self.assertEquals(emv[0,lid,1], 1.0)
        else:
            self.assertEquals(result, 1)

    def testReplaceMyValue1(self):
        "Test Epetra.MultiVector ReplaceMyValue method"
        emv = Epetra.MultiVector(self.map,self.numPyArray1)
        lid = 4
        self.assertEquals(emv[0,lid], 0.5)
        result = emv.ReplaceMyValue(lid,0,5.0)
        self.assertEquals(result, 0)
        self.assertEquals(emv[0,lid], 5.0)

    def testReplaceMyValue2(self):
        "Test Epetra.MultiVector ReplaceMyValue method for BlockMaps"
        map = Epetra.BlockMap(3*self.comm.NumProc(),3,0,self.comm)
        self.numPyArray1.shape = (1,3,3)  # 1 vector, 3 elements, 3 points per element
        emv = Epetra.MultiVector(map,self.numPyArray1)
        lid = 1
        self.assertEquals(emv[0,lid,1], 0.5)
        result = emv.ReplaceMyValue(lid,1,0,5.0)
        self.assertEquals(result, 0)
        self.assertEquals(emv[0,lid,1], 5.0)

    def testSumIntoMyValue1(self):
        "Test Epetra.MultiVector SumIntoMyValue method"
        emv = Epetra.MultiVector(self.map,self.numPyArray1)
        lid = 4
        self.assertEquals(emv[0,lid], 0.5)
        result = emv.SumIntoMyValue(lid,0,0.5)
        self.assertEquals(result, 0)
        self.assertEquals(emv[0,lid], 1.0)

    def testSumIntoMyValue2(self):
        "Test Epetra.MultiVector SumIntoMyValue method for BlockMaps"
        map = Epetra.BlockMap(3*self.comm.NumProc(),3,0,self.comm)
        self.numPyArray1.shape = (1,3,3)  # 1 vector, 3 elements, 3 points per element
        emv = Epetra.MultiVector(map,self.numPyArray1)
        lid = 1
        self.assertEquals(emv[0,lid,1], 0.5)
        result = emv.SumIntoMyValue(lid,1,0,0.5)
        self.assertEquals(result, 0)
        self.assertEquals(emv[0,lid,1], 1.0)

    def testPutScalar(self):
        "Test Epetra.MultiVector PutScalar method"
        numVec = 3
        emv    = Epetra.MultiVector(self.map,numVec,True)
        self.failUnless((emv == 0.0).all())
        scalar = 3.14
        emv.PutScalar(scalar)
        self.failUnless((emv == scalar).all())

    def testRandom(self):
        "Test Epetra.MultiVector Random method"
        numVec = 2
        emv    = Epetra.MultiVector(self.map,numVec)
        scalar = 3.14
        emv.PutScalar(scalar)
        self.failUnless((emv == scalar).all())
        emv.Random()
        for v in range(numVec):
            for i in range(self.map.NumMyPoints()):
                self.failUnless(emv[v,i] > -1.0)
                self.failUnless(emv[v,i] <  1.0)

    def testExtractCopy(self):
        "Test Epetra.MultiVector ExtractCopy method"
        a     = [self.numPyArray1] * 4
        emv   = Epetra.MultiVector(self.map,a)
        array = emv.ExtractCopy()
        self.assertEquals(type(array), ArrayType)
        self.failUnless((emv == array).all())
        self.assertEquals(emv.array is array, False)

    def testExtractView(self):
        "Test Epetra.MultiVector ExtractView method"
        a     = [self.numPyArray1] * 4
        emv   = Epetra.MultiVector(self.map,a)
        array = emv.ExtractView()
        self.assertEquals(type(array), ArrayType)
        self.failUnless((emv == array).all())
        self.assertEquals(emv.array is array, True)

    def testNumVectors(self):
        "Test Epetra.MultiVector NumVectors method"
        for i in range(1,8):
            emv = Epetra.MultiVector(self.map,i)
            self.assertEquals(emv.NumVectors(), i)

    def testMyLength(self):
        "Test Epetra.MultiVector MyLength method"
        a   = [self.numPyArray1] * 3
        emv = Epetra.MultiVector(self.map,a)
        self.assertEquals(emv.MyLength(), self.length)

    def testGlobalLength(self):
        "Test Epetra.MultiVector GlobalLength method"
        a   = [self.numPyArray1] * 3
        emv = Epetra.MultiVector(self.map,a)
        self.assertEquals(emv.GlobalLength(), self.length*self.comm.NumProc())

    def testConstantStride(self):
        "Test Epetra.MultiVector ConstantStride method"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv1  = Epetra.MultiVector(self.map,multiArray)
        indexes = (0,2,3,7)
        emv2  = Epetra.MultiVector(Epetra.View,emv1,indexes)
        self.assertEquals(emv1.ConstantStride(), True )
        self.assertEquals(emv2.ConstantStride(), False)

    def testStride(self):
        "Test Epetra.MultiVector Stride method"
        squareArray = [[-1.2,  3.4, -5.6],
                       [ 7.8, -9.0,  1.2],
                       [-3.4,  5.6, -7.8]]
        multiArray = [squareArray] * 8
        emv  = Epetra.MultiVector(self.map,multiArray)
        self.assertEquals(emv.Stride(), 9)

    def testSeed(self):
        "Test Epetra.MultiVector Seed method"
        emv  = Epetra.MultiVector(self.map,1)
        seed = emv.Seed()
        max  = 2**31 - 1
        self.assertEquals(seed>0,   True)
        self.assertEquals(seed<max, True)

    def testSetSeed1(self):
        "Test Epetra.MultiVector SetSeed method"
        emv    = Epetra.MultiVector(self.map,1)
        seed   = 2005
        result = emv.SetSeed(seed)
        self.assertEquals(result,     0   )
        self.assertEquals(emv.Seed(), seed)

    def testSetSeed2(self):
        "Test Epetra.MultiVector SetSeed method with negative seed"
        emv  = Epetra.MultiVector(self.map,1)
        seed = -2005
        # The exception type depends on the version of SWIG used to generate the
        # wrapper
        self.assertRaises((TypeError,OverflowError),emv.SetSeed,seed)

    def testPrint(self):
        "Test Epetra.MultiVector Print method"
        output = ""
        if self.comm.MyPID() == 0:
            output += "%10s%14s%20s%20s  \n" % ("MyPID","GID","Value","Value")
        for lid in range(self.length):
            gid = self.map.GID(lid)
            output += "%10d%14d%24d%20d\n" % (self.comm.MyPID(),gid,0,0)
        emv = Epetra.MultiVector(self.map,2,True)
        filename = "testMultiVector%d.dat" % self.comm.MyPID()
        f = open(filename,"w")
        emv.Print(f)
        f.close()
        self.assertEqual(open(filename,"r").read(), output)

    def testDot(self):
        "Test Epetra.MultiVector Dot method"
        map    = Epetra.Map(4*self.comm.NumProc(),0,self.comm)
        array1 = [[-1, 2,-3, 4],
                  [ 5, 1,-8,-7]]
        array2 = [[ 9, 0,-1,-2],
                  [-7,-8, 1, 5]]
        emv1   = Epetra.MultiVector(map,array1)
        emv2   = Epetra.MultiVector(map,array2)
        dot    = emv1.Dot(emv2)
        result = array([-14,-86])*self.comm.NumProc()
        self.failUnless((dot == result).all())

    def testAbs(self):
        "Test Epetra.MultiVector Abs method"
        a    = array([self.numPyArray1,self.numPyArray2])
        emv1 = Epetra.MultiVector(self.map,a)
        emv2 = Epetra.MultiVector(self.map,2)
        self.assertEquals(emv2[:],0.0)
        result = emv2.Abs(emv1)
        self.assertEquals(result,0)
        self.assertEquals(emv2[:],abs(a))

    def testReciprocal(self):
        "Test Epetra.MultiVector Reciprocal method"
        a    = array([self.numPyArray1,self.numPyArray2])
        a[0,0] = a[1,0] = 1.0  # Don't want to invert zero
        emv1 = Epetra.MultiVector(self.map,a)
        emv2 = Epetra.MultiVector(self.map,2)
        self.assertEquals(emv2[:],0.0)
        result = emv2.Reciprocal(emv1)
        self.assertEquals(result,0)
        self.assertEquals(emv2[:],1.0/a)

    def testScale1(self):
        "Test Epetra.MultiVector Scale method in-place"
        a   = array([self.numPyArray1,self.numPyArray2])
        emv = Epetra.MultiVector(self.map,a)
        self.assertEquals(emv[:],0.0)
        result = emv.Scale(2.0)
        self.assertEquals(result,0)
        self.assertEquals(emv[:],2.0*a)

    def testScale2(self):
        "Test Epetra.MultiVector Scale method with replace"
        a    = array([self.numPyArray1,self.numPyArray2])
        emv1 = Epetra.MultiVector(self.map,a)
        emv2 = Epetra.MultiVector(self.map,2)
        self.assertEquals(emv2[:],0.0)
        result = emv2.Scale(pi,emv1)
        self.assertEquals(result,0)
        self.assertEquals(emv2[:],pi*a)

    def testUpdate1(self):
        "Test Epetra.MultiVector Update method with one MultiVector"
        emv1 = Epetra.MultiVector(self.map,self.numPyArray1)
        emv2 = Epetra.MultiVector(self.map,self.numPyArray2)
        result = emv2.Update(2.0,emv1,3.0)
        self.assertEquals(result,0)
        self.assertEquals(emv2[:],2.0*self.numPyArray1 + 3.0*self.numPyArray2)

    def testUpdate2(self):
        "Test Epetra.MultiVector Update method with two MultiVectors"
        emv0 = Epetra.MultiVector(self.map,1               )
        emv1 = Epetra.MultiVector(self.map,self.numPyArray1)
        emv2 = Epetra.MultiVector(self.map,self.numPyArray2)
        result = emv0.Update(2.0,emv1,3.0,emv2,pi)
        self.assertEquals(result,0)
        self.assertEquals(emv0[:],2.0*self.numPyArray1 + 3.0*self.numPyArray2)

    def testNorm1(self):
        "Test Epetra.MultiVector Norm1 method"
        a      = [self.numPyArray1,self.numPyArray2]
        emv    = Epetra.MultiVector(self.map,a)
        result = [sum(abs(self.numPyArray1)) * self.comm.NumProc(),
                  sum(abs(self.numPyArray2)) * self.comm.NumProc()]
        norm1  = emv.Norm1()
        self.assertEquals(len(norm1), 2)
        self.failUnless((norm1 == result).all())

    def testNorm2(self):
        "Test Epetra.MultiVector Norm2 method"
        a      = [self.numPyArray1,self.numPyArray2]
        emv    = Epetra.MultiVector(self.map,a)
        result = [sqrt(sum(self.numPyArray1*self.numPyArray1) * self.comm.NumProc()),
                  sqrt(sum(self.numPyArray2*self.numPyArray2) * self.comm.NumProc())]
        norm2  = emv.Norm2()
        self.assertEquals(len(norm2), 2)
        self.failUnless((norm2 == result).all())

    def testNormInf(self):
        "Test Epetra.MultiVector NormInf method"
        a       = [self.numPyArray1,self.numPyArray2]
        emv     = Epetra.MultiVector(self.map,a)
        result  = [max(abs(self.numPyArray1)),max(abs(self.numPyArray2))]
        normInf = emv.NormInf()
        self.assertEquals(len(normInf), 2)
        self.failUnless((normInf == result).all())

    def testNormWeighted(self):
        "Test Epetra.MultiVector NormWeighted method"
        a       = array([self.numPyArray1,self.numPyArray2])
        emv     = Epetra.MultiVector(self.map,a)
        wts     = sin(pi*(arange(self.length) + 0.5) / self.length)
        weights = Epetra.MultiVector(self.map,wts)
        result  = sqrt(sum((a/wts)**2,1)/self.length)
        norm    = emv.NormWeighted(weights)
        self.assertEquals(len(norm), 2)
        self.failUnless((norm == result).all())

    def testMinValue(self):
        "Test Epetra.MultiVector MinValue method"
        a        = [self.numPyArray1,self.numPyArray2]
        emv      = Epetra.MultiVector(self.map,a)
        result   = [min(self.numPyArray1),min(self.numPyArray2)]
        minValue = emv.MinValue()
        self.assertEquals(len(minValue), 2)
        self.failUnless((minValue == result).all())

    def testMaxValue(self):
        "Test Epetra.MultiVector MaxValue method"
        a        = [self.numPyArray1,self.numPyArray2]
        emv      = Epetra.MultiVector(self.map,a)
        result   = [max(self.numPyArray1),max(self.numPyArray2)]
        maxValue = emv.MaxValue()
        self.assertEquals(len(maxValue), 2)
        self.failUnless((maxValue == result).all())

    def testMeanValue(self):
        "Test Epetra.MultiVector MeanValue method"
        a         = [self.numPyArray1,self.numPyArray2]
        emv       = Epetra.MultiVector(self.map,a)
        result    = [sum(self.numPyArray1)/self.length,
                     sum(self.numPyArray2)/self.length]
        meanValue = emv.MeanValue()
        self.assertEquals(len(meanValue), 2)
        self.failUnless((meanValue == result).all())

    def testMultiply1(self):
        "Test Epetra.MultiVector Multiply method"
        n    = 2 * self.comm.NumProc()
        map  = Epetra.Map(n,0,self.comm)
        emv0 = Epetra.MultiVector(map,n)
        emv1 = Epetra.MultiVector(map,n)
        emv2 = Epetra.MultiVector(map,n)
        emv0.Random()
        emv1.Random()
        emv2.Random()
        result = emv0.Multiply(1.0,emv1,emv2,2.0)
        self.assertEquals(result,0)

# The following unit test has been commented out because Epetra does not
# currently support this form (transposed) of multivector multiplication
#     def testMultiply2(self):
#         "Test Epetra.MultiVector Multiply method with transpose"
#         n    = 2 * self.comm.NumProc()
#         map  = Epetra.Map(n,0,self.comm)
#         emv0 = Epetra.MultiVector(map,1)
#         emv1 = Epetra.MultiVector(map,n)
#         emv2 = Epetra.MultiVector(map,1)
#         emv0.Random()
#         emv1.Random()
#         emv2.Random()
#         result = emv0.Multiply('T','N',3.0,emv1,emv2,4.0)
#         self.assertEquals(result,0)
#         result = emv0.Multiply('N','T',5.0,emv1,emv2,5.0)
#         self.assertEquals(result,0)
#         result = emv0.Multiply('T','T',7.0,emv1,emv2,6.0)
#         self.assertEquals(result,0)

    def testSetArray(self):
        "Test Epetra.MultiVector __setattr__ method for 'array'"
        ev = Epetra.Vector(self.map)
        self.assertRaises(AttributeError, ev.__setattr__, "array", "junk")

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraMultiVectorTestCase))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
       "\n**************************\nTesting Epetra.MultiVector\n**************************\n"
    verbosity = 2 * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
