#include "Direct.h"
#include "spral.h"
#include <cmath>

bool increasing_index(const HighsSparseMatrix& matrix) {
  if (matrix.isRowwise()) {
    for (int iRow = 0; iRow < matrix.num_row_; iRow++)
      for (int iEl = matrix.start_[iRow]+1; iEl < matrix.start_[iRow+1]; iEl++) 
	if (matrix.index_[iEl] <= matrix.index_[iEl-1]) return false;
  } else {
    for (int iCol = 0; iCol < matrix.num_col_; iCol++)
      for (int iEl = matrix.start_[iCol]+1; iEl < matrix.start_[iCol+1]; iEl++)
	if (matrix.index_[iEl] <= matrix.index_[iEl-1]) return false;
  }
  return true;
}

void productAThetaAT(const HighsSparseMatrix& matrix,
		     const std::vector<double>& theta,
		     const std::vector<double>& x,
		     std::vector<double>& result) {
  assert(int(x.size()) == matrix.num_row_);
  std::vector<double> ATx;
  matrix.productTranspose(ATx, x);
  if (!theta.empty()) 
    for (int ix = 0; ix < matrix.num_col_; ix++) ATx[ix] *= theta[ix];
  matrix.product(result, ATx);
}

HighsSparseMatrix computeAThetaAT(const HighsSparseMatrix& matrix,
				  const std::vector<double>& theta) {
  const bool scatter = true;
  // Create a row-wise copy of the matrix
  HighsSparseMatrix AT = matrix;
  AT.ensureRowwise();

  int AAT_dim = matrix.num_row_;
  HighsSparseMatrix AAT;
  AAT.num_col_ = AAT_dim;
  AAT.num_row_ = AAT_dim;
  AAT.start_.resize(AAT_dim+1,0);

  std::vector<std::tuple<int, int, double>> non_zero_values;

  // First pass to calculate the number of non-zero elements in each column
  //
  if (scatter) {
    std::vector<double> matrix_row(matrix.num_col_, 0);
    for (int iRow = 0; iRow < AAT_dim; iRow++) {
      const double theta_i = !theta.empty() ? theta[iRow] : 1;
      for (int iEl = AT.start_[iRow]; iEl < AT.start_[iRow+1]; iEl++) 
	matrix_row[AT.index_[iEl]] = AT.value_[iEl];
      for (int iCol = iRow; iCol < AAT_dim; iCol++) {
	double dot = 0.0;
	for (int iEl = AT.start_[iCol]; iEl < AT.start_[iCol+1]; iEl++) 
	  dot += theta_i * matrix_row[AT.index_[iEl]] * AT.value_[iEl];
	if (dot != 0.0) {
	  non_zero_values.emplace_back(iRow, iCol, dot);
	  AAT.start_[iRow+1]++;
	  if (iRow != iCol) AAT.start_[iCol+1]++;
	}
      }
      for (int iEl = AT.start_[iRow]; iEl < AT.start_[iRow+1]; iEl++) 
	matrix_row[AT.index_[iEl]] = 0;
      for (int ix = 0; ix < matrix.num_col_; ix++)
	assert(!matrix_row[ix]);
    }
  } else {
    assert(increasing_index(AT));
    for (int i = 0; i < AAT_dim; ++i) {
      const double theta_i = !theta.empty() ? theta[i] : 1;
      for (int j = i; j < AAT_dim; ++j) {
	double dot = 0.0;
	int k = AT.start_[i];
	int l = AT.start_[j];
	while (k < AT.start_[i + 1] && l < AT.start_[j + 1]) {
	  if (AT.index_[k] < AT.index_[l]) {
	    ++k;
	  } else if (AT.index_[k] > AT.index_[l]) {
	    ++l;
	  } else {                 
	    dot += theta_i * AT.value_[k] * AT.value_[l];
	    ++k;
	    ++l;
	  }
	}
	if (dot != 0.0) {
	  non_zero_values.emplace_back(i, j, dot);
	  AAT.start_[i+1]++;
	  if (i != j) AAT.start_[j+1]++;
	}
      }
    }
  }

  // Prefix sum to get the correct column pointers
  for (int i = 0; i < AAT_dim; ++i) 
    AAT.start_[i+1] += AAT.start_[i];
 
  AAT.index_.resize(AAT.start_.back());
  AAT.value_.resize(AAT.start_.back());
  AAT.p_end_ = AAT.start_;
  AAT.p_end_.back() = AAT.index_.size();
 
  std::vector<int> current_positions = AAT.start_;
 
  // Second pass to actually fill in the indices and values
  for (const auto& val : non_zero_values){
    int i = std::get<0>(val);
    int j = std::get<1>(val);
    double dot = std::get<2>(val);
 
    AAT.index_[current_positions[i]] = j;
    AAT.value_[current_positions[i]] = dot;
    current_positions[i]++;
    AAT.p_end_[i] = current_positions[i];
 
    if (i != j){
      AAT.index_[current_positions[j]] = i;
      AAT.value_[current_positions[j]] = dot;
      current_positions[j]++;
      AAT.p_end_[j] = current_positions[j];
    }
  }
  AAT.p_end_.clear();
  return AAT;
}

