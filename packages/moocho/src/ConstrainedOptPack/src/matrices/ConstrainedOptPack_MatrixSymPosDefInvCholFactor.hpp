// //////////////////////////////////////////////////////////////////////////////////
// MatrixSymPosDefInvCholFactor.hpp
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

#ifndef MATRIX_SYM_POS_DEF_INV_CHOL_FACTOR_H
#define MATRIX_SYM_POS_DEF_INV_CHOL_FACTOR_H

#include "SymInvCholMatrixClass.hpp"
#include "MatrixSymSecantUpdateable.hpp"
#include "MatrixExtractInvCholFactor.hpp"
#include "SparseLinAlgPack/src/MatrixWithOpConcreteEncap.hpp"
#include "SparseLinAlgPack/src/MatrixSymWithOpFactorized.hpp"

namespace ConstrainedOptimizationPack {

///
/** Implementation of MatrixWithOp abstract interface for SymInvCholMatrix
  */
class MatrixSymPosDefInvCholFactor
	: public virtual MatrixWithOpConcreteEncap<SymInvCholMatrix>
	, public virtual MatrixSymWithOpFactorized
	, public MatrixSymSecantUpdateable
	, public MatrixExtractInvCholFactor
{
public:

	///
	MatrixSymPosDefInvCholFactor()
	{}

	///
	MatrixSymPosDefInvCholFactor(const SymInvCholMatrix& m)
		: MatrixWithOpConcreteEncap<SymInvCholMatrix>(m)
	{}

	// /////////////////////////////////////////////////////////
	/** @name Overridden from Matrix */
	//@{

	/// 
	size_type cols() const;

	//@}

	// /////////////////////////////////////////////////////////
	/** @name Overridden from MatrixWithOp */
	//@{

	///
	MatrixWithOp& operator=(const MatrixWithOp& m);
	///
	std::ostream& output(std::ostream& out) const;
	///
	void Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha, BLAS_Cpp::Transp trans_rhs1
		, const DVectorSlice& vs_rhs2, value_type beta) const;
	///
	void Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha, BLAS_Cpp::Transp trans_rhs1
		, const SpVectorSlice& sv_rhs2, value_type beta) const;
	///
	value_type transVtMtV(const DVectorSlice& vs_rhs1, BLAS_Cpp::Transp trans_rhs2
		, const DVectorSlice& vs_rhs3) const;
	///
	value_type transVtMtV(const SpVectorSlice& sv_rhs1, BLAS_Cpp::Transp trans_rhs2
		, const SpVectorSlice& sv_rhs3) const;

	//@}

	// ////////////////////////////////////////////////////////////
	/** @name Overridden from MatrixFactorized */
	//@{

	///
	void V_InvMtV(DVector* v_lhs, BLAS_Cpp::Transp trans_rhs1
		, const DVectorSlice& vs_rhs2) const;
	///
	void V_InvMtV(DVectorSlice* vs_lhs, BLAS_Cpp::Transp trans_rhs1
		, const DVectorSlice& vs_rhs2) const;
	///
	void V_InvMtV(DVector* v_lhs, BLAS_Cpp::Transp trans_rhs1
		, const SpVectorSlice& sv_rhs2) const;
	///
	void V_InvMtV(DVectorSlice* vs_lhs, BLAS_Cpp::Transp trans_rhs1
		, const SpVectorSlice& sv_rhs2) const;
	///
	value_type transVtInvMtV(const DVectorSlice& vs_rhs1
		, BLAS_Cpp::Transp trans_rhs2, const DVectorSlice& vs_rhs3) const;
	///
	value_type transVtInvMtV(const SpVectorSlice& sv_rhs1
		, BLAS_Cpp::Transp trans_rhs2, const SpVectorSlice& sv_rhs3) const;

	//@}

	// ////////////////////////////////////////////////////////////
	/** @name Overridden from MatrixSymFactorized */
	//@{

	///
	void M_StMtInvMtM( DMatrixSliceSym* sym_gms_lhs, value_type alpha
		, const MatrixWithOp& mwo, BLAS_Cpp::Transp mwo_trans, EMatrixDummyArg
		) const;

	//@}

	// ///////////////////////////////////////////////////////////
	/** @name Overridden from MatrixSymSecantUpdateable */
	//@{

	///
	void init_identity( size_type n, value_type alpha );
	///
	void init_diagonal( const DVectorSlice& diag );
	///
	void secant_update(DVectorSlice* s, DVectorSlice* y, DVectorSlice* Bs);

	//@}

	// ///////////////////////////////////////////////////////////
	/** @name Overridden from MatrixExtractInvCholFactor */
	//@{

	///
	void extract_inv_chol( DMatrixSliceTriEle* InvChol ) const;

	//@}

};	// end class MatrixSymPosDefInvCholFactor

}	// end namespace ConstrainedOptimizationPack 

#endif	// MATRIX_SYM_POS_DEF_INV_CHOL_FACTOR_H
