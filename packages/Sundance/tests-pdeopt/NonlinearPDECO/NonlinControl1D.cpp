#include "Sundance.hpp"
#include "PDEOptNonlinearPDEConstrainedObj.hpp"

#include "PlayaBasicLMBFGS.hpp"
#include "PlayaOptBuilder.hpp"


int main(int argc, char** argv)
{
  try
		{
			Sundance::init(&argc, &argv);
      int np = MPIComm::world().getNProc();
      
      int nx = 64;
      const double pi = 4.0*atan(1.0);
      MeshType meshType = new BasicSimplicialMeshType();
      MeshSource mesher = new PartitionedLineMesher(0.0, pi, nx, meshType);

      Mesh mesh = mesher.getMesh();
      CellFilter interior = new MaximalCellFilter();
      CellFilter bdry = new BoundaryCellFilter();
      
      /* Create a vector space factory, used to 
       * specify the low-level linear algebra representation */
      VectorType<double> vecType = new EpetraVectorType();

      /* create symbolic coordinate functions */
      Expr x = new CoordExpr(0);

      /* create target function */
      double R = 0.01;
      Expr sx = sin(x);
      Expr cx = cos(x);
      Expr ssx = sin(sx);
      Expr sx2 = sx*sx;
      Expr cx2 = cx*cx;
      Expr f = sx2 - sx - ssx;
      Expr uStar = 2.0*R*(sx2-cx2) + R*sx2*ssx + sx;

      /* Form exact solution */
      Expr uEx = sx;
      Expr lambdaEx = R*sx2;
      Expr alphaEx = -lambdaEx/R;
      

      /* create a discrete space on the mesh */
      BasisFamily bas = new Lagrange(1);
      DiscreteSpace discreteSpace(mesh, bas, vecType);

      /* initialize the design, state, and multiplier vectors to constants */
      Expr alpha0 = new DiscreteFunction(discreteSpace, 0.25, "alpha0");
      Expr u0 = new DiscreteFunction(discreteSpace, 0.5, "u0");
      Expr lambda0 = new DiscreteFunction(discreteSpace, 0.25, "lambda0");

      /* create symbolic objects for test and unknown functions */
      Expr u = new UnknownFunction(bas, "u");
      Expr lambda = new UnknownFunction(bas, "lambda");
      Expr alpha = new UnknownFunction(bas, "alpha");

      /* create symbolic differential operators */
      Expr dx = new Derivative(0);
      Expr grad = dx;

      /* create quadrature rules of different orders */
      QuadratureFamily q1 = new GaussianQuadrature(1);
      QuadratureFamily q2 = new GaussianQuadrature(2);
      QuadratureFamily q4 = new GaussianQuadrature(4);

      /* Form objective function */
      Expr reg = Integral(interior, 0.5 * R * alpha*alpha, q2);
      Expr fit = Integral(interior, 0.5 * pow(u-uStar, 2.0), q4);

      Expr constraintEqn = Integral(interior, 
        (grad*lambda)*(grad*u) + lambda*(alpha + sin(u) + f), q4);
      Expr L = reg + fit + constraintEqn;

      Expr constraintBC = EssentialBC(bdry, lambda*u, q2);

      Functional Lagrangian(mesh, L, constraintBC, vecType);
      
      LinearSolver<double> adjSolver 
        = LinearSolverBuilder::createSolver("amesos.xml");
      ParameterXMLFileReader reader("nox-amesos.xml");
      ParameterList noxParams = reader.getParameters();
      NOXSolver nonlinSolver(noxParams);

      RCP<PDEConstrainedObjBase> obj = rcp(new NonlinearPDEConstrainedObj(
        Lagrangian, u, u0, lambda, lambda0, alpha, alpha0,
        nonlinSolver, adjSolver));

      Vector<double> xInit = obj->getInit();

      bool doFDCheck = true;
      if (doFDCheck)
      {
        Out::root() << "Doing FD check of gradient..." << endl;
        bool fdOK = obj->fdCheck(xInit, 1.0e-6, 0);
        if (fdOK) 
        {
          Out::root() << "FD check OK" << endl;
        }
        else
        {
          Out::root() << "FD check FAILED" << endl;
          TEST_FOR_EXCEPT(!fdOK);
        }
      }

      RCP<UnconstrainedOptimizerBase> opt 
          = OptBuilder::createOptimizer("basicLMBFGS.xml");
      opt->setVerb(3);

      OptState state = opt->run(obj, xInit);

      if (state.status() != Opt_Converged)
      {
        Out::root()<< "optimization failed: " << state.status() << endl;
        TEST_FOR_EXCEPT(state.status() != Opt_Converged);
      }
      else
      {
        Out::root() << "opt converged: " << state.iter() << " iterations"
                    << endl;
      }
      FieldWriter w = new MatlabWriter("NonlinControl1D");
      w.addMesh(mesh);
      w.addField("u", new ExprFieldWrapper(u0));
      w.addField("alpha", new ExprFieldWrapper(alpha0));
      w.addField("lambda", new ExprFieldWrapper(lambda0));
      w.write();

      double uErr = L2Norm(mesh, interior, u0-uEx, q4);
      double lamErr = L2Norm(mesh, interior, lambda0-lambdaEx, q4);
      double aErr = L2Norm(mesh, interior, alpha0-alphaEx, q4);
      Out::root() << "error in u = " << uErr << endl;
      Out::root() << "error in lambda = " << lamErr << endl;
      Out::root() << "error in alpha = " << aErr << endl;

      double tol = 0.05;
      Sundance::passFailTest(uErr + lamErr + aErr, tol);
    }
	catch(exception& e)
		{
      cerr << "main() caught exception: " << e.what() << endl;
		}
	Sundance::finalize();
  return Sundance::testStatus(); 
}
