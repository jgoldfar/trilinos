// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#include "FEApp_Application.hpp"
#include "FEApp_ProblemFactory.hpp"
#include "FEApp_QuadratureFactory.hpp"
#include "FEApp_DiscretizationFactory.hpp"
#include "FEApp_InitPostOps.hpp"
#include "FEApp_GlobalFill.hpp"

FEApp::Application::Application(
		   const std::vector<double>& coords,
		   const Teuchos::RCP<const Epetra_Comm>& comm,
		   const Teuchos::RCP<Teuchos::ParameterList>& params,
		   bool is_transient) :
  transient(is_transient)
{
  // Create parameter library
  paramLib = Teuchos::rcp(new Sacado::ScalarParameterLibrary);

  // Create problem object
  Teuchos::RCP<Teuchos::ParameterList> problemParams = 
    Teuchos::rcp(&(params->sublist("Problem")),false);
  FEApp::ProblemFactory problemFactory(problemParams, paramLib);
  Teuchos::RCP<FEApp::AbstractProblem> problem = 
    problemFactory.create();

  // Get number of equations
  unsigned int num_equations = problem->numEquations();

  // Create quadrature object
  Teuchos::RCP<Teuchos::ParameterList> quadParams = 
    Teuchos::rcp(&(params->sublist("Quadrature")),false);
  FEApp::QuadratureFactory quadFactory(quadParams);
  quad = quadFactory.create();

  // Create discretization object
  Teuchos::RCP<Teuchos::ParameterList> discParams = 
    Teuchos::rcp(&(params->sublist("Discretization")),false);
  FEApp::DiscretizationFactory discFactory(discParams);
  disc = discFactory.create(coords, num_equations, comm);
  disc->createMesh();
  disc->createMaps();
  disc->createJacobianGraphs();

  // Create Epetra objects
  importer = Teuchos::rcp(new Epetra_Import(*(disc->getOverlapMap()), 
					    *(disc->getMap())));
  exporter = Teuchos::rcp(new Epetra_Export(*(disc->getOverlapMap()), 
					    *(disc->getMap())));
  overlapped_x = Teuchos::rcp(new Epetra_Vector(*(disc->getOverlapMap())));
  if (transient)
    overlapped_xdot = 
      Teuchos::rcp(new Epetra_Vector(*(disc->getOverlapMap())));
  overlapped_f = Teuchos::rcp(new Epetra_Vector(*(disc->getOverlapMap())));
  overlapped_jac = 
    Teuchos::rcp(new Epetra_CrsMatrix(Copy, 
				      *(disc->getOverlapJacobianGraph())));

  // Initialize problem
  initial_x = Teuchos::rcp(new Epetra_Vector(*(disc->getMap())));
  problem->buildProblem(*(disc->getMap()), *(disc->getOverlapMap()), 
			pdeTM, bc, initial_x);
  typedef FEApp::AbstractPDE_TemplateManager<ValidTypes>::iterator iterator;
  int nqp = quad->numPoints();
  int nn = disc->getNumNodesPerElement();
  for (iterator it = pdeTM.begin(); it != pdeTM.end(); ++it)
    it->init(nqp, nn);

#if SG_ACTIVE
  enable_sg = params->get("Enable Stochastic Galerkin",false);
  if (enable_sg) {
    sg_basis = params->get< Teuchos::RCP<const Stokhos::OrthogPolyBasis<double> > >("Stochastic Galerkin basis");
    Cijk = params->get< Teuchos::RCP<const Stokhos::TripleProduct< Stokhos::OrthogPolyBasis<double> > > >("Stochastic Galerkin triple product");

    
    sg_disc = Teuchos::rcp(new FEApp::BlockDiscretization(comm, disc,
							  sg_basis));

    // Create Epetra objects
    sg_initial_x = 
      Teuchos::rcp(new EpetraExt::BlockVector(*(disc->getMap()),
					      *(sg_disc->getMap())));
    sg_importer = Teuchos::rcp(new Epetra_Import(*(sg_disc->getOverlapMap()), 
						 *(sg_disc->getMap())));
    sg_exporter = Teuchos::rcp(new Epetra_Export(*(sg_disc->getOverlapMap()), 
						 *(sg_disc->getMap())));
    sg_overlapped_x = 
      Teuchos::rcp(new EpetraExt::BlockVector(*(disc->getOverlapMap()),
					      *(sg_disc->getOverlapMap())));
    if (transient)
      sg_overlapped_xdot = 
	Teuchos::rcp(new EpetraExt::BlockVector(*(disc->getOverlapMap()),
						*(sg_disc->getOverlapMap())));
    sg_overlapped_f = 
      Teuchos::rcp(new EpetraExt::BlockVector(*(disc->getOverlapMap()),
					      *(sg_disc->getOverlapMap())));
    sg_overlapped_jac = sg_disc->getOverlapJacobian();
  }
#endif
}

