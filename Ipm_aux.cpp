#include "Ipm_aux.h"

#include <fstream>
#include <iostream>

#include "../FactorHiGHS/FactorHiGHSSettings.h"
#include "VectorOperations.h"

int computeLowerAThetaAT(const HighsSparseMatrix& matrix,
                         const std::vector<double>& scaling,
                         HighsSparseMatrix& AAT, const int max_num_nz) {
  // Create a row-wise copy of the matrix
  HighsSparseMatrix AT = matrix;
  AT.ensureRowwise();

  int AAT_dim = matrix.num_row_;
  AAT.num_col_ = AAT_dim;
  AAT.num_row_ = AAT_dim;
  AAT.start_.resize(AAT_dim + 1, 0);

  std::vector<std::tuple<int, int, double>> non_zero_values;

  // First pass to calculate the number of non-zero elements in each column
  //
  int AAT_num_nz = 0;
  std::vector<double> AAT_col_value(AAT_dim, 0);
  std::vector<int> AAT_col_index(AAT_dim);
  std::vector<bool> AAT_col_in_index(AAT_dim, false);
  for (int iRow = 0; iRow < AAT_dim; iRow++) {
    // Go along the row of A, and then down the columns corresponding
    // to its nonzeros
    int num_col_el = 0;
    for (int iRowEl = AT.start_[iRow]; iRowEl < AT.start_[iRow + 1]; iRowEl++) {
      int iCol = AT.index_[iRowEl];
      const double theta_value =
          scaling.empty() ? 1.0
                          : 1.0 / (scaling[iCol] + kPrimalStaticRegularization);
      if (!theta_value) continue;
      const double row_value = theta_value * AT.value_[iRowEl];
      for (int iColEl = matrix.start_[iCol]; iColEl < matrix.start_[iCol + 1];
           iColEl++) {
        int iRow1 = matrix.index_[iColEl];
        if (iRow1 < iRow) continue;
        double term = row_value * matrix.value_[iColEl];
        if (!AAT_col_in_index[iRow1]) {
          // This entry is not yet in the list of possible nonzeros
          AAT_col_in_index[iRow1] = true;
          AAT_col_index[num_col_el++] = iRow1;
          AAT_col_value[iRow1] = term;
        } else {
          // This entry is in the list of possible nonzeros
          AAT_col_value[iRow1] += term;
        }
      }
    }
    for (int iEl = 0; iEl < num_col_el; iEl++) {
      int iCol = AAT_col_index[iEl];
      assert(iCol >= iRow);
      const double value = AAT_col_value[iCol];

      non_zero_values.emplace_back(iRow, iCol, value);
      const int num_new_nz = 1;
      if (AAT_num_nz + num_new_nz >= max_num_nz)
        return kDecomposerStatusErrorOom;
      AAT.start_[iRow + 1]++;
      AAT_num_nz += num_new_nz;
      AAT_col_in_index[iCol] = false;
    }
  }

  // Prefix sum to get the correct column pointers
  for (int i = 0; i < AAT_dim; ++i) AAT.start_[i + 1] += AAT.start_[i];

  AAT.index_.resize(AAT.start_.back());
  AAT.value_.resize(AAT.start_.back());
  AAT.p_end_ = AAT.start_;
  AAT.p_end_.back() = AAT.index_.size();

  std::vector<int> current_positions = AAT.start_;

  // Second pass to actually fill in the indices and values
  for (const auto& val : non_zero_values) {
    int i = std::get<0>(val);
    int j = std::get<1>(val);
    double dot = std::get<2>(val);

    // i>=j, so to get lower triangle, i is the col, j is row
    AAT.index_[current_positions[i]] = j;
    AAT.value_[current_positions[i]] = dot;
    current_positions[i]++;
    AAT.p_end_[i] = current_positions[i];
  }
  AAT.p_end_.clear();
  return kDecomposerStatusOk;
}

void debug_print(std::string filestr, const std::vector<int>& data) {
  char filename[100];
  snprintf(filename, 100, "../FactorHiGHS/matlab/%s", filestr.c_str());

  FILE* out_file = fopen(filename, "w+");

  for (int i : data) {
    fprintf(out_file, "%d\n", i);
  }

  fclose(out_file);
}

void debug_print(std::string filestr, const std::vector<double>& data) {
  char filename[100];
  snprintf(filename, 100, "../FactorHiGHS/matlab/%s", filestr.c_str());

  FILE* out_file = fopen(filename, "w+");

  for (double d : data) {
    fprintf(out_file, "%.17e\n", d);
  }

  fclose(out_file);
}