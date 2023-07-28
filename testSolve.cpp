#include "Direct.h"
#include "Highs.h"

bool infNormDiffOk(const std::vector<double> x0, const std::vector<double> x1) {
  assert(x1.size() >= x0.size());
  double norm_diff = 0;
  for (HighsInt ix = 0; ix < HighsInt(x0.size()); ix++)
    norm_diff = std::max(std::fabs(x0[ix] - x1[ix]), norm_diff);
  return norm_diff < 1e-12;
}

int main() {

  const bool use_lp = false;
  //  const int test_matrix = 1;
  int x_dim;
  int y_dim;
  HighsSparseMatrix matrix;
  if (use_lp) {
    Highs highs;
    highs.setOptionValue("output_flag", false);
    HighsStatus status = highs.readModel("ml.mps");
    assert(status == HighsStatus::kOk);
    matrix = highs.getLp().a_matrix_;
    y_dim = matrix.num_row_;
    int nnz = matrix.numNz();
    for (int ix = 0; ix < y_dim; ix++) {
      matrix.start_.push_back(++nnz);
      matrix.index_.push_back(ix);
      matrix.value_.push_back(1);
    }
    matrix.num_col_ += y_dim;
    x_dim = matrix.num_col_;
  } else {
    x_dim = 4;
    y_dim = 2;
    matrix.num_row_ = y_dim;
    matrix.num_col_ = x_dim;
    matrix.format_ = MatrixFormat::kRowwise;
    matrix.start_ = {0, 3, 6};
    matrix.index_ = {0, 1, 2, 0, 1, 3};
    matrix.value_ = {1, 1, 1, 1, -1, 1};
  }
  matrix.ensureColwise();
  HighsRandom random;
  std::vector<double> theta;
  for (int ix = 0; ix < x_dim; ix++) theta.push_back(1.0);// + 1e-3 * random.fraction();

  // Test solution of
  //
  // [-\Theta A^T ][x_star] = [rhs_x]
  // [    A    0  ][y_star]   [rhs_y]
  //
  // First directly, and then by solving
  //
  // A\Theta.A^T y_star = rhs_y + A\Theta.rhs_x
  //
  // before substituting x_star = \Theta(A^Ty_star - rhs_x)
  
  const bool unit_solution = true;
  std::vector<double> x_star(x_dim);
  for (int ix = 0; ix < x_dim; ix++)
    x_star[ix] = unit_solution ? 1 : random.fraction();

  std::vector<double> y_star(y_dim);
  for (int ix = 0; ix < y_dim; ix++)
    y_star[ix] = unit_solution ? 1 : random.fraction();

  // Form rhs_x = -\Theta.x_star + A^T.y_star
  std::vector<double> at_y_star;
  matrix.productTranspose(at_y_star, y_star);
  std::vector<double> rhs_x = x_star;
  for (int ix = 0; ix < x_dim; ix++) {
    rhs_x[ix] *= -theta[ix];
    rhs_x[ix] += at_y_star[ix];
  }
  // Form rhs_y = A.x_star
  std::vector<double> rhs_y;
  matrix.product(rhs_y, x_star);

  const bool augmented_solve = false;
  const bool newton_solve = true;
  assert(augmented_solve || newton_solve);
  ExperimentData data;
  if (augmented_solve) {
    // Solve the augmented system
    std::vector<double> lhs_x;
    std::vector<double> lhs_y;

    int augmented_status = augmentedSolve(matrix, theta, rhs_x, rhs_y,
					  lhs_x, lhs_y, data);
    data.model_num_col = x_dim;
    data.model_num_row = y_dim;
    if (augmented_status) {
      std::cout << data << "\n";
      return augmented_status;
    }
    double solution_error = 0;
    for (int ix = 0; ix < x_dim; ix++)
      solution_error = std::max(std::fabs(x_star[ix] - lhs_x[ix]), solution_error);
    for (int ix = 0; ix < y_dim; ix++)
      solution_error = std::max(std::fabs(y_star[ix] - lhs_y[ix]), solution_error);
    data.solution_error = solution_error;
    std::cout << data << "\n";
    assert(data.solution_error < 1e-6);
    assert(data.residual_error < 1e-6);
  }

  if (newton_solve) {
    // Now solve the Newton equation
    // 
    // Form rhs_newton == rhs_y + A\Theta.rhs_x
    std::vector<double> theta_rhs_x = rhs_x;
    for (int ix = 0; ix < x_dim; ix++) theta_rhs_x[ix] *= theta[ix];
    std::vector<double> a_theta_rhs_x;
    matrix.product(a_theta_rhs_x, theta_rhs_x);
    std::vector<double> rhs_newton = rhs_y;
    for (int ix = 0; ix < y_dim; ix++) rhs_newton[ix] += a_theta_rhs_x[ix];
    
    std::vector<double> lhs(y_dim);
    int newton_status = newtonSolve(matrix, theta, rhs_newton, lhs, 1, 0.4, data);
    data.model_num_col = x_dim;
    data.model_num_row = y_dim;
    if (newton_status) {
      std::cout << data << "\n";
      return newton_status;
    }
    double solution_error = 0;
    for (int ix = 0; ix < y_dim; ix++)
      solution_error = std::max(std::fabs(y_star[ix] - lhs[ix]), solution_error);
    data.solution_error = solution_error;

    std::cout << data << "\n";
    assert(data.solution_error < 1e-6);
    assert(data.residual_error < 1e-6);
  }
  return 0;
}

