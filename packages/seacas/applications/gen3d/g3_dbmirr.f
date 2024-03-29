C Copyright(C) 2011 Sandia Corporation.  Under the terms of Contract
C DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
C certain rights in this software
C 
C Redistribution and use in source and binary forms, with or without
C modification, are permitted provided that the following conditions are
C met:
C 
C * Redistributions of source code must retain the above copyright
C    notice, this list of conditions and the following disclaimer.
C           
C * Redistributions in binary form must reproduce the above
C   copyright notice, this list of conditions and the following
C   disclaimer in the documentation and/or other materials provided
C   with the distribution.
C                         
C * Neither the name of Sandia Corporation nor the names of its
C   contributors may be used to endorse or promote products derived
C   from this software without specific prior written permission.
C                                                 
C THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
C "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
C LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
C A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
C OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
C SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
C LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
C DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
C THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
C (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
C OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
C 

C   $Id: dbmirr.f,v 1.2 1999/01/25 16:38:36 gdsjaar Exp $
C=======================================================================
      SUBROUTINE DBMIRR (NELBS, NELBE, IDELB, NUMELB, NUMLNK, LINK)
C=======================================================================

C   --*** DBMIRR *** (GEN3D) Fixup connectivity for reflections
C   --   Written by Greg Sjaardema - revised 02/10/89
C   --   Modified from DBOELB Written by Amy Gilkey
C   --
C   --Parameters:
C   --   NELBS, NELBE - IN - the number of first and last element blocks
C   --   IDELB - IN - the element block IDs for each block
C   --   NUMELB - IN - the number of elements in each block
C   --   NUMLNK - IN - the number of nodes per element in each block
C   --   LINK - IN/OUT - the connectivity for each block
C   --

      INTEGER IDELB(*)
      INTEGER NUMELB(*)
      INTEGER NUMLNK(*)
      INTEGER LINK(*)

      IELNK = 0

      DO 10 NELB = NELBS, NELBE
         IELB = NELB-NELBS+1

         ISLNK = IELNK + 1
         IELNK = IELNK + NUMLNK(IELB) * NUMELB(IELB)

         CALL DBMIR1 (NELB, NUMELB(IELB), NUMLNK(IELB), LINK(ISLNK))
   10 CONTINUE

      RETURN
      END