FEApp::Application::~Application()
{
}

Teuchos::RCP<const Epetra_Map>
FEApp::Application::getMap() const
{
#if SG_ACTIVE
  if (enable_sg)
    return sg_disc->getMap();
  else
#endif
    return disc->getMap();
}

Teuchos::RCP<const Epetra_CrsGraph>
FEApp::Application::getJacobianGraph() const
{
#if SG_ACTIVE
  if (enable_sg)
    return sg_disc->getJacobianGraph();
  else
#endif
    return disc->getJacobianGraph();
}

Teuchos::RCP<const Epetra_Vector>
FEApp::Application::getInitialSolution() const
{
#if SG_ACTIVE
  if (enable_sg)
    return sg_initial_x;
  else
#endif
    return initial_x;
}

Teuchos::RCP<Sacado::ScalarParameterLibrary> 
FEApp::Application::getParamLib()
{
  return paramLib;
}

bool
FEApp::Application::isTransient() const
{
  return transient;
}

void
FEApp::Application::computeGlobalResidual(
				    const Epetra_Vector* xdot,
				    const Epetra_Vector& x,
				    const Sacado::ScalarParameterVector* p,
				    Epetra_Vector& f)
{
  if (enable_sg) {
    computeGlobalSGResidual(xdot, x, p, f);
    return;
  }

  // Scatter x to the overlapped distrbution
  overlapped_x->Import(x, *importer, Insert);

  // Scatter xdot to the overlapped distribution
  if (transient)
    overlapped_xdot->Import(*xdot, *importer, Insert);

  // Set parameters
  if (p != NULL) {
    for (unsigned int i=0; i<p->size(); ++i) {
      (*p)[i].family->setRealValueForAllTypes((*p)[i].baseValue);
    }
  }

  // Zero out overlapped residual
  overlapped_f->PutScalar(0.0);
  f.PutScalar(0.0);

  // Create residual init/post op
  Teuchos::RCP<FEApp::ResidualOp> op;
  op = Teuchos::rcp(new FEApp::ResidualOp(overlapped_xdot, overlapped_x, 
					  overlapped_f));

  // Get template PDE instantiation
  Teuchos::RCP< FEApp::AbstractPDE<ResidualOp::fill_type> > pde = 
    pdeTM.getAsObject<ResidualOp::fill_type>();

  // Do global fill
  FEApp::GlobalFill<ResidualOp::fill_type> globalFill(disc->getMesh(), quad, 
						      pde, bc, transient);
  globalFill.computeGlobalFill(*op);

  // Assemble global residual
  f.Export(*overlapped_f, *exporter, Add);
}

void
FEApp::Application::computeGlobalJacobian(
				      double alpha, double beta,
				      const Epetra_Vector* xdot,
				      const Epetra_Vector& x,
				      const Sacado::ScalarParameterVector* p,
				      Epetra_Vector* f,
				      Epetra_CrsMatrix& jac)
{
  if (enable_sg) {
    computeGlobalSGJacobian(alpha, beta, xdot, x, p, f, jac);
    return;
  }

  // Scatter x to the overlapped distrbution
  overlapped_x->Import(x, *importer, Insert);

  // Scatter xdot to the overlapped distribution
  if (transient)
    overlapped_xdot->Import(*xdot, *importer, Insert);

  // Set parameters
  if (p != NULL) {
    for (unsigned int i=0; i<p->size(); ++i) {
      (*p)[i].family->setRealValueForAllTypes((*p)[i].baseValue);
    }
  }

  // Zero out overlapped residual
  Teuchos::RCP<Epetra_Vector> overlapped_ff;
  if (f != NULL) {
    overlapped_ff = overlapped_f;
    overlapped_ff->PutScalar(0.0);
    f->PutScalar(0.0);
  }

  // Zero out Jacobian
  overlapped_jac->PutScalar(0.0);
  jac.PutScalar(0.0);

  // Create Jacobian init/post op
  Teuchos::RCP<FEApp::JacobianOp> op;
  op = Teuchos::rcp(new FEApp::JacobianOp(alpha, beta, overlapped_xdot, 
					  overlapped_x, overlapped_ff, 
					  overlapped_jac));

  // Get template PDE instantiation
  Teuchos::RCP< FEApp::AbstractPDE<JacobianOp::fill_type> > pde = 
    pdeTM.getAsObject<JacobianOp::fill_type>();

  // Do global fill
  FEApp::GlobalFill<JacobianOp::fill_type> globalFill(disc->getMesh(), 
						      quad, pde, bc, 
						      transient);
  globalFill.computeGlobalFill(*op);

  // Assemble global residual
  if (f != NULL)
    f->Export(*overlapped_f, *exporter, Add);

  // Assemble global Jacobian
  jac.Export(*overlapped_jac, *exporter, Add);

  jac.FillComplete(true);
}

