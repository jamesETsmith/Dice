#!/usr/bin/python

import numpy as np
import h5py


def test_spin_rdm(shci_data_file:str, tol: float):
    """Trace spin1RDM and compare to spatial 1RDM.

    Parameters
    ----------
    shci_data_file : str
        HDF5 file from Dice containing RDMs.
    tol : float
        Tolerance for L2 error.
    """
    f = h5py.File(shci_data_file, "r")

    spatialRDM = np.asarray(f["spatial_rdm/1RDM_0_0"])
    spinRDM = np.asarray(f["spin_rdm/1RDM_0_0"])

    # Trace over spin
    test_spatial = np.zeros_like(spatialRDM)
    for i in range(spatialRDM.shape[0]):
        for j in range(spatialRDM.shape[0]):
            for k in [0, 1]:
                test_spatial[i, j] += spinRDM[2 * i + k, 2 * j + k]

    l2_norm = np.linalg.norm(spatialRDM - test_spatial)

    error_per_element = l2_norm / spatialRDM.size

    if error_per_element > float(tol):
        msg = "\t\033[91mFailed\033[00m  Spin 1RDM Test: Error per Element: {:.3e}\n".format(
            error_per_element
        )
        msg += "\t                   L2-Norm of Error: {:.3e}\n".format(l2_norm)
        msg += "\t                   L\u221E-Norm of Error: {:.3e}\n".format(
            np.max(np.abs(spatialRDM - test_spatial))
        )
        print(msg)
    else:
        msg = "\t\033[92mPassed\033[00m Spin 1RDM Test: Error per Element: {:.3e}".format(
            error_per_element
        )
        print(msg)


if __name__ == "__main__":
    import argparse

    # Read in command line args
    # fmt: off
    parser = argparse.ArgumentParser(description="Compare spatial and spin RDMs")
    parser.add_argument("--shci_data_file", help="HDF5 data file from Dice", type=str, required=True)
    parser.add_argument("--rtol", help="Relative tolerance when comparing relative L-2 norm", type=float, required=True)
    args = parser.parse_args()
    # fmt: on

    test_spin_rdm(args.shci_data_file, args.rtol)
