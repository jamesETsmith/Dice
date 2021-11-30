
#include <string>
#include <vector>

#include "Determinants.h"
#include "global.h"

void create_hdf5_datafile(const std::string filename);
void save_ci_vectors(const std::vector<Eigen::MatrixXx> &ci, Determinant *Dets);
void save_rdm(const Eigen::MatrixXx &rdm, const std::vector<int> &dimensions,
              const std::string dataset_name);