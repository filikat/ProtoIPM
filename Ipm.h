#ifndef IPM_H
#define IPM_H

#include <string>

#include "../FactorHiGHS/FactorHiGHS.h"
#include "CgSolver.h"
#include "FactorHiGHSSolver.h"
#include "IpmIterate.h"
#include "IpmModel.h"
#include "Ipm_aux.h"
#include "Ipm_const.h"
#include "LinearSolver.h"
#include "VectorOperations.h"
#include "util/HighsSparseMatrix.h"

class Ipm {
  // LP model
  IpmModel model_;

  // Objects used during iterations
  NewtonDir delta_{};

  // Linear solver interface
  std::unique_ptr<LinearSolver> LS_;

  // Iterate object interface
  std::unique_ptr<IpmIterate> it_;

  // Size of the problem
  int m_{}, n_{};

  // Iterations counters
  int iter_{}, bad_iter_{};

  // Other statistics
  double min_prod_{}, max_prod_{};

  // Stepsizes
  double alpha_primal_{}, alpha_dual_{};

  // Coefficient for reduction of mu
  double sigma_{}, sigma_affine_{};

  // Status of the solver
  IpmStatus ipm_status_ = kIpmStatusMaxIter;

  // Run-time options
  Options options_{};

  // Timer for iterations
  Clock clock_;

  // user solution
  std::vector<double> x_user, xl_user, xu_user, slack_user, y_user, zl_user,
      zu_user;

 public:
  // ===================================================================================
  // Load an LP:
  //
  //  min   obj^T * x
  //  s.t.  Ax {<=,=,>=} rhs
  //        lower <= x <= upper
  //
  // Transform constraints in equalities by adding slacks to inequalities:
  //  <= : add slack    0 <= s_i <= +inf
  //  >= : add slack -inf <= s_i <=    0
  // ===================================================================================
  void load(const int num_var,           // number of variables
            const int num_con,           // number of constraints
            const double* obj,           // objective function c
            const double* rhs,           // rhs vector b
            const double* lower,         // lower bound vector
            const double* upper,         // upper bound vector
            const int* A_ptr,            // column pointers of A
            const int* A_rows,           // row indices of A
            const double* A_vals,        // values of A
            const char* constraints,     // type of constraints
            const std::string& pb_name,  // problem name
            const Options& options       // options
  );

  // ===================================================================================
  // Solve the LP
  // ===================================================================================
  IpmStatus solve();

  // ===================================================================================
  // Extract information
  // ===================================================================================
  void getSolution(std::vector<double>& x, std::vector<double>& xl,
                   std::vector<double>& xu, std::vector<double>& slack,
                   std::vector<double>& y, std::vector<double>& zl,
                   std::vector<double>& zu) const;
  int getIter() const;

 private:
  // ===================================================================================
  // Solve:
  //
  // ___Augmented system___
  //
  //      [ -Theta^{-1}  A^T ] [ Deltax ] = [ res7 ]
  //      [ A            0   ] [ Deltay ] = [ res1 ]
  //
  // with:
  //  res7 = res4 - Xl^{-1} * (res5 + Zl * res2) + Xu^{-1} * (res6 - Zu * res3)
  //  Theta^{-1} = diag( scaling )
  //
  // (the computation of res7 takes into account only the components for which
  // the correspoding upper/lower bounds are finite)
  //
  // OR
  //
  // ___Normal equations___
  //
  //      A * Theta * A^T * Deltay = res8
  //      Delta x = Theta * (A^T* Deltay - res7)
  //
  // with:
  //  res8 = res1 + A * Theta * res7
  // ===================================================================================
  bool solveNewtonSystem(NewtonDir& delta);

  // ===================================================================================
  // Reconstruct the solution of the full Newton system:
  //
  //  Deltaxl = Deltax - res2
  //  Deltaxu = res3 - Deltax
  //  Deltazl = Xl^{-1} * (res5 - zl * Deltaxl)
  //  Deltazu = Xu^{-1} * (res6 - zu * Deltaxu)
  // ===================================================================================
  bool recoverDirection(NewtonDir& delta);

