#ifndef METIS_CALLER_H
#define METIS_CALLER_H

#include "Direct.h"
#include "GKlib.h"
#include "VertexCover.h"
#include "metis.h"
#include "util/HighsSparseMatrix.h"

enum MetisPartitionType {
  kMetisAugmented,
  kMetisNormalEq,
};

class Metis_caller {
  // adjacency matrix of the graph
  HighsSparseMatrix M;

  // type of M (augmented or normal equations)
  MetisPartitionType type;

  // number of parts in the partition
  int nparts;

  // number of vertices and edges in the graph
  int nvertex;
  int nedges;

  // partition of the graph.
  // partition[i] is in [0, nparts - 1] when Metis returns.
  // partition[i] is in [0, nparts] after the permutation is computed, because
  //   the linking block is counted as another part.
  std::vector<int> partition;

  // permutation and inverse permutation for matrix M
  std::vector<int> permutation;
  std::vector<int> perminv;

  // Contains the size of the blocks, after getPermutation returns.
  // blockSize[i], 0 <= i < nparts: size of the diagonal blocks
  // blockSize[nparts]: size of the linking block
  std::vector<int> blockSize;

  // Contains the blocks, after getBlocks returns.
  // Blocks[2 * i] contains the i-th diagonal block
  // Blocks[2 * i + 1] contains the i-th linking block
  // Blocks[2 * nparts + 1] contains the Schur block (i.e. the last diagonal
  //   block that will become the Schur compl)
  std::vector<HighsSparseMatrix> Blocks;

  // Number of nonzeros of each of the diagonal and linking blocks, after
  // getNonzeros returns.
  // nzCount[2 * i] contains the nonzero count of the i-th diag block
  // nzCount[2 * i + 1] contains the nonzero count of the i-th linking block
  // nzCount[2 * nparts + 1] contains the nonzero count of the Schur block
  // nzCount[2 * nparts + 2] should not be used for anything
  std::vector<int> nzCount;

  // if debug is true, print out multiple files for debug, using out_file
  bool debug = true;
  std::ofstream out_file;

 public:
  // Constructor
  // Set up matrix M with either augmented system or normal equations, depending
  // on the value of type.
  Metis_caller(const HighsSparseMatrix& A, MetisPartitionType input_type,
               int input_nparts);

  // Call Metis and produce the partition of the graph
  void getMetisPartition();

  // From the partition, obtain the permutation to use for matrix M.
  // Two approached are tried to obtain a vertex cover of the edge cut:
  // - find a maximal matching of the edge cut
  // - "greedy" heuristic: for each edge in the cut, include in the vertex cover
  //    the node with highest original numbering (when possible)
  // The one which yields the smallest Schur complement is used.
  void getMetisPermutation();

  // Extracts the diagonal and linking blocks of matrix M permuted.
  void getBlocks();

  // Computes the number of nonzeros of each of the diagonal and linking blocks.
  void getNonzeros();

  // print for debug
  template <typename T>
  void debug_print(const std::vector<T>& vec, const std::string& filename);
  void debug_print(const HighsSparseMatrix& mat, const std::string& filename);
};

template <typename T>
void Metis_caller::debug_print(const std::vector<T>& vec,
                               const std::string& filename) {
  out_file.open(filename);
  for (auto a : vec) {
    out_file << a << '\n';
  }
  out_file.close();
}

#endif