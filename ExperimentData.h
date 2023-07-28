#ifndef EXPERIMENTDATA_H_
#define EXPERIMENTDATA_H_
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>

#include "Highs.h"

const int kDataNotSet = -1;
const int kSystemTypeAugmented = 1;
const int kSystemTypeNewton = 2;

class ExperimentData {
public:
  std::string decomposer;
  std::string model_name;
  int model_num_col;
  int model_num_row;
  int model_num_dense_col;
  double model_max_dense_col;
  double dense_col_tolerance;
  int use_num_dense_col;
  int system_type;
  int system_size;
  //bool is_A_positive_definite;
  int system_nnz;
  int nnz_L;
  double solution_error;
  double residual_error;
  double fill_in_factor;

  //time
  double time_taken;
  double form_time;
  double analysis_time;
  double factorization_time;
  double solve_time;

  void reset(){
    decomposer = "na";
    model_num_col = kDataNotSet;
    model_num_row = kDataNotSet;
    model_num_dense_col = kDataNotSet;
    model_max_dense_col = kDataNotSet;
    dense_col_tolerance = kDataNotSet;
    use_num_dense_col = kDataNotSet;
    system_type = kDataNotSet;
    system_size = kDataNotSet;
    system_nnz = kDataNotSet;
    nnz_L = kDataNotSet;
    solution_error = kDataNotSet;
    residual_error = kDataNotSet;
    fill_in_factor = kDataNotSet;
    time_taken = kDataNotSet;
    analysis_time = kDataNotSet;
    factorization_time = kDataNotSet;
    solve_time = kDataNotSet;
    
  }
  void fillIn_LL();
  void fillIn_LDL();

};

double getWallTime();

std::ostream& operator<<(std::ostream& os, const ExperimentData& data);
void writeDataToCSV(const std::vector<ExperimentData>& data, const std::string& filename);
double residualErrorAugmented(const HighsSparseMatrix& A, 
			      const std::vector<double> &theta,
			      const std::vector<double> &rhs_x,
			      const std::vector<double> &rhs_y,
			      std::vector<double> &lhs_x,
			      std::vector<double> &lhs_y);
double residualErrorNewton(const HighsSparseMatrix& A,
			   const std::vector<double>& theta,
			   const std::vector<double>& rhs,
			   const std::vector<double>& lhs);
#endif
