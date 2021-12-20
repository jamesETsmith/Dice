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

    spatial_rdm.close();
    spin_rdm.close();
    file.close();

  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
    std::cerr << "HDF5 CAUGHT A FILE EXCEPTION EXITING..." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void save_ci_vectors(const std::string filename,
                     const std::vector<Eigen::MatrixXx> &ci) {
  //
  const H5std_string FILE_NAME(filename);

  const int n_roots = ci.size();
  const int n_dets = ci[0].size();

  // Copy data to C-style arrays before handing off to HDF5
  // Write the data to the ci_vectors using default memory space, file
  // space, and transfer properties.
  double data[n_roots][n_dets];
  for (int i = 0; i < n_roots; i++) {
    std::copy(ci[i].data(), ci[i].data() + n_dets, &data[0][0] + i * n_dets);
  }

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

    ci_vectors.write(data, PredType::NATIVE_DOUBLE);
  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSet operations
  catch (DataSetIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSpace operations
  catch (DataSpaceIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }
}

void save_determinants(const std::string filename, Determinant *Dets,
                       size_t n_dets) {
  //
  const H5std_string FILE_NAME(filename);

  // Copy data to C-style arrays before handing off to HDF5
  int norbs = Dets[0].norbs;
  char dets_data[n_dets][norbs];

  // Loop over all determinants (could do this in parallel, but it shouldn't be
  // a bottleneck)
  for (size_t i = 0; i < n_dets; i++) {
    Determinant D_i = Dets[i];
    char occupation_vec[norbs];
    D_i.getRepArray(occupation_vec);
    std::copy(&occupation_vec[0], &occupation_vec[norbs - 1], dets_data[i]);
  }

  // Try block to detect exceptions raised by any of the calls inside it
  try {
    // Turn off the auto-printing when failure occurs so that we can
    // handle the errors appropriately
    Exception::dontPrint();

    // Create a new file using the default property lists.
    H5File file(FILE_NAME, H5F_ACC_RDWR);

    //
    // Dump Determinants
    //
    hsize_t det_dims[2];
    det_dims[0] = n_dets;
    det_dims[1] = norbs;
    DataSpace det_dataspace(2, det_dims);
    const H5std_string DET_DATASET_NAME("determinants");
    DataSet determinants = file.createDataSet(
        DET_DATASET_NAME, PredType::NATIVE_CHAR, det_dataspace);
    determinants.write(&dets_data[0], PredType::NATIVE_CHAR);

  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
    std::cerr << "FIleIException ERROR" << std::endl;
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSet operations
  catch (DataSetIException error) {
    error.printErrorStack();
    std::cerr << "DataSetIException ERROR" << std::endl;
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSpace operations
  catch (DataSpaceIException error) {
    error.printErrorStack();
    std::cerr << "DataSpaceIException ERROR" << std::endl;
    exit(EXIT_FAILURE);
  }
}

void save_rdm(const std::string filename, const Eigen::MatrixXx &rdm,
              const std::vector<int> &dimensions,
              const std::string dataset_name) {

  const H5std_string FILE_NAME(filename);

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
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSet operations
  catch (DataSetIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSpace operations
  catch (DataSpaceIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }
}

void save_energies(const std::string filename, const std::string dataset,
                   const std::vector<double> &energies) {

  const H5std_string FILE_NAME(filename);

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
    const int RANK = 1;
    hsize_t dims[RANK];
    dims[0] = energies.size();
    DataSpace dataspace(RANK, dims);

    const H5std_string DATASET_NAME(dataset);
    DataSet dataset =
        file.createDataSet(DATASET_NAME, PredType::NATIVE_DOUBLE, dataspace);
    dataset.write(&energies[0], PredType::NATIVE_DOUBLE);

  } // end of try block

  // catch failure caused by the H5File operations
  catch (FileIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSet operations
  catch (DataSetIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }

  // catch failure caused by the DataSpace operations
  catch (DataSpaceIException error) {
    error.printErrorStack();
    exit(EXIT_FAILURE);
  }
}