!*********************************************************************
! ForTrilinos: Object-Oriented Fortran 2003 interface to Trilinos
!                Copyright 2010 Sandia Corporation
!
! Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
! the U.S. Government retains certain rights in this software.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!
! 1. Redistributions of source code must retain the above copyright
!    notice, this list of conditions and the following disclaimer.
!
! 2. Redistributions in binary form must reproduce the above copyright
!    notice, this list of conditions and the following disclaimer in the
!    documentation and/or other materials provided with the distribution.
!
! 3. Neither the name of the Corporation nor the names of the
!    contributors may be used to endorse or promote products derived from
!    this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
! EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
! PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
! CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
! EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
! PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
! PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
! LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
! NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
! SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!
! Questions? Contact Karla Morris  (knmorri@sandia.gov) or
!                    Damian Rouson (rouson@sandia.gov)
!*********************************************************************


module F$Package_$Class
  use ForTrilinos_enums ,only: FT_$Package_$Class_ID_t,ForTrilinos_Universal_ID_t
  use ForTrilinos_table_man ,only : CT_Alias
  use ForTrilinos_universal ,only : universal
  use ForTrilinos_error ,only : error
  use F$Package_$ComponentClass , only: $Package_$ComponentClass
  use for$Package
  implicit none
  private                   ! Hide everything by default
  public :: $Package_$Class ! Expose type/constructors/methods

  type ,extends(universal)      :: $Package_$Class 
    private
    type(FT_$Package_$Class_ID_t) :: $Class_id 
    type($Package_$ComponentClass) :: $ComponentClass
  contains
     !User interface -- insert here type-bound procedures intended for use by end applications:

     !Developers only -- to be called by developers from other ForTrilinos modules, not by end applications:
     procedure         :: invalidate_id => invalidate_$Package$Class_ID
     procedure         :: ctrilinos_delete => ctrilinos_delete_$Package$Class
     procedure         :: component_finalization => component_finalization_$Package$Class
     procedure         :: get_$Package$Class_ID 
     procedure ,nopass :: alias_$Package$Class_ID
     procedure         :: generalize 
  end type

   interface $Package_$Class 
     !User interface -- constructors for use by end applications:
     module procedure create,duplicate
     !Developers only -- to be called by developers from other ForTrilinos modules, not by end applications:
     module procedure from_struct
   end interface
 
contains

  ! Common type-bound procedures begin here: all ForTrilinos classes at the base of the API have procedures analogous to these.
  ! The function from_struct should be called by developers only from other ForTrilinos modules, never by end applications:

  type($Package_$Class) function from_struct(id)
     type(FT_$Package_$Class_ID_t) ,intent(in) :: id
     from_struct%$Class_id = id
     from_struct%$ComponentClass = $Package_$ComponentClass(from_struct%$ComponentClass%alias_$Package$ComponentClass_ID(from_struct%generalize()))
     call from_struct%register_self()
  end function
 
  ! All additional constructors should take two steps: (1) obtain a struct ID by invoking a procedural binding and then (2) pass
  ! this ID to from_struct to initialize the constructed object's ID component and register the object for reference counting.
 
  type($Package_$Class) function create($args)
    ! Declare args
    type(FT_$Package_$Class_ID_t) :: create_id
    create_id = $Package_$Class_Create($args)
    create = from_struct(create_id)
  end function

  type($Package_$Class) function duplicate(this)
    type($Package_$Class) ,intent(in) :: this 
    duplicate = from_struct($Package_$Class_Duplicate(this%$Class_id))
  end function

  !----------------- Struct access ---------------------------------------------

  type(FT_$Package_$Class_ID_t) function get_$Package$Class_ID(this)
    class($Package_$Class) ,intent(in) :: this 
    get_$Package$Class_ID=this%$Class_id
  end function

  !----------------- Type casting ---------------------------------------------
  
  type(FT_$Package_$Class_ID_t) function alias_$Package$Class_ID(generic_id)
    use ForTrilinos_table_man, only : CT_Alias
    use ForTrilinos_enums ,only: ForTrilinos_Universal_ID_t,FT_$Package_$Class_ID
    use iso_c_binding     ,only: c_loc,c_int
    type(ForTrilinos_Universal_ID_t) ,intent(in) :: generic_id
    type(ForTrilinos_Universal_ID_t) ,allocatable ,target :: alias_id
    integer(c_int) :: status
    type(error) :: ierr
    allocate(alias_id,source=CT_Alias(generic_id,FT_$Package_$Class_ID),stat=status)
    ierr=error(status,'F$Package_$Class:alias_$Package$Class_ID')
    call ierr%check_success()
    alias_$Package$Class_ID=degeneralize_$Package$Class(c_loc(alias_id))
  end function

  type(ForTrilinos_Universal_ID_t) function generalize(this)
   use ForTrilinos_utils ,only: generalize_all
   use iso_c_binding     ,only: c_loc
   class($Package_$Class) ,intent(in) ,target :: this
   generalize = generalize_all(c_loc(this%$Class_id))
  end function

 type(FT_$Package_$Class_ID_t) function degeneralize_$Package$Class(generic_id) bind(C)
    use ForTrilinos_enums ,only : ForTrilinos_Universal_ID_t,FT_$Package_$Class_ID_t
    use ,intrinsic :: iso_c_binding ,only: c_ptr,c_f_pointer
    type(c_ptr)                   ,value   :: generic_id
    type(FT_$Package_$Class_ID_t) ,pointer :: local_ptr=>null()
    call c_f_pointer (generic_id, local_ptr)
    degeneralize_$Package$Class = local_ptr
  end function
 
 !__________ Garbage collection __________________________________________________
 
  subroutine invalidate_$Package$Class_ID(this)
    class($Package_$Class),intent(inout) :: this
    call this%$ComponentClass%invalidate_id
    this%$Class_id%table = FT_Invalid_ID
    this%$Class_id%index = FT_Invalid_Index 
    this%$Class_id%is_const = FT_FALSE
  end subroutine

  subroutine component_finalization_$Package$Class(this)
    class($Package_$Class),intent(inout) :: this
    call this%$Package_$ComponentClass%force_finalize() 
  end subroutine

  subroutine ctrilinos_delete_$Package$Class(this)
    class($Package_$Class),intent(inout) :: this
    call $Package_$Class_Destroy( this%$Class_id ) 
  end subroutine

  ! Package-specific type-cound procedures begin here

end module 
