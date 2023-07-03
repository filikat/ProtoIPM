#ifndef DIRECT_H
#define DIRECT_H

#include "util/HighsSparseMatrix.h"

void newtonSolve(const HighsSparseMatrix &highs_a,
		 const std::vector<double> &theta,
                 const std::vector<double> &rhs,
		 std::vector<double> &lhs,
		 const int option_max_dense_col,
		 const double option_dense_col_tolerance);

/*
void augmentedSolve(const HighsSparseMatrix &highs_a,
                 const std::vector<double> &scaling,
                 const std::vector<double> &rhs,
                 std::vector<double> &lhs);
*/
#endif
