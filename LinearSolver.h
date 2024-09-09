#ifndef LINEAR_SOLVER_H
#define LINEAR_SOLVER_H

#include <vector>

#include "Ipm_aux.h"
#include "Ipm_const.h"
#include "VectorOperations.h"
#include "util/HighsSparseMatrix.h"

// Interface class for solving augmented system or normal equations.
// Any linear solver needs to define the functions:
// - factorAS: factorize the augmented system
// - solveAS: solve a linear system with the augmented system
// - factorNE: factorize the normal equations
// - solveNE: solve a linear system with the normal equations
// - setup: perform any preliminary calculation (e.g. symbolic factorization)
// - clear: reset the data structure for the next factorization.

// NB: forming the normal equations or augmented system is delegated to the
// linear solver chosen, so that only the appropriate data (upper triangle,
// lower triangle, or else) is constructed.

class LinearSolver {
 public:
  bool valid_ = false;

  virtual int factorAS(const HighsSparseMatrix& A,
                       const std::vector<double>& theta) = 0;

  virtual int solveAS(const HighsSparseMatrix& A,
                      const std::vector<double>& theta,
                      const std::vector<double>& rhs_x,
                      const std::vector<double>& rhs_y,
                      std::vector<double>& lhs_x,
                      std::vector<double>& lhs_y) = 0;

  virtual int factorNE(const HighsSparseMatrix& A,
                       const std::vector<double>& theta) = 0;

  virtual int solveNE(const HighsSparseMatrix& A,
                      const std::vector<double>& theta,
                      const std::vector<double>& rhs,
                      std::vector<double>& lhs) = 0;

  virtual void setup(const HighsSparseMatrix& A, int type) = 0;

  virtual void clear() = 0;
};

#endif