void
FEApp::Application::computeGlobalTangent(
			      double alpha, double beta,
			      bool sum_derivs,
			      const Epetra_Vector* xdot,
			      const Epetra_Vector& x,
			      Sacado::ScalarParameterVector* p,
			      const Epetra_MultiVector* Vx,
			      const Teuchos::SerialDenseMatrix<int,double>* Vp,
			      Epetra_Vector* f,
			      Epetra_MultiVector* JVx,
			      Epetra_MultiVector* fVp)
{
  // Scatter x to the overlapped distrbution
  overlapped_x->Import(x, *importer, Insert);

  // Scatter xdot to the overlapped distribution
  if (transient)
    overlapped_xdot->Import(*xdot, *importer, Insert);

  // Zero out overlapped residual
  Teuchos::RCP<Epetra_Vector> overlapped_ff;
  if (f != NULL) {
    overlapped_ff = overlapped_f;
    overlapped_ff->PutScalar(0.0);
    f->PutScalar(0.0);
  }

  Teuchos::RCP<Epetra_MultiVector> overlapped_JVx;
  if (JVx != NULL) {
    overlapped_JVx = 
      Teuchos::rcp(new Epetra_MultiVector(*(disc->getOverlapMap()), 
					  JVx->NumVectors()));
    overlapped_JVx->PutScalar(0.0);
    JVx->PutScalar(0.0);
  }
  
  Teuchos::RCP<Epetra_MultiVector> overlapped_fVp;
  if (fVp != NULL) {
    overlapped_fVp = 
      Teuchos::rcp(new Epetra_MultiVector(*(disc->getOverlapMap()), 
					  fVp->NumVectors()));
    overlapped_fVp->PutScalar(0.0);
    fVp->PutScalar(0.0);
  }

  Teuchos::RCP<Epetra_MultiVector> overlapped_Vx;
  if (Vx != NULL) {
    overlapped_Vx = 
      Teuchos::rcp(new Epetra_MultiVector(*(disc->getOverlapMap()), 
					  Vx->NumVectors()));
  }

  Teuchos::RCP<const Teuchos::SerialDenseMatrix<int,double> > vp =
    Teuchos::rcp(Vp, false);
  Teuchos::RCP<Sacado::ScalarParameterVector> params = 
    Teuchos::rcp(p, false);

  // Create Jacobian init/post op
  Teuchos::RCP<FEApp::TangentOp> op;
  op = Teuchos::rcp(new FEApp::TangentOp(alpha, beta, sum_derivs,
					 overlapped_xdot, 
					 overlapped_x,
					 params,
					 overlapped_Vx,
					 overlapped_Vx,
					 vp,
					 overlapped_ff, 
					 overlapped_JVx,
					 overlapped_fVp));

  // Get template PDE instantiation
  Teuchos::RCP< FEApp::AbstractPDE<JacobianOp::fill_type> > pde = 
    pdeTM.getAsObject<JacobianOp::fill_type>();

  // Do global fill
  FEApp::GlobalFill<JacobianOp::fill_type> globalFill(disc->getMesh(), 
						      quad, pde, bc, 
						      transient);
  globalFill.computeGlobalFill(*op);

  // Assemble global residual
  if (f != NULL)
    f->Export(*overlapped_f, *exporter, Add);

  // Assemble derivatives
  if (JVx != NULL)
    JVx->Export(*overlapped_JVx, *exporter, Add);
  if (fVp != NULL)
    fVp->Export(*overlapped_fVp, *exporter, Add);
}

void
FEApp::Application::computeGlobalSGResidual(
				    const Epetra_Vector* sg_xdot,
				    const Epetra_Vector& sg_x,
				    const Sacado::ScalarParameterVector* p,
				    Epetra_Vector& sg_f)
{
#if SG_ACTIVE
  // Scatter x to the overlapped distrbution
  sg_overlapped_x->Import(sg_x, *sg_importer, Insert);

  // Scatter xdot to the overlapped distribution
  if (transient)
    sg_overlapped_xdot->Import(*sg_xdot, *sg_importer, Insert);

  // Set parameters
  if (p != NULL) {
    for (unsigned int i=0; i<p->size(); ++i) {
      (*p)[i].family->setRealValueForAllTypes((*p)[i].baseValue);
    }
  }

  // Zero out overlapped residual
  sg_overlapped_f->PutScalar(0.0);
  sg_f.PutScalar(0.0);

  // Create residual init/post op
  Teuchos::RCP<FEApp::SGResidualOp> op;
  op = Teuchos::rcp(new FEApp::SGResidualOp(disc->getOverlapMap(),
					    sg_basis,
					    sg_overlapped_xdot, 
					    sg_overlapped_x, 
					    sg_overlapped_f));

  // Get template PDE instantiation
  Teuchos::RCP< FEApp::AbstractPDE<SGResidualOp::fill_type> > pde = 
    pdeTM.getAsObject<SGResidualOp::fill_type>();

  // Do global fill
  FEApp::GlobalFill<SGResidualOp::fill_type> globalFill(disc->getMesh(), quad, 
							pde, bc, transient);
  globalFill.computeGlobalFill(*op);

  // Assemble global residual
  sg_f.Export(*sg_overlapped_f, *sg_exporter, Add);
#endif
}

void
FEApp::Application::computeGlobalSGJacobian(
				      double alpha, double beta,
				      const Epetra_Vector* sg_xdot,
				      const Epetra_Vector& sg_x,
				      const Sacado::ScalarParameterVector* p,
				      Epetra_Vector* sg_f,
				      Epetra_CrsMatrix& sg_jac)
{
#if SGFAD_ACTIVE
  // Scatter x to the overlapped distrbution
  sg_overlapped_x->Import(sg_x, *sg_importer, Insert);

  // Scatter xdot to the overlapped distribution
  if (transient)
    sg_overlapped_xdot->Import(*sg_xdot, *sg_importer, Insert);

  // Set parameters
  if (p != NULL) {
    for (unsigned int i=0; i<p->size(); ++i) {
      (*p)[i].family->setRealValueForAllTypes((*p)[i].baseValue);
    }
  }

  // Zero out overlapped residual
  Teuchos::RCP<EpetraExt::BlockVector> sg_overlapped_ff;
  if (sg_f != NULL) {
    sg_overlapped_ff = sg_overlapped_f;
    sg_overlapped_ff->PutScalar(0.0);
    sg_f->PutScalar(0.0);
  }

  // Zero out Jacobian
  sg_overlapped_jac->PutScalar(0.0);
  sg_jac.PutScalar(0.0);

  // Create Jacobian init/post op
  Teuchos::RCP<FEApp::SGJacobianOp> op;
  op = Teuchos::rcp(new FEApp::SGJacobianOp(disc->getOverlapMap(),
					    disc->getOverlapJacobianGraph(),
					    sg_basis,
					    Cijk,
					    alpha, beta, 
					    sg_overlapped_xdot, 
					    sg_overlapped_x, 
					    sg_overlapped_ff, 
					    sg_overlapped_jac));

  // Get template PDE instantiation
  Teuchos::RCP< FEApp::AbstractPDE<SGJacobianOp::fill_type> > pde = 
    pdeTM.getAsObject<SGJacobianOp::fill_type>();

  // Do global fill
  FEApp::GlobalFill<SGJacobianOp::fill_type> globalFill(disc->getMesh(), 
							quad, pde, bc, 
							transient);
  globalFill.computeGlobalFill(*op);

  // Assemble global residual
  if (sg_f != NULL)
    sg_f->Export(*sg_overlapped_f, *sg_exporter, Add);

  // Assemble global Jacobian
  sg_jac.Export(*sg_overlapped_jac, *sg_exporter, Add);

  sg_jac.FillComplete(true);
#endif
}
