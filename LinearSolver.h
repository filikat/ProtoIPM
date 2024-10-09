#ifndef LINEAR_SOLVER_H
#define LINEAR_SOLVER_H

#include <vector>

#include "Ipm_aux.h"
#include "Ipm_const.h"
#include "VectorOperations.h"
#include "util/HighsSparseMatrix.h"

struct LS_data {
  double minD;       // minimum element of D
  double maxD;       // maximum element of D
  double minL;       // minimum element of L
  double maxL;       // maximum element of L
  double max_reg;    // maximum regularization used
  int num_reg;       // number of regularized pivots
  double worst_res;  // worst residual after refinement
};

// Interface class for solving augmented system or normal equations.
//
// Any linear solver needs to define the functions:
// - factorAS: factorize the augmented system
// - solveAS: solve a linear system with the augmented system
// - factorNE: factorize the normal equations
// - solveNE: solve a linear system with the normal equations
// - clear: reset the data structure for the next factorization.
//
// The linear solver may also define functions:
// - setup: perform any preliminary calculation (e.g. symbolic factorization)
// - refine: apply iterative refinement to the solution
// - finalise: perform any final action
//
// NB: forming the normal equations or augmented system is delegated to the
// linear solver chosen, so that only the appropriate data (upper triangle,
// lower triangle, or else) is constructed.

class LinearSolver {
 public:
  bool valid_ = false;

  // =================================================================
  // Pure virtual functions.
  // These need to be defined by any derived class.
  // =================================================================
  virtual int factorAS(const HighsSparseMatrix& A,
                       const std::vector<double>& scaling) = 0;

  virtual int solveAS(const std::vector<double>& rhs_x,
                      const std::vector<double>& rhs_y,
                      std::vector<double>& lhs_x,
                      std::vector<double>& lhs_y) = 0;

  virtual int factorNE(const HighsSparseMatrix& A,
                       const std::vector<double>& scaling) = 0;

  virtual int solveNE(const std::vector<double>& rhs,
                      std::vector<double>& lhs) = 0;

  virtual void clear() = 0;
  // =================================================================

  // =================================================================
  // Virtual functions.
  // These may be overridden by derived classes, if needed.
  // =================================================================
  virtual int setup(const HighsSparseMatrix& A, const Options& options) {
    return 0;
  }

  virtual void refine(const HighsSparseMatrix& A,
                      const std::vector<double>& scaling,
                      const std::vector<double>& rhs_x,
                      const std::vector<double>& rhs_y,
                      std::vector<double>& lhs_x, std::vector<double>& lhs_y) {}

  virtual void finalise() {}

  virtual void extractData(LS_data& data){};
  // =================================================================
};

#endif