  // ===================================================================================
  // Steps to boundary are computed so that
  //
  //  x  + alpha_primal * Deltax
  //  xl + alpha_primal * Deltaxl > 0     (if lower bound finite)
  //  xu + alpha_primal * Deltaxu > 0     (if upper bound finite)
  //
  //  y  + alpha_dual * Deltay
  //  zl + alpha_dual * Deltazl > 0       (if lower bound finite)
  //  zu + alpha_dual * Deltazu > 0       (if upper bound finite)
  //
  // If cor is valid, the direction used is delta + weight * cor
  // If block is valid, the blocking index is returned.
  // ===================================================================================

  // step to boundary for single direction
  double stepToBoundary(const std::vector<double>& x,
                        const std::vector<double>& dx,
                        const std::vector<double>* cor, double weight, bool lo,
                        int* block = nullptr) const;

  // primal and dual steps to boundary
  void stepsToBoundary(double& alpha_primal, double& alpha_dual,
                       const NewtonDir& delta, const NewtonDir* cor = nullptr,
                       double weight = 1.0) const;

  // ===================================================================================
  // Find stepsizes using Mehrotra heuristic.
  // Given the steps to the boundary for xl, xu, zl, zu, and the blocking
  // indices, find stepsizes so that the primal (resp dual) blocking variable
  // produces a complementarity product not too far from the mu that would be
  // obtained using the steps to the boundary.
  // ===================================================================================
  void stepSizes();

  // ===================================================================================
  // Make the step in the Newton direction with appropriate stepsizes.
  // ===================================================================================
  void makeStep();

  // ===================================================================================
  // Compute the Mehrotra starting point.
  // In doing so, CG is used to solve two linear systems with matrix A*A^T.
  // This task does not need a factorization and can continue to use CG, because
  // these linear systems are very easy to solve with CG.
  // ===================================================================================
  void startingPoint();

  // ===================================================================================
  // Compute the sigma to use for affine scaling direction or correctors, based
  // on the smallest stepsize of the previous iteration.
  // If stepsize was large, use small sigma to reduce mu.
  // If stepsize was small, use large sigma to re-centre.
  //
  //  alpha | sigma  |   sigma    |
  //        | affine | correctors |
  //  1.0   |--------|------------|
  //        |        |    0.01    |
  //  0.5   |        |------------|
  //        |        |    0.10    |
  //  0.2   |        |------------|
  //        |  0.01  |    0.25    |
  //  0.1   |        |------------|
  //        |        |    0.50    |
  //  0.05  |        |------------|
  //        |        |    0.90    |
  //  0.0   |--------|------------|
  //
  // ===================================================================================
  void sigmaAffine();
  void sigmaCorrectors();

  // ===================================================================================
  // Compute the residuals for the computation of multiple centrality
  // correctors.
  // ===================================================================================
  void residualsMcc();

  // ===================================================================================
  // Iteratively compute correctors, until they improve the stepsizes.
  // Based on Gondzio, "Multiple centrality corrections in a primal-dual method
  // for linear programming" and Colombo, Gondzio, "Further Development of
  // Multiple Centrality Correctors for Interior Point Methods".
  // ===================================================================================
  bool centralityCorrectors();

  // ===================================================================================
  // Given the current direction delta and the latest corrector, compute the
  // best primal and dual weights, that maximize the primal and dual stepsize.
  // ===================================================================================
  void bestWeight(const NewtonDir& delta, const NewtonDir& corrector,
                  double& wp, double& wd, double& alpha_p,
                  double& alpha_d) const;

  // ===================================================================================
  // If the current iterate is nan or inf, abort the iterations.
  // ===================================================================================
  bool checkIterate();

  // ===================================================================================
  // If too many bad iterations happened consecutively, abort the iterations.
  // ===================================================================================
  bool checkBadIter();

  // ===================================================================================
  // Check the termination criterion:
  //  - primal infeasibility < tolerance
  //  - dual infeasiblity    < tolerance
  //  - relative dual gap    < tolerance
  // ===================================================================================
  bool checkTermination();

  // ===================================================================================
  // Compute the normwise and componentwise backward error for the large 6x6
  // linear system
  // ===================================================================================
  void backwardError(const NewtonDir& delta) const;

  void printInfo() const;
  void printHeader() const;
  void printOutput() const;
  void collectData() const;
};

#endif
