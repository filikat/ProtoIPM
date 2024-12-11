#ifndef FACTORHIGHS_SOLVER_H
#define FACTORHIGHS_SOLVER_H

#include <algorithm>

#include "../FactorHiGHS/FactorHiGHS.h"
#include "LinearSolver.h"

class FactorHiGHSSolver : public LinearSolver {
  // symbolic factorization
  Symbolic S_;

  // numeric factorization
  Numeric N_;

  // keep track of whether as or ne is being factorized
  bool use_as_ = true;

 public:
  FactorHiGHSSolver(const Options& options);

  // Override functions
  int factorAS(const HighsSparseMatrix& A,
               const std::vector<double>& scaling) override;
  int factorNE(const HighsSparseMatrix& A,
               const std::vector<double>& scaling) override;
  int solveNE(const std::vector<double>& rhs,
              std::vector<double>& lhs) override;
  int solveAS(const std::vector<double>& rhs_x,
              const std::vector<double>& rhs_y, std::vector<double>& lhs_x,
              std::vector<double>& lhs_y) override;
  int setup(const HighsSparseMatrix& A, const Options& options) override;
  void clear() override;
  void refine(const HighsSparseMatrix& A, const std::vector<double>& scaling,
              const std::vector<double>& rhs_x,
              const std::vector<double>& rhs_y, std::vector<double>& lhs_x,
              std::vector<double>& lhs_y) override;
  void finalise() override;

  // Other functions
  void solveForRefineNE(const HighsSparseMatrix& A,
                        const std::vector<double>& scaling,
                        std::vector<double>& res_x,
                        std::vector<double>& res_y) const;
};

#endif
