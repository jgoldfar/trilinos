// ///////////////////////////////////////////////////////////
// NLPFirstDerivativesTester.cpp
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

#include <assert.h>
#include <math.h>

#include <iomanip>
#include <sstream>

#include "NLPFirstDerivativesTester.h"
#include "NLPInterfacePack/include/NLPFirstOrderInfo.h"
#include "AbstractLinAlgPack/include/MatrixWithOp.h"
#include "AbstractLinAlgPack/include/VectorWithOpMutable.h"
#include "AbstractLinAlgPack/include/VectorWithOpOut.h"
#include "AbstractLinAlgPack/include/VectorSpace.h"
#include "AbstractLinAlgPack/include/VectorStdOps.h"
#include "AbstractLinAlgPack/include/LinAlgOpPack.h"
#include "AbstractLinAlgPack/include/assert_print_nan_inf.h"
#include "AbstractLinAlgPack/include/EtaVector.h"
#include "Range1D.h"
#include "update_success.h"
#include "ThrowException.h"

namespace NLPInterfacePack {

NLPFirstDerivativesTester::NLPFirstDerivativesTester(
	const calc_fd_prod_ptr_t  &calc_fd_prod
	,ETestingMethod           fd_testing_method
	,size_type                num_fd_directions
	,value_type               warning_tol
	,value_type               error_tol
	)
	:calc_fd_prod_(calc_fd_prod)
	,fd_testing_method_(fd_testing_method)
	,num_fd_directions_(num_fd_directions)
	,warning_tol_(warning_tol)
	,error_tol_(error_tol)
{}

bool NLPFirstDerivativesTester::finite_diff_check(
	NLP                     *nlp
	,const VectorWithOp     &xo
	,const VectorWithOp     *xl
	,const VectorWithOp     *xu
	,const MatrixWithOp     *Gc
	,const MatrixWithOp     *Gh
	,const VectorWithOp     *Gf
	,bool                   print_all_warnings
	,std::ostream           *out
	) const
{
	namespace rcp = MemMngPack;
	using AbstractLinAlgPack::assert_print_nan_inf;

	const size_type
		n  = nlp->n(),
		m  = nlp->m(),
		mI = nlp->mI();

	// ///////////////////////////////////
	// Validate the input

	THROW_EXCEPTION(
		!m && Gc, std::invalid_argument
		,"NLPFirstDerivativesTester::finite_diff_check(...) : "
		"Error, Gc must be NULL if m == 0" );
	THROW_EXCEPTION(
		!mI && Gh, std::invalid_argument
		,"NLPFirstDerivativesTester::finite_diff_check(...) : "
		"Error, Gh must be NULL if mI == 0" );

	assert_print_nan_inf(xo, "xo",true,out);
	if(Gf)
		assert_print_nan_inf(*Gf, "Gf",true,out); 

	bool success = true;

	try {

	switch(fd_testing_method_) {
		case FD_COMPUTE_ALL:
			return fd_check_all(nlp,xo,xl,xu,Gc,Gh,Gf
				,print_all_warnings,out);
		case FD_DIRECTIONAL:
			return fd_directional_check(nlp,xo,xl,xu,Gc,Gh,Gf
				,print_all_warnings,out);
		default:
			assert(0);
	}

	} // end try
	catch( const AbstractLinAlgPack::NaNInfException& except ) {
		if(out)
			*out
				<< "Error: found a NaN or Inf.  Stoping tests!\n";
		success = false;
	}
	
	return success;	// will never be executed
}

// private

bool NLPFirstDerivativesTester::fd_check_all(
	NLP                     *nlp
	,const VectorWithOp     &xo
	,const VectorWithOp     *xl
	,const VectorWithOp     *xu
	,const MatrixWithOp     *Gc
	,const MatrixWithOp     *Gh
	,const VectorWithOp     *Gf
	,bool                   print_all_warnings
	,std::ostream           *out
	) const
{
	using std::setw;
	using std::endl;
	using std::right;

	namespace rcp = MemMngPack;
	using AbstractLinAlgPack::sum;
	using AbstractLinAlgPack::dot;
	using AbstractLinAlgPack::Vp_StV;
	using AbstractLinAlgPack::assert_print_nan_inf;
	using AbstractLinAlgPack::random_vector;
	using LinAlgOpPack::V_StV;
	using LinAlgOpPack::V_MtV;

 	using TestingHelperPack::update_success;

	bool success = true;

	const size_type
		n  = nlp->n(),
		m  = nlp->m(),
		mI = nlp->mI();

	// //////////////////////////////////////////////
	// Validate the input

	NLP::vec_space_ptr_t
		space_x = nlp->space_x(),
		space_c = nlp->space_c(),
		space_h = nlp->space_h();

	const CalcFiniteDiffProd
		&fd_deriv_prod = this->calc_fd_prod();

	const value_type
		rand_y_l = -1.0, rand_y_u = 1.0,
		small_num = ::pow(std::numeric_limits<value_type>::epsilon(),0.25);

	if(out)
		*out
			<< "\nComparing Gf, Gc and/or Gh with finite-difference values "
				"FDGf, FDGc and/or FDGh one variable i at a time ...\n";

	value_type  max_Gf_warning_viol = 0.0;
	int         num_Gf_warning_viol = 0;
	value_type  max_Gc_warning_viol = 0.0;
	int         num_Gc_warning_viol = 0;

	VectorSpace::vec_mut_ptr_t
		e_i       = space_x->create_member(),
		Gc_i      = ( Gc ? space_c->create_member()  : rcp::null ),
		FDGc_i    = ( Gc ? space_c->create_member()  : rcp::null ),
		Gh_i      = ( Gh ? space_c->create_member()  : rcp::null ),
		FDGh_i    = ( Gh ? space_c->create_member()  : rcp::null );

	*e_i = 0.0;

	for( size_type i = 1; i <= n; ++ i ) {
		EtaVector eta_i(i,n);
		e_i->set_ele(i,1.0);
		// Compute exact??? values
		value_type
			Gf_i = Gf ? Gf->get_ele(i) : 0.0;
		if(Gc)
			V_MtV( Gc_i.get(), *Gc, BLAS_Cpp::trans, eta_i() );
		if(Gh)
			V_MtV( Gh_i.get(), *Gh, BLAS_Cpp::trans, eta_i() );
		// Compute finite difference values
		value_type
			FDGf_i;
		const bool preformed_fd = fd_deriv_prod.calc_deriv_product(
			xo,xl,xu
			,*e_i
			,NULL // fo
			,NULL // co
			,NULL // ho
			,true // check_nan_inf
			,nlp
			,Gf ? &FDGf_i : NULL
			,Gc ? FDGc_i.get() : NULL
			,Gh ? FDGc_i.get() : NULL
			,out
			);
		if( !preformed_fd ) {
			if(out)
				*out
					<< "\nError, the finite difference computation was not preformed due to cramped bounds\n"
					<< "Finite difference test failed!\n" << endl;
			return false;
		}
		
		// Compare the quantities
		// Gf
		assert_print_nan_inf(FDGf_i, "FDGf'*e_i",true,out);
		const value_type
			Gf_err = ::fabs( Gf_i - FDGf_i ) / ( ::fabs(Gf_i) + ::fabs(FDGf_i) + small_num );
		if(out)
			*out
				<< "\nrel_err(Gf("<<i<<"),FDGf'*e("<<i<<")) = "
				<< "rel_err(" << Gf_i << "," << FDGf_i << ") = "
				<< Gf_err << endl;
		if( Gf_err >= warning_tol() ) {
			max_Gf_warning_viol = std::_MAX( max_Gf_warning_viol, Gf_err );
			++num_Gf_warning_viol;
		}
		if( Gf_err >= error_tol() ) {
			if(out)
				*out
					<< "\nError, exceeded Gf_error_tol = " << error_tol() << endl
					<< "Stoping the tests!\n";
			return false;
		}
		// Gc
		if(Gc) {
			const value_type
				sum_Gc_i   = sum(*Gc_i),
				sum_FDGc_i = sum(*FDGc_i);
			assert_print_nan_inf(sum_FDGc_i, "sum(FDGc'*e_i)",true,out);
			const value_type
				calc_err = ::fabs( ( sum_Gc_i - sum_FDGc_i )
								   /( ::fabs(sum_Gc_i) + ::fabs(sum_FDGc_i) + small_num ) );
			if(out)
				*out
					<< "\nrel_err(sum(Gc'*e("<<i<<")),sum(FDGc'*e("<<i<<"))) = "
					<< "rel_err(" << sum_Gc_i << "," << sum_FDGc_i << ") = "
					<< calc_err << endl;
			if( calc_err >= warning_tol() ) {
				max_Gc_warning_viol = std::_MAX( max_Gc_warning_viol, calc_err );
				++num_Gc_warning_viol;
			}
			if( calc_err >= error_tol() ) {
				if(out)
					*out
						<< "\nError, rel_err(sum(Gc'*e("<<i<<")),sum(FDGc'*e("<<i<<"))) = "
						<< "rel_err(" << sum_Gc_i << "," << sum_FDGc_i << ") = "
						<< calc_err << endl
						<< "exceeded error_tol = " << error_tol() << endl
						<< "Stoping the tests!\n";
				if(print_all_warnings)
					*out << "\ne_i =\n"     << *e_i
						 << "\nGc_i =\n"    << *Gc_i
						 << "\nFDGc_i =\n"  << *FDGc_i;
				update_success( false, &success );
				return false;
			}
			if( calc_err >= warning_tol() ) {
				if(out)
					*out
						<< "\nWarning, rel_err(sum(Gc'*e("<<i<<")),sum(FDGc'*e("<<i<<"))) = "
						<< "rel_err(" << sum_Gc_i << "," << sum_FDGc_i << ") = "
						<< calc_err << endl
						<< "exceeded warning_tol = " << warning_tol() << endl;
			}
		}
		// Gh
		if(Gh) {
			assert(0); // ToDo: Implement for general inequalities!
		}
		e_i->set_ele(i,0.0);
	}
	if(out && num_Gf_warning_viol)
		*out
			<< "\nFor Gf, there were " << num_Gf_warning_viol << " warning tolerance "
			<< "violations out of num_fd_directions = " << num_fd_directions()
			<< " computations of FDGf'*e(i)\n"
			<< "and the maximum violation was " << max_Gf_warning_viol
			<< " > Gf_waring_tol = " << warning_tol() << endl;
	if(out && num_Gc_warning_viol)
		*out
			<< "\nFor Gc, there were " << num_Gc_warning_viol << " warning tolerance "
			<< "violations out of num_fd_directions = " << num_fd_directions()
			<< " computations of FDGc'*e(i)\n"
			<< "and the maximum violation was " << max_Gc_warning_viol
			<< " > Gc_waring_tol = " << warning_tol() << endl;
	if(out)
		*out
			<< "\nCongradulations!  All of the computed errors were within the specified error tolerance!\n";

	return true;

}

bool NLPFirstDerivativesTester::fd_directional_check(
	NLP                     *nlp
	,const VectorWithOp     &xo
	,const VectorWithOp     *xl
	,const VectorWithOp     *xu
	,const MatrixWithOp     *Gc
	,const MatrixWithOp     *Gh
	,const VectorWithOp     *Gf
	,bool                   print_all_warnings
	,std::ostream           *out
	) const
{
	using std::setw;
	using std::endl;
	using std::right;

	namespace rcp = MemMngPack;
	using AbstractLinAlgPack::sum;
	using AbstractLinAlgPack::dot;
	using AbstractLinAlgPack::Vp_StV;
	using AbstractLinAlgPack::assert_print_nan_inf;
	using AbstractLinAlgPack::random_vector;
	using LinAlgOpPack::V_StV;
	using LinAlgOpPack::V_MtV;

 	using TestingHelperPack::update_success;

	bool success = true;

	const size_type
		n  = nlp->n(),
		m  = nlp->m(),
		mI = nlp->mI();

	// //////////////////////////////////////////////
	// Validate the input

	NLP::vec_space_ptr_t
		space_x = nlp->space_x(),
		space_c = nlp->space_c(),
		space_h = nlp->space_h();

	const CalcFiniteDiffProd
		&fd_deriv_prod = this->calc_fd_prod();

	const value_type
		rand_y_l = -1.0, rand_y_u = 1.0,
		small_num = ::pow(std::numeric_limits<value_type>::epsilon(),0.25);

	if(out)
		*out
			<< "\nComparing products Gf'*y Gc'*y and/or Gh'*y with finite difference values "
				" FDGf'*y, FDGc'*y and/or FDGh'*y for random y's ...\n";

	value_type  max_Gf_warning_viol = 0.0;
	int         num_Gf_warning_viol = 0;

	VectorSpace::vec_mut_ptr_t
		y         = space_x->create_member(),
		Gc_prod   = ( Gc ? space_c->create_member()  : rcp::null ),
		FDGc_prod = ( Gc ? space_c->create_member()  : rcp::null ),
		Gh_prod   = ( Gh ? space_c->create_member()  : rcp::null ),
		FDGh_prod = ( Gh ? space_c->create_member()  : rcp::null );

	for( int direc_i = 1; direc_i <= num_fd_directions(); ++direc_i ) {
		random_vector( rand_y_l, rand_y_u, y.get() );
		if(out)
			*out
				<< "\n****"
				<< "\n**** Random directional vector " << direc_i << " ( ||y||_1 / n = "
				<< (y->norm_1() / y->dim()) << " )"
				<< "\n***\n";
		// Compute exact??? values
		value_type
			Gf_y = Gf ? dot( *Gf, *y ) : 0.0;
		if(Gc)
			V_MtV( Gc_prod.get(), *Gc, BLAS_Cpp::trans, *y );
		if(Gh)
			V_MtV( Gh_prod.get(), *Gh, BLAS_Cpp::trans, *y );
		// Compute finite difference values
		value_type
			FDGf_y;
		const bool preformed_fd = fd_deriv_prod.calc_deriv_product(
			xo,xl,xu
			,*y
			,NULL // fo
			,NULL // co
			,NULL // ho
			,true // check_nan_inf
			,nlp
			,Gf ? &FDGf_y : NULL
			,Gc ? FDGc_prod.get() : NULL
			,Gh ? FDGc_prod.get() : NULL
			,out
			);
		if( !preformed_fd ) {
			if(out)
				*out
					<< "\nError, the finite difference computation was not preformed due to cramped bounds\n"
					<< "Finite difference test failed!\n" << endl;
			return false;
		}
		
		// Compare the quantities
		// Gf
		assert_print_nan_inf(FDGf_y, "FDGf'*y",true,out);
		const value_type
			Gf_err = ::fabs( Gf_y - FDGf_y ) / ( ::fabs(Gf_y) + ::fabs(FDGf_y) + small_num );
		if(out)
			*out
				<< "\nrel_err(Gf'*y,FDGf'*y) = "
				<< "rel_err(" << Gf_y << "," << FDGf_y << ") = "
				<< Gf_err << endl;
		if( Gf_err >= warning_tol() ) {
			max_Gf_warning_viol = std::_MAX( max_Gf_warning_viol, Gf_err );
			++num_Gf_warning_viol;
		}
		if( Gf_err >= error_tol() ) {
			if(out)
				*out
					<< "\nError, exceeded Gf_error_tol = " << error_tol() << endl
					<< "Stoping the tests!\n";
			return false;
		}
		// Gc
		if(Gc) {
			const value_type
				sum_Gc_prod   = sum(*Gc_prod),
				sum_FDGc_prod = sum(*FDGc_prod);
			assert_print_nan_inf(sum_FDGc_prod, "sum(FDGc'*y)",true,out);
			const value_type
				calc_err = ::fabs( ( sum_Gc_prod - sum_FDGc_prod )
								   /( ::fabs(sum_Gc_prod) + ::fabs(sum_FDGc_prod) + small_num ) );
			if(out)
				*out
					<< "\nrel_err(sum(Gc'*y),sum(FDGc'*y)) = "
					<< "rel_err(" << sum_Gc_prod << "," << sum_FDGc_prod << ") = "
					<< calc_err << endl;
			if( calc_err >= error_tol() ) {
				if(out)
					*out
						<< "\nError, rel_err(sum(Gc'*y),sum(FDGc'*y)) = "
						<< "rel_err(" << sum_Gc_prod << "," << sum_FDGc_prod << ") = "
						<< calc_err << endl
						<< "exceeded error_tol = " << error_tol() << endl
						<< "Stoping the tests!\n";
				if(print_all_warnings)
					*out << "\ny =\n"          << *y
						 << "\nGc_prod =\n"    << *Gc_prod
						 << "\nFDGc_prod =\n"  << *FDGc_prod;
				update_success( false, &success );
				return false;
			}
			if( calc_err >= warning_tol() ) {
				if(out)
					*out
						<< "\nWarning, rel_err(sum(Gc'*y),sum(FDGc'*y)) = "
						<< "rel_err(" << sum_Gc_prod << "," << sum_FDGc_prod << ") = "
						<< calc_err << endl
						<< "exceeded warning_tol = " << warning_tol() << endl;
			}
		}
		// Gh
		if(Gh) {
			assert(0); // ToDo: Implement for general inequalities!
		}
	}
	if(out && num_Gf_warning_viol)
		*out
			<< "\nFor Gf, there were " << num_Gf_warning_viol << " warning tolerance "
			<< "violations out of num_fd_directions = " << num_fd_directions()
			<< " computations of FDGf'*y\n"
			<< "and the maximum violation was " << max_Gf_warning_viol
			<< " > Gf_waring_tol = " << warning_tol() << endl;

	if(out)
		*out
			<< "\nCongradulations!  All of the computed errors were within the specified error tolerance!\n";

	return true;
}

}	// end namespace NLPInterfacePack
