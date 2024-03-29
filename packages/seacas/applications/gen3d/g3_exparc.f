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

C   $Id: exparc.f,v 1.4 2005/03/30 19:18:53 gdsjaar Exp $
C=======================================================================
      SUBROUTINE EXPARC (XN, YN, XN3, YN3, ZN3, IXNP, NRNP, NPCEN,
     &   SINANG, COSANG)
C=======================================================================

C   --*** EXPARC *** (GEN3D) Calculate 3D coordinates
C   --   Written by Amy Gilkey - revised 05/09/88
C   --   Modified by Greg Sjaardema - 02/06/89
C   --       Added Warp Function
C   --       Added Gradient to Rotations (not for center blocks)
C   --
C   --NEWXYZ calculates the coordinate array for the 3D database.
C   --
C   --Parameters:
C   --   XN, YN - IN - the 2D coordinates, destroyed
C   --   XN3, YN3, ZN3 - OUT - the 3D coordinates
C   --   IXNP - IN - the new index for each node
C   --   NRNP - IN - the number of new nodes generated for each node
C   --   NPCEN - IN - the node numbers of the center nodes by column and row
C   --   SINANG, COSANG - SCRATCH - size = NNREPL, holds sin and cos of
C   --      angles for rotations
C   --
C   --Common Variables:
C   --   Uses NDIM, NUMNP of /DBNUMS/
C   --   Uses NDIM3, NUMNP3 of /DBNUM3/
C   --   Uses DOTRAN, NNREPL, DIM3, NRTRAN, D3TRAN, ZGRAD,
C   --      CENTER, NUMCOL, NUMROW of /PARAMS/

      INCLUDE 'g3_dbnums.blk'
      INCLUDE 'g3_dbnum3.blk'
      INCLUDE 'g3_params.blk'

      REAL XN(NUMNP), YN(NUMNP),
     &   XN3(NUMNP3), YN3(NUMNP3), ZN3(NUMNP3)
      INTEGER IXNP(*), NRNP(*)
      INTEGER NPCEN(NUMCDM,*)
      REAL SINANG(NNREPL), COSANG(NNREPL)

      PI = ATAN2(0.0, -1.0)
C      --For rotations, change X and add Z so that they define pie-shaped
C      --regions around the Y axis

C   --Copy Y coordinate from original

         DO 520 INP = 1, NUMNP
            JNP0 = IXNP(INP) - 1
            DO 510 NR = 1, NRNP(INP)
               YN3(JNP0+NR) = YN(INP)
  510       CONTINUE
  520    CONTINUE

C ... Check minimum X coordinate to see if we will generate bad elements
         XMIN = XN(1)
         DO 5 INP = 2, NUMNP
           XMIN = MIN(XMIN, XN(INP))
 5       CONTINUE
         XMIN = XMIN - CENTER
         if (XMIN .LT. 0) THEN
           call prterr('WARNING',
     &       'Input mesh crosses over axis of rotation (Y). Negative '//
     &       'elements may be generated. Adjust ROTCEN or input mesh.')
         end if

C      --Subtract center, so rotation is around zero

      IF (CENTER .NE. 0.0) THEN
         DO 20 INP = 1, NUMNP
            XN(INP) = XN(INP) - CENTER
   20    CONTINUE
      END IF

C      --Get the coordinates for the non-center nodes 

      IBLK = 0
      NXTNR = 1

      ZEND = 0.0
      DEGR = 0.0
    1    CONTINUE
         IBLK = IBLK + 1
         IF (NRTRAN(IBLK) .GT. 0) THEN
            ZBEG = ZEND
            ZEND = ZBEG + D3TRAN(IBLK) * PI / 180.
            DEGR = DEGR + D3TRAN(IBLK)
            if (DEGR .lt. 360.0) then
               nrot = nrtran(iblk)+1
            else
               nrot = nrtran(iblk)
            end if
            CALL INIGRD (-ZBEG, -ZEND, ZGRAD(IBLK), 
     *         NRTRAN(IBLK), nrot, COSANG(NXTNR) )
            NXTNR = NXTNR + NRTRAN(IBLK)
            IF (IBLK .LT. MAXINT) GO TO 1
         END IF

         DO 30 NR = 1, NNREPL
            ANG = COSANG(NR)
            SINANG(NR) = SIN (ANG)
            COSANG(NR) = COS (ANG)
   30    CONTINUE

      DO 60 INP = 1, NUMNP
         IF (NRNP(INP) .EQ. NNREPL) THEN
            JNP = IXNP(INP)
            X = XN(INP)
            DO 50 NR = 1, NNREPL
               XN3(JNP) = X * COSANG(NR)
               ZN3(JNP) = X * SINANG(NR)
               JNP = JNP + 1
   50       CONTINUE
         END IF
   60 CONTINUE

C      --Add center of rotation

      IF (CENTER .NE. 0.0) THEN
         DO 180 JNP = 1, NUMNP3
            XN3(JNP) = XN3(JNP) + CENTER
  180    CONTINUE
      END IF
      RETURN
      END