int augmentedSolve(const HighsSparseMatrix &highs_a,
		   const std::vector<double> &scaling,
		   const std::vector<double> &rhs_x,
		   const std::vector<double> &rhs_y,
		   std::vector<double> &lhs_x,
		   std::vector<double> &lhs_y,
		   ExperimentData& data) {
  return 1;
}

int newtonSolve(const HighsSparseMatrix &highs_a,
		const std::vector<double> &theta,
		const std::vector<double> &rhs,
		std::vector<double> &lhs,
		const int option_max_dense_col,
		const double option_dense_col_tolerance,
		ExperimentData& data) {

  assert(highs_a.isColwise());
  std::vector<double> use_theta = theta;
  double use_dense_col_tolerance = option_dense_col_tolerance;
  //  use_dense_col_tolerance = 1.2;
  //  use_dense_col_tolerance = 0.1;

  double start_time0 = getWallTime();
  double start_time = start_time0;
  int num_dense_col = 0;
  int col_max_nz = 0;
  std::vector<double> density;
  for (int col = 0; col < highs_a.num_col_; col++) {
    int col_nz = highs_a.start_[col+1] - highs_a.start_[col];
    double density_value = double(col_nz) / double(highs_a.num_row_);
    density.push_back(density_value);
    col_max_nz = std::max(col_nz, col_max_nz);
    if (density_value > use_dense_col_tolerance) {
      num_dense_col++;
      //      use_theta[col] = 0;
    }
  }
  double max_density = double(col_max_nz)/ double(highs_a.num_row_);
  printf("Problem has %d rows and %d columns (max nonzeros = %d; density = %g) with %d dense at a tolerance of %g\n",
	 int(highs_a.num_row_), int(highs_a.num_col_),
	 int(col_max_nz), max_density, int(num_dense_col), use_dense_col_tolerance);
  analyseVectorValues(nullptr, "Column density", highs_a.num_col_, density);

  HighsSparseMatrix AAT = computeAThetaAT(highs_a, use_theta);
  data.reset();
  data.decomposer = "ssids";
  data.system_size = highs_a.num_row_;
  data.system_nnz = AAT.numNz();

  // Prepare data structures for SPRAL
  std::vector<long> ptr;
  std::vector<int> row;
  std::vector<double> val;

  // Extract upper triangular part of AAT
  for (int col = 0; col < AAT.num_col_; col++){
    ptr.push_back(val.size());
    for (int idx = AAT.start_[col]; idx < AAT.start_[col+1]; idx++){
      int row_idx = AAT.index_[idx];
      if (row_idx >= col){
	val.push_back(AAT.value_[idx]);
	row.push_back(row_idx + 1);
      }
    }
  }

  for (auto& p : ptr) {
    ++p;
  }
  ptr.push_back(val.size() + 1);

  long* ptr_ptr = ptr.data();
  int* row_ptr = row.data();
  double* val_ptr = val.data();

  // Derived types
  void *akeep, *fkeep;
  struct spral_ssids_options options;
  struct spral_ssids_inform inform;

  // Initialize derived types
  akeep = NULL; fkeep = NULL;
  spral_ssids_default_options(&options);
  options.array_base = 1; // Need to set to 1 if using Fortran 1-based indexing 
  
  data.form_time = getWallTime() - start_time;

  // Compute solution in lhs
  lhs = rhs;

  // Perform analyse and factorise with data checking 
  bool check = true;
  start_time = getWallTime();
  spral_ssids_analyse(check, AAT.num_col_, NULL, ptr_ptr, row_ptr, NULL, &akeep, &options, &inform);
  data.analysis_time = getWallTime() - start_time;
  if(inform.flag<0) {
    spral_ssids_free(&akeep, &fkeep);
    return 1;
  }
  
  bool positive_definite =true;
  start_time = getWallTime();
  spral_ssids_factor(positive_definite, NULL, NULL, val_ptr, NULL, akeep, &fkeep, &options, &inform);
  data.factorization_time = getWallTime() - start_time;
  if(inform.flag<0) {
    spral_ssids_free(&akeep, &fkeep);
    throw std::runtime_error("Error in spral_ssids_factor");
  }
  //Return the diagonal entries of the Cholesky factor
  std::vector<double> d(AAT.num_col_);
  
  void spral_ssids_enquire_posdef(const void *akeep,
				  const void *fkeep,
				  const struct spral_ssids_options *options,
				  struct spral_ssids_inform *inform,
				  double *d);
  if (inform.flag<0){
    spral_ssids_free(&akeep, &fkeep);
    return 1;
  }

  // Solve 
  start_time = getWallTime();
  spral_ssids_solve1(0, lhs.data(), akeep, fkeep, &options, &inform);
  data.solve_time = getWallTime() - start_time;
  if(inform.flag<0) {
    spral_ssids_free(&akeep, &fkeep);
    throw std::runtime_error("Error in spral_ssids_solve1");
  }
  data.nnz_L = inform.num_factor;
  data.time_taken = getWallTime() - start_time0;

  data.fillIn_LL();
  data.residual_error = residualErrorAThetaAT(highs_a, theta, rhs, lhs);

  // Free the memory allocated for SPRAL
  int cuda_error = spral_ssids_free(&akeep, &fkeep);
  if (cuda_error != 0){
    return 1;
  }

  return 0;
}
