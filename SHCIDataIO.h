
#include <string>
#include <vector>

#include "Determinants.h"
#include "global.h"

void create_hdf5_datafile(const std::string filename);
void save_ci_vectors(const std::string filename,
                     const std::vector<Eigen::MatrixXx> &ci);

void save_determinants(const std::string filename, Determinant *Dets,
                       size_t n_dets);
void save_rdm(const std::string filename, const Eigen::MatrixXx &rdm,
              const std::vector<int> &dimensions,
              const std::string dataset_name);
void save_energies(const std::string filename, const std::string dataset,
                   const std::vector<double> &energies);