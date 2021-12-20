#!/usr/bin/python

import numpy as np
from rdm_utilities import read_Dice2RDM
import h5py


def test_2RDM(file1: str, file2: str, tol: float):
    """Compare the L2 norm for two RDM files. If they are .txt files, we
    assume they are from Dice and in the Dice RDM format. If they are in .npy
    we assume they're from PySCF and we process them accordingly, i.e. transpose
    some indices. If they are from an HDF5 file, we assume Dice RDM format.

    Parameters
    ----------
    file1 : str
        RDM file, either from Dice (.txt/.h5) or PySCF (.npy).
    file2 : str
        RDM file, either from Dice (.txt/.h5) or PySCF (.npy).
    tol : float
        Tolerance for L2 error.
    """

    rdm_1, rdm_2 = None, None

    if file1.endswith("npy"):
        rdm_1 = np.load(file1)
        rdm_1 = rdm_1.transpose(0, 2, 1, 3)
    elif file1.endswith(".h5"):
        f = h5py.File(file1, "r")
        rdm_1 = np.asarray(f["spatial_rdm/2RDM_0_0"])
    else:
        rdm_1 = read_Dice2RDM(file1)

    if file2.endswith("npy"):
        rdm_2 = np.load(file2)
        rdm_2 = rdm_2.transpose(0, 2, 1, 3)
    elif file2.endswith(".h5"):
        f = h5py.File(file2, "r")
        rdm_2 = np.asarray(f["spatial_rdm/2RDM_0_0"])
    else:
        rdm_2 = read_Dice2RDM(file2)

    l2_norm = np.linalg.norm(rdm_1 - rdm_2)
    error_per_element = l2_norm / rdm_1.size

    if error_per_element > float(tol):
        msg = "\t\033[91mFailed\033[00m  2RDM Test: Error per Element: {:.3e}\n".format(
            error_per_element
        )
        msg += "\t                   L2-Norm of Error: {:.3e}\n".format(
            l2_norm)
        msg += "\t                   L\u221E-Norm of Error: {:.3e}\n".format(
            np.max(np.abs(rdm_1 - rdm_2))
        )
        print(msg)
    else:
        msg = "\t\033[92mPassed\033[00m 2RDM Test: Error per Element: {:.3e}".format(
            error_per_element
        )
        print(msg)


if __name__ == "__main__":
    import argparse

    # Read in command line args
    # fmt: off
    parser = argparse.ArgumentParser(description="Compare SHCI 2RDM to a trusted calculation")
    parser.add_argument("--file1", help="RDM file 1", type=str, required=True)
    parser.add_argument("--file2", help="RMD file 2", type=str, required=True)
    parser.add_argument("--tol", help="Absolute tolerance when comparing relative L-2 norm", type=float, required=True)
    args = parser.parse_args()
    # fmt: on

    test_2RDM(args.file1, args.file2, args.tol)
