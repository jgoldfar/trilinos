// /////////////////////////////////////////////////////////////////////////////
// DecompositionSystemVarReductImp.cpp
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.

#include <typeinfo>

#include "ConstrainedOptimizationPack/src/DecompositionSystemVarReductImp.hpp"
#include "ConstrainedOptimizationPack/src/MatrixIdentConcatStd.hpp"
#include "ConstrainedOptimizationPack/src/MatrixVarReductImplicit.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/MatrixOpNonsing.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/MatrixOpSubView.hpp"
#include "AbstractFactoryStd.hpp"
#include "dynamic_cast_verbose.hpp"
#include "ThrowException.hpp"

namespace ConstrainedOptimizationPack {

DecompositionSystemVarReductImp::DecompositionSystemVarReductImp(
	const VectorSpace::space_ptr_t     &space_x
	,const VectorSpace::space_ptr_t    &space_c
	,const VectorSpace::space_ptr_t    &space_h
	,const basis_sys_ptr_t             &basis_sys
	,const basis_sys_tester_ptr_t      &basis_sys_tester
	,EExplicitImplicit                 D_imp
	,EExplicitImplicit                 Uz_imp
	,EExplicitImplicit                 Vz_imp
	)
	:DecompositionSystemVarReduct(D_imp,Uz_imp,Vz_imp)
	,basis_sys_tester_(basis_sys_tester)
	,D_imp_used_(MAT_IMP_AUTO)
{
	this->initialize(space_x,space_c,space_h,basis_sys);
}

void DecompositionSystemVarReductImp::initialize(
	const VectorSpace::space_ptr_t     &space_x
	,const VectorSpace::space_ptr_t    &space_c
	,const VectorSpace::space_ptr_t    &space_h
	,const basis_sys_ptr_t             &basis_sys
	)
{
	namespace rcp = MemMngPack;
	size_type num_vars = 0;
#ifdef _DEBUG
	THROW_EXCEPTION(
		basis_sys.get() != NULL && (space_x.get() == NULL || space_c.get() == NULL)
		,std::invalid_argument
		,"DecompositionSystemVarReductImp::initialize(...) : Error, "
		"if basis_sys is set, then space_x and space_c must also be set!" );
#endif
	if(basis_sys.get()) {
		num_vars = basis_sys->var_dep().size() + basis_sys->var_indep().size();
#ifdef _DEBUG
		const size_type num_inequ_decomp = basis_sys->inequ_decomp().size();
		THROW_EXCEPTION(
			num_inequ_decomp > 0, std::invalid_argument
			,"DecompositionSystemVarReductImp::initialize(...) : Error, "
			"decomposed inequality constraints can not be handled!" );
		const size_type
			space_x_dim = space_x->dim(),
			space_c_dim = space_c->dim(),
			num_equ     = basis_sys->equ_decomp().size() + basis_sys->equ_undecomp().size();
		THROW_EXCEPTION(
			num_vars != 0 && (space_x_dim != num_vars || space_c_dim != num_equ)
			, std::invalid_argument
			,"DecompositionSystemVarReductImp::initialize(...) : Error, "
			"the dimensions of space_x, space_c and basis_sys do not match up!" );
		if( space_h.get() ) {
			const size_type
				space_h_dim = space_h->dim(),
				num_inequ   = basis_sys->inequ_decomp().size() + basis_sys->inequ_undecomp().size();
			THROW_EXCEPTION(
				num_vars != 0 && (space_h_dim != num_inequ), std::invalid_argument
				,"DecompositionSystemVarReductImp::initialize(...) : Error, "
				"the dimensions of space_h, and basis_sys do not match up!" );
		}
#endif
	}
	space_x_    = space_x;
	space_c_    = space_c;
	space_h_    = space_h;
	basis_sys_  = basis_sys;
	if(num_vars) {
		space_range_ = space_x_->sub_space(basis_sys->var_dep())->clone();
		space_null_  = space_x_->sub_space(basis_sys->var_indep())->clone();
	}
	else {
		space_range_ = rcp::null;
		space_null_  = rcp::null;
	}
}

// Basis manipulation

void DecompositionSystemVarReductImp::get_basis_matrices(
	std::ostream                                          *out
	,EOutputLevel                                         olevel
	,ERunTests                                            test_what
	,MatrixOp                                         *Z
	,MatrixOp                                         *Y
	,MatrixOpNonsing                              *R
	,MatrixOp                                         *Uz
	,MatrixOp                                         *Uy
	,MatrixOp                                         *Vz
	,MatrixOp                                         *Vy
	,MemMngPack::ref_count_ptr<MatrixOpNonsing>   *C_ptr
	,MemMngPack::ref_count_ptr<MatrixOp>              *D_ptr
	)
{

	// ToDo: Implement undecomposed general equalities and general inequalities

	namespace rcp = MemMngPack;
	using DynamicCastHelperPack::dyn_cast;

	if( out && olevel >= PRINT_BASIC_INFO )
		*out << "\n****************************************************************"
			 << "\n*** DecompositionSystemVarReductImp::get_basis_matrices(...) ***"
			 << "\n****************************************************************\n";

	// ToDo: Validate input!

	//
	// Get references to concrete matrix objects
	//

	MatrixIdentConcatStd
		*Z_vr = Z ? &dyn_cast<MatrixIdentConcatStd>(*Z) : NULL;

	//
	// Make all matrices but Z uninitialized to determine if we can
	// reuse the Z.D matrix object (explicit or implicit).
	// Also, return a reference to the basis matrix C from the
	// R object.
	//

	(*C_ptr) = uninitialize_matrices(out,olevel,Y,R,Uy,Vy);

	//
	// Determine if we should be using an explicit or implicit D = -inv(C)*N object
	// (if we are allowed to choose).
	//
	update_D_imp_used(&D_imp_used_);

	//
	// Determine if we need to allocate a new matrix object for Z.D.
	// Also, get a reference to the explicit D matrix (if one exits)
	// and remove any reference to the basis matrix C by Z.D.
	//

	bool new_D_mat_object; // compiler should warn if used before initialized!
	if( Z_vr ) {
		if( Z_vr->D_ptr().get() == NULL ) {
			if( out && olevel >= PRINT_BASIC_INFO )
				*out << "\nMust allocate a new matrix object for D = -inv(C)*N since "
					 << "one has not been allocated yet ...\n";
			new_D_mat_object = true;
		}
		else {
			MatrixVarReductImplicit
				*D_vr = dynamic_cast<MatrixVarReductImplicit*>(
					const_cast<MatrixOp*>(Z_vr->D_ptr().get()) );
			// We may have to reallocate a new object if someone is sharing it or
			// if we are switching from implicit to explicit or visa-versa.
			if( Z_vr->D_ptr().count() > 1 ) {
				if( out && olevel >= PRINT_BASIC_INFO )
					*out << "\nMust allocate a new matrix object for D = -inv(C)*N since someone "
						 << "else is using the current one ...\n";
				new_D_mat_object = true;
			}
			else if( D_vr != NULL ) {
				if( D_imp_used_ == MAT_IMP_EXPLICIT ) {
					if( out && olevel >= PRINT_BASIC_INFO )
						*out << "\nMust allocate a new matrix object for D = -inv(C)*N since we "
							 << "are switching from implicit to explicit ...\n";
					new_D_mat_object = true;
				}
			}
			else if( D_imp_used_ == MAT_IMP_IMPLICIT ) {
				if( out && olevel >= PRINT_BASIC_INFO )
					*out << "\nMust allocate a new matrix object for D = -inv(C)*N since we "
						 << "are switching from explicit to implicit ...\n";
				new_D_mat_object = true;
			}
			// Remove the reference to the basis matrix C
			if(D_vr)
				D_vr->set_uninitialized();
		}
	}
	else {
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nMust allocate a new matrix object for D = -inv(C)*N since "
				 << " Z == NULL was passed in ...\n";
		new_D_mat_object = true;
	}

	//
	// Get the matrix object of D and allocate a new matrix object if needed.
	//

	rcp::ref_count_ptr<MatrixOp> _D_ptr;
	if( new_D_mat_object) {
		// Create a new matrix object!
		alloc_new_D_matrix( out, olevel, &_D_ptr );
	}
	else {
		// Use current matrix object!
		_D_ptr = rcp::rcp_const_cast<MatrixOp>(Z_vr->D_ptr());
	}

	// Set cached implicit D matrix or external explicit D matrix
	if(D_imp_used_ == MAT_IMP_IMPLICIT) {
		D_ptr_ = _D_ptr;     // Need to remember this for when update_decomp() is called!
		if(D_ptr)
			(*D_ptr) = rcp::null;  // No explicit D matrix
	}
	else {
		D_ptr_ = rcp::null;  // This matrix will be passed back in set_basis_matrices(...) before update_decomp(...) is called.
		if(D_ptr)
			(*D_ptr) = _D_ptr;  // Externalize explicit D matrix.
	}

	//
	// Determine if we need to allocate a new basis matrix object C.
	//
	// At this point, if a matrix object for C already exits and noone
	// outside has a reference to this basis matrix object then the only
	// references to it should be in C_ptr
	//

	bool new_C_mat_object; // compiler should warn if used before initialized!
	if( (*C_ptr).get() == NULL ) {
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nMust allocate a new basis matrix object for C since "
				 << "one has not been allocated yet ...\n";
		new_C_mat_object = true;
	}
	else {
		if( (*C_ptr).count() > 1 ) {
			if( out && olevel >= PRINT_BASIC_INFO )
				*out << "\nMust allocate a new basis matrix object C since someone "
					 << "else is using the current one ...\n";
			new_C_mat_object = true;
		}
		else {
			new_C_mat_object = false; // The current C matrix is owned only by us!
		}
	}
	
	//
	// Get the basis matrix object C and allocate a new object for if we have to.
	//

	if( new_C_mat_object) {
		(*C_ptr) = basis_sys_->factory_C()->create();
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nAllocated a new basis matrix object C "
				 << "of type \'" << typeid(*(*C_ptr)).name() << "\' ...\n";
	}


	if( out && olevel >= PRINT_BASIC_INFO )
		*out << "\nEnd DecompositionSystemVarReductImp::get_basis_matrices(...)\n";

}

void DecompositionSystemVarReductImp::set_basis_matrices(
	std::ostream                                               *out
	,EOutputLevel                                              olevel
	,ERunTests                                                 test_what
	,const MemMngPack::ref_count_ptr<MatrixOpNonsing>  &C_ptr
	,const MemMngPack::ref_count_ptr<MatrixOp>             &D_ptr
	,MatrixOp                                              *Uz
	,MatrixOp                                              *Vz
	,const basis_sys_ptr_t                                     &basis_sys
	)
{
	C_ptr_ = C_ptr;
	if( D_ptr.get() )
		D_ptr_ = D_ptr;
	if(basis_sys.get()) {
		basis_sys_   = basis_sys;
		space_range_ = space_x_->sub_space(basis_sys->var_dep())->clone();
		space_null_  = space_x_->sub_space(basis_sys->var_indep())->clone();
	}
}

// Overridden from DecompositionSystem

size_type DecompositionSystemVarReductImp::n() const
{
	if(space_x_.get()) {
		return space_x_->dim();
	}
	return 0; // Not fully initialized!
}

size_type DecompositionSystemVarReductImp::m() const
{
	if(space_c_.get()) {
		return space_c_->dim();
	}
	return 0; // Not fully initialized!
}

size_type DecompositionSystemVarReductImp::r() const
{
	if(basis_sys_.get()) {
		return basis_sys_->equ_decomp().size();
	}
	return 0; // Not fully initialized!
}

// range and null spaces

const VectorSpace::space_ptr_t
DecompositionSystemVarReductImp::space_range() const
{
	return space_range_;
}

const VectorSpace::space_ptr_t
DecompositionSystemVarReductImp::space_null() const
{
	return space_null_;
}

// matrix factories

const DecompositionSystem::mat_fcty_ptr_t
DecompositionSystemVarReductImp::factory_Z() const
{
	namespace rcp = MemMngPack;
	return rcp::rcp(
		new MemMngPack::AbstractFactoryStd<MatrixOp,MatrixIdentConcatStd>()
		);
}

const DecompositionSystem::mat_fcty_ptr_t
DecompositionSystemVarReductImp::factory_Uz() const
{
	return MemMngPack::rcp(	new MemMngPack::AbstractFactoryStd<MatrixOp,MatrixOpSubView>() );
}

const DecompositionSystem::mat_fcty_ptr_t
DecompositionSystemVarReductImp::factory_Vz() const
{
	return MemMngPack::rcp(	new MemMngPack::AbstractFactoryStd<MatrixOp,MatrixOpSubView>() );
}

void DecompositionSystemVarReductImp::update_decomp(
	std::ostream              *out
	,EOutputLevel             olevel
	,ERunTests                test_what
	,const MatrixOp       &Gc
	,const MatrixOp       *Gh
	,MatrixOp             *Z
	,MatrixOp             *Y
	,MatrixOpNonsing  *R
	,MatrixOp             *Uz
	,MatrixOp             *Uy
	,MatrixOp             *Vz
	,MatrixOp             *Vy
	,EMatRelations            mat_rel
	) const
{
	namespace rcp = MemMngPack;
	using DynamicCastHelperPack::dyn_cast;

	if( out && olevel >= PRINT_BASIC_INFO ) {
		*out << "\n***********************************************************"
			 << "\n*** DecompositionSystemVarReductImp::update_decomp(...) ***"
			 << "\n************************************************************\n";

		if(mat_rel != MATRICES_INDEP_IMPS)
			*out << "\nWarning!!! mat_rel != MATRICES_INDEP_IMPS; The decompsition matrix "
				 << "objects may not be independent of each other!\n"; 
	}

#ifdef _DEBUG
	// Validate setup
	THROW_EXCEPTION(
		basis_sys_.get() == NULL, std::logic_error
		,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
		"a BasisSystem object has not been set yet!" );
#endif
	
	const size_type
		n = this->n(),
		m = this->m(),
		r = this->r();
	const Range1D
		var_dep(1,r),
		var_indep(r+1,n),
		equ_decomp   = this->equ_decomp(),
		equ_undecomp = this->equ_undecomp();

#ifdef _DEBUG
	// Validate input
	THROW_EXCEPTION(
	    Z==NULL&&Y==NULL&&R==NULL&&Uz==NULL&&Uy==NULL&&Vz==NULL&&Vy==NULL
		, std::invalid_argument
		,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
		"at least one of Z, Y, R, Uz, Uy, Vz or Vy can not be NULL NULL!" );
	THROW_EXCEPTION(
		m == r && Uz != NULL, std::invalid_argument
		,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
		"Uz must be NULL if m==r is NULL!" );
	THROW_EXCEPTION(
		m == r && Uy != NULL, std::invalid_argument
		,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
		"Uy must be NULL if m==r is NULL!" );
	THROW_EXCEPTION(
	    Gh == NULL && Vz != NULL, std::invalid_argument
		,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
		"Vz must be NULL if Gh is NULL!" );
	THROW_EXCEPTION(
	    Gh == NULL && Vy != NULL, std::invalid_argument
		,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
		"Vy must be NULL if Gh is NULL!" );
#endif

	//
	// Get references to concrete matrix objects
	//

	MatrixIdentConcatStd
		*Z_vr = Z ? &dyn_cast<MatrixIdentConcatStd>(*Z) : NULL;
	assert(Uz == NULL); // ToDo: Implement undecomposed general equalities
	assert(Vz == NULL); // ToDo: Implement general inequalities

	//
	// Get smart pointers to unreferenced C and D matrix objects.
	//

	rcp::ref_count_ptr<MatrixOpNonsing>    C_ptr;
	rcp::ref_count_ptr<MatrixOp>               D_ptr;

	if( C_ptr_.get() ) {
		//
		// The function get_basis_matrices() was called by external client
		// so we don't need to call it here or update the decomposition matrices.
		//
		C_ptr = C_ptr_; // This was set by set_basis_matrices()
	}
	else {
		//
		// Make all matrices uninitialized and get unreferenced smart
		// pointers to C and D (explicit only!).
		//
		const_cast<DecompositionSystemVarReductImp*>(this)->get_basis_matrices(
			out,olevel,test_what,Z,Y,R,Uz,Uy,Vz,Vy,&C_ptr,&D_ptr);
	}

	// Get the D matrix created by get_basis_matrices() and set by
	// get_basis_matrices() if implicit or set by set_basis_matrices()
	// if explicit.  This matrix may not be allocated yet in which
	// case we need to allocate it.
	if( D_ptr.get() == NULL ) {
		// D_ptr was not set in get_basis_matrix() in code above but
		// it may be cashed (if explicit) in D_ptr.
		if( D_ptr_.get() != NULL )
			D_ptr = D_ptr_;
		else
			alloc_new_D_matrix( out, olevel, &D_ptr );
	}

	if( C_ptr_.get() == NULL ) {

		//
		// The basis matrices were not updated by an external client
		// so we must do it ourselves here using basis_sys.
		//
	
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nUpdating the basis matrix C and other matices using the BasisSystem object ...\n";
	
		assert( D_ptr.get() ); // local programming error only!
		assert( C_ptr.get() ); // local programming error only!
	
		basis_sys_->update_basis(
			&Gc                                                      // Gc
			,Gh                                                      // Gh?
			,C_ptr.get()                                             // C
			,D_imp_used_ == MAT_IMP_EXPLICIT ? D_ptr.get() : NULL     // D?
			,NULL                                                    // GcUP == Uz
			,NULL                                                    // GhUP == Vz
			,(mat_rel == mat_rel == MATRICES_INDEP_IMPS
			  ? BasisSystem::MATRICES_INDEP_IMPS
			  : BasisSystem::MATRICES_ALLOW_DEP_IMPS )
			,out && olevel >= PRINT_BASIC_INFO ? out : NULL
			);
	}
		
	//
	// Create the matrix object: N = Gc(var_indep,cond_decomp)' 
	//
	rcp::ref_count_ptr<const MatrixOp>
		N_ptr = rcp::null;
	if( D_imp_used_ == MAT_IMP_IMPLICIT ) {
		rcp::ref_count_ptr<const MatrixOp>
			GcDd_ptr = Gc.sub_view(var_indep,equ_decomp);
		THROW_EXCEPTION(
			GcDd_ptr.get() == NULL, std::logic_error
			,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
			"The matrix class used for the gradient of constraints matrix Gc of type \'"
			<< typeid(Gc).name() << "\' must return return.get() != NULL from "
			"Gc.sub_view(var_indep,equ_decomp)!" );
		if(mat_rel == MATRICES_INDEP_IMPS) {
			GcDd_ptr = GcDd_ptr->clone();
			THROW_EXCEPTION(
				GcDd_ptr.get() == NULL, std::logic_error
				,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
				"The matrix class used for the gradient of constraints matrix Gc.sub_view(var_indep,equ_decomp) "
				"of type \'" << typeid(*GcDd_ptr).name() << "\' must return return.get() != NULL from \n"
				"Gc.sub_view(var_indep,equ_decomp)->clone() since mat_rel == MATRICES_INDEP_IMPS!" );
		}
		N_ptr = rcp::rcp(
			new MatrixOpSubView(
				rcp::rcp_const_cast<MatrixOp>(GcDd_ptr)  // M_full
				,Range1D()                                   // rng_rows
				,Range1D()                                   // rng_cols
				,BLAS_Cpp::trans                             // M_trans
				)
			);
	}

	//
	// Test the basis matrix C and the other matrices.
	//
	
	if( test_what == RUN_TESTS ) {
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nTesting the basis matrix C and other matices updated using the BasisSystem object ...\n";
		THROW_EXCEPTION(
			basis_sys_tester_.get() == NULL, std::logic_error
			,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
			"test_what == RUN_TESTS but this->basis_sys_tester().get() == NULL!" );
		if( basis_sys_tester_->print_tests() == BasisSystemTester::PRINT_NOT_SELECTED ) {
			BasisSystemTester::EPrintTestLevel
				print_tests;
			switch(olevel) {
				case PRINT_NONE:
					print_tests = BasisSystemTester::PRINT_NONE;
					break;
				case PRINT_BASIC_INFO:
					print_tests = BasisSystemTester::PRINT_BASIC;
					break;
				case PRINT_MORE_INFO:
					print_tests = BasisSystemTester::PRINT_MORE;
					break;
				case PRINT_VECTORS:
				case PRINT_EVERY_THING:
					print_tests = BasisSystemTester::PRINT_ALL;
					break;
			}
			basis_sys_tester_->print_tests(print_tests);
			basis_sys_tester_->dump_all( olevel == PRINT_EVERY_THING );
		}
		const bool passed = basis_sys_tester_->test_basis_system(
			*basis_sys_                                              // basis_sys
			,&Gc                                                     // Gc
			,Gh                                                      // Gh?
			,C_ptr.get()                                             // C
			,N_ptr.get()                                             // N
			,D_imp_used_ == MAT_IMP_EXPLICIT ? D_ptr.get() : NULL     // D?
			,NULL                                                    // GcUP == Uz
			,NULL                                                    // GhUP == Vz
			,out                                                     // out
			);
		THROW_EXCEPTION(
			!passed, TestFailed
			,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
			"Test of the basis system failed!" );
	}

	//
	// Initialize the implicit D = -inv(C)*N matrix object.
	//

	assert(D_ptr.get()); // local programming error only?
	if( D_imp_used_ == MAT_IMP_IMPLICIT ) {
		if( !C_ptr.has_ownership() && mat_rel == MATRICES_INDEP_IMPS ) {
			C_ptr = C_ptr->clone_mwons();
			THROW_EXCEPTION(
				C_ptr.get() == NULL, std::logic_error
				,"DecompositionSystemVarReductImp::update_decomp(...) : Error, "
				"The matrix class used for the basis matrix C (from the BasisSystem object) "
				"must return return.get() != NULL from the clone_mwons() method since "
				"mat_rel == MATRICES_INDEP_IMPS!" );
		}
		dyn_cast<MatrixVarReductImplicit>(*D_ptr).initialize(
			C_ptr       // C
			,N_ptr      // N
			,rcp::null  // D_direct
			);
		// Above, if the basis matrix object C is not owned by the smart
		// reference counted pointer object C_ptr, then we need to make the
		// reference that the implicit D = -inv(C)*N matrix object has to
		// C independent and the clone() method does that very nicely.
	}

	//
	// Call on the subclass to construct Y, R, Uy and Vy given
	// C and D.
	//

	initialize_matrices(out,olevel,C_ptr,D_ptr,Y,R,Uy,Vy,mat_rel);

	//
	// Reconstruct Z, Uz and Uy.
	//

	if( Z_vr ) {
		Z_vr->initialize(
			space_x_                                   // space_cols
			,space_x_->sub_space(var_indep)->clone()   // space_rows
			,MatrixIdentConcatStd::TOP                 // top_or_bottom
			,1.0                                       // alpha
			,D_ptr                                     // D_ptr
			,BLAS_Cpp::no_trans                        // D_trans
			);
	}

	assert(Uz == NULL); // ToDo: Implement for undecomposed general equalities
	assert(Vz == NULL); // ToDo: Implement for general inequalities

	// Clear cache for basis matrices.
	C_ptr_ = rcp::null;
	D_ptr_ = rcp::null;

	if( out && olevel >= PRINT_BASIC_INFO )
		*out << "\nEnd DecompositionSystemVarReductImp::update_decomp(...)\n";
}

void DecompositionSystemVarReductImp::print_update_decomp(
	std::ostream& out, const std::string& L
	) const
{
	out
		<< L << "*** Variable reduction decomposition (class DecompositionSytemVarReductImp)\n"
		<< L << "C = Gc(var_dep,equ_decomp)' (using basis_sys)\n"
		<< L << "if C is nearly singular then throw SingularDecomposition exception\n"
		<< L << "if D_imp == MAT_IMP_IMPICIT then\n"
		<< L << "  D = -inv(C)*N represented implicitly (class MatrixVarReductImplicit)\n"
		<< L << "else\n"
		<< L << "  D = -inv(C)*N computed explicity (using basis_sys)\n"
		<< L << "end\n"
		<< L << "Z = [ D; I ] (class MatrixIdentConcatStd)\n"
		<< L << "Uz = Gc(var_indep,equ_undecomp)' - Gc(var_dep,equ_undecomp)'*D\n"
		<< L << "Vz = Gh(var_indep,:)' - Gh(var_dep,:)'*D\n"
		<< L << "begin update Y, R, Uy, and Vy\n"
		;
	print_update_matrices( out, L + "  " );
	out
		<< L << "end update of Y, R, Uy, and Vy\n"
		;
}

// Overridden from DecompositionSystemVarReduct

Range1D DecompositionSystemVarReductImp::var_indep() const
{
	return basis_sys_.get() ? basis_sys_->var_indep() : Range1D::Invalid;
}

Range1D DecompositionSystemVarReductImp::var_dep() const
{
	return basis_sys_.get() ? basis_sys_->var_dep() : Range1D::Invalid;
}

// protected

void DecompositionSystemVarReductImp::update_D_imp_used(EExplicitImplicit *D_imp_used) const
{
	*D_imp_used = ( D_imp() == MAT_IMP_AUTO
				   ? MAT_IMP_IMPLICIT     // Without better info, use implicit by default!
				   : D_imp() );
}

// private

void DecompositionSystemVarReductImp::alloc_new_D_matrix( 
	std::ostream                             *out
	,EOutputLevel                            olevel
	,MemMngPack::ref_count_ptr<MatrixOp> *D_ptr
	) const
{
	if(D_imp_used_ == MAT_IMP_IMPLICIT) {
		(*D_ptr) = MemMngPack::rcp(new MatrixVarReductImplicit());
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nAllocated a new implicit matrix object for D = -inv(C)*N "
				 << "of type \'MatrixVarReductImplicit\' ...\n";
	}
	else {
		(*D_ptr) = basis_sys_->factory_D()->create();
		if( out && olevel >= PRINT_BASIC_INFO )
			*out << "\nAllocated a new explicit matrix object for D = -inv(C)*N "
				 << "of type \'" << typeid(*(*D_ptr)).name() << "\' ...\n";
	}
}

}	// end namespace ConstrainedOptimizationPack
