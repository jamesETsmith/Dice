#include <H5Cpp.h>
#include <algorithm>

#include "SHCIDataIO.h"

using namespace H5;

void create_hdf5_datafile(const std::string filename) {
  // Try block to detect exceptions raised by any of the calls inside it
  try {
    // Turn off the auto-printing when failure occurs so that we can
    // handle the errors appropriately
    Exception::dontPrint();

    // Create a new file using the default property lists.
    const H5std_string FILE_NAME(filename);

    H5File file(FILE_NAME, H5F_ACC_TRUNC);

    Group spatial_rdm = file.createGroup("/spatial_rdm");
    Group spin_rdm = file.createGroup("/spin_rdm");
  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
  }
}

void save_ci_vectors(const std::vector<Eigen::MatrixXx> &ci,
                     Determinant *Dets) {
  //
  const H5std_string FILE_NAME("shci_data.h5");

  const int n_roots = ci.size();
  const int n_dets = ci[0].size();

  // Try block to detect exceptions raised by any of the calls inside it
  try {
    // Turn off the auto-printing when failure occurs so that we can
    // handle the errors appropriately
    Exception::dontPrint();

    // Create a new file using the default property lists.
    H5File file(FILE_NAME, H5F_ACC_RDWR);

    //
    // Dump CI Vectors
    //
    int RANK = 2;
    hsize_t dims[RANK];
    dims[0] = n_roots;
    dims[1] = n_dets;
    DataSpace dataspace(RANK, dims);

    const H5std_string DATASET_NAME("ci_vectors");
    DataSet ci_vectors =
        file.createDataSet(DATASET_NAME, PredType::NATIVE_DOUBLE, dataspace);

    // Write the data to the ci_vectors using default memory space, file
    // space, and transfer properties.
    double data[n_roots][n_dets];
    for (int i = 0; i < n_roots; i++) {
      std::copy(ci[i].data(), ci[i].data() + n_dets, &data[0][0] + i * n_dets);
    }
    ci_vectors.write(data, PredType::NATIVE_DOUBLE);

    //
    // Dump Determinants
    //
    hsize_t det_dims[2];
    det_dims[0] = n_dets;
    det_dims[1] = Dets[0].norbs;
    DataSpace det_dataspace(2, det_dims);
    const H5std_string DET_DATASET_NAME("determinants");
    DataSet determinants = file.createDataSet(
        DET_DATASET_NAME, PredType::NATIVE_HBOOL, det_dataspace);
    bool dets_data[n_dets][det_dims[1]];

    for (size_t i = 0; i < n_dets; i++) {
      for (size_t orb_i = 0; orb_i < det_dims[1]; orb_i++) {
        dets_data[i][orb_i] = Dets[i].getocc(orb_i);
      }
    }
    determinants.write(dets_data, PredType::NATIVE_HBOOL);

  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
    // return -1;
  }

  // catch failure caused by the DataSet operations
  catch (DataSetIException error) {
    error.printErrorStack();
    // return -1;
  }

  // catch failure caused by the DataSpace operations
  catch (DataSpaceIException error) {
    error.printErrorStack();
    // return -1;
  }
}

void save_rdm(const Eigen::MatrixXx &rdm, const std::vector<int> &dimensions,
              const std::string dataset_name) {

  const H5std_string FILE_NAME("shci_data.h5");

  // Try block to detect exceptions raised by any of the calls inside it
  try {
    // Turn off the auto-printing when failure occurs so that we can
    // handle the errors appropriately
    Exception::dontPrint();

    // Create a new file using the default property lists.
    H5File file(FILE_NAME, H5F_ACC_RDWR);

    //
    // Dump RDM
    //
    int RANK = dimensions.size();
    hsize_t dims[RANK];
    std::copy(dimensions.begin(), dimensions.end(), &dims[0]);
    DataSpace dataspace(RANK, dims);

    const H5std_string DATASET_NAME(dataset_name);
    DataSet dataset =
        file.createDataSet(DATASET_NAME, PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(rdm.data(), PredType::NATIVE_DOUBLE);

  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
    // return -1;
  }

  // catch failure caused by the DataSet operations
  catch (DataSetIException error) {
    error.printErrorStack();
    // return -1;
  }

  // catch failure caused by the DataSpace operations
  catch (DataSpaceIException error) {
    error.printErrorStack();
    // return -1;
  }
}
