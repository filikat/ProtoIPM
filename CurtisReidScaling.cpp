#include "CurtisReidScaling.h"

#include "../FactorHiGHS/KrylovMethods.h"

void product(const double* x, std::vector<double>& y,
             const std::vector<int>& ptr, const std::vector<int>& rows) {
  // Multiply by matrix E, i.e. matrix A with all entries equal to one
  // E * x = y
  int n = ptr.size() - 1;
  for (int col = 0; col < n; ++col) {
    for (int el = ptr[col]; el < ptr[col + 1]; ++el) {
      y[rows[el]] += x[col];
    }
  }
}

void product_transpose(const double* x, std::vector<double>& y,
                       const std::vector<int>& ptr,
                       const std::vector<int>& rows) {
  // Multiply by matrix E^T, i.e. matrix A^T with all entries equal to one
  // E^T * x = y
  int n = ptr.size() - 1;
  for (int col = 0; col < n; ++col) {
    for (int el = ptr[col]; el < ptr[col + 1]; ++el) {
      y[col] += x[rows[el]];
    }
  }
}

// class to apply matrix
class CRscalingMatrix : public AbstractMatrix {
  const std::vector<double>& M_;
  const std::vector<double>& N_;
  const std::vector<int>& ptr_;
  const std::vector<int>& rows_;

 public:
  CRscalingMatrix(const std::vector<double>& M, const std::vector<double>& N,
                  const std::vector<int>& ptr, const std::vector<int>& rows)
      : M_{M}, N_{N}, ptr_{ptr}, rows_{rows} {}

  void apply(std::vector<double>& x) const override {
    int n = N_.size();
    int m = M_.size();

    // split rhs
    const double* rho = &x.data()[0];
    const double* gamma = &x.data()[m];

    // compute E*gamma
    std::vector<double> Egamma(m);
    product(gamma, Egamma, ptr_, rows_);

    // compute E^T*rho
    std::vector<double> ETrho(n);
    product_transpose(rho, ETrho, ptr_, rows_);

    // populate lhs

    // 0...m-1
    for (int i = 0; i < m; ++i) x[i] = M_[i] * rho[i] + Egamma[i];

    // m...m+n-1
    for (int j = 0; j < n; ++j) x[m + j] = ETrho[j] + N_[j] * gamma[j];
  }
};

// class to apply diagonal preconditioner
class CRscalingPrec : public AbstractMatrix {
  const std::vector<double>& M_;
  const std::vector<double>& N_;

 public:
  CRscalingPrec(const std::vector<double>& M, const std::vector<double>& N)
      : M_{M}, N_{N} {}

  void apply(std::vector<double>& x) const override {
    int m = M_.size();
    for (int i = 0; i < m; ++i) x[i] /= M_[i];
    for (int j = 0; j < N_.size(); ++j) x[m + j] /= N_[j];
  }
};

void CurtisReidScaling(const std::vector<int>& ptr,
                       const std::vector<int>& rows,
                       const std::vector<double>& val, std::vector<int>& rowexp,
                       std::vector<int>& colexp) {
  // Takes as input the CSC matrix A.
  // Computes Curtis-Reid scaling exponents for the matrix, using powers of 2.

  int n = colexp.size();
  int m = rowexp.size();

  // rhs for CG
  std::vector<double> rhs(m + n, 0.0);

  // sum abs log2 A_i:
  double* sumlogrow = &rhs.data()[0];

  // sum abs log2 A_:j
  double* sumlogcol = &rhs.data()[m];

  // number of nonzero entries in each row and column
  std::vector<double> row_entries(m, 0.0);
  std::vector<double> col_entries(n, 0.0);

  // log A_ij
  for (int col = 0; col < n; ++col) {
    for (int el = ptr[col]; el < ptr[col + 1]; ++el) {
      int row = rows[el];
      if (val[el] != 0.0) {
        double temp = log2(std::abs(val[el]));
        sumlogrow[row] += temp;
        sumlogcol[col] += temp;
        row_entries[row] += 1.0;
        col_entries[col] += 1.0;
      }
    }
  }

  // solve linear system with CG and diagonal preconditioner
  std::vector<double> exponents(m + n);
  CRscalingMatrix CRmat(row_entries, col_entries, ptr, rows);
  CRscalingPrec CRprec(row_entries, col_entries);
  int cgiter = Cg(&CRmat, &CRprec, rhs, exponents, 1e-6, 1000);
  printf("CR scaling required %d CG iterations\n", cgiter);

  // unpack exponents into various components
  for (int i = 0; i < m; ++i) rowexp[i] = -std::round(exponents[i]);
  for (int j = 0; j < n; ++j) colexp[j] = -std::round(exponents[m + j]);
}
