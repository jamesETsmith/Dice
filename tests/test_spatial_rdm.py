#!/usr/bin/python

import numpy as np
from rdm_utilities import read_Dice2RDM
import h5py

def correct_index_ordering(rdm: np.ndarray)->np.ndarray:
    if rdm.ndim == 4: # 2-RDM
        rdm = rdm.transpose(0, 2, 1, 3)
    elif rdm.ndim == 6: # 3-RDM
        rdm = rdm.transpose(0, 2, 4, 1, 3, 5)

    return rdm

def read_rdm(test: int, filepath:str)->np.ndarray:
    if filepath.endswith("npy"): # Assuming this is from PySCF
        rdm = np.load(filepath)
        rdm = correct_index_ordering(rdm)
    elif filepath.endswith(".h5"):
        f = h5py.File(filepath, "r")
        rdm = np.asarray(f[f"spatial_rdm/{test}RDM_0_0"])
        if rdm.ndim == 6:
            rdm = rdm.transpose(0,1,2,5,4,3)
    else:
        rdm = read_Dice2RDM(filepath)

    # Check dimensions
    if rdm.ndim != int(2*test):
        raise ValueError("You specified an RDM file with the wrong number of dimensions.")

    return rdm

def test_rdm(test: int, file1: str, file2: str, tol: float):
    """Compare the L2 norm for two RDM files. If they are .txt files, we
    assume they are from Dice and in the Dice RDM format. If they are in .npy
    we assume they're from PySCF and we process them accordingly, i.e. transpose
    some indices. If they are from an HDF5 file, we assume Dice RDM format.

    Parameters
    ----------
    test : int
        The RDM to test, e.g. if test=1, this function tests the 1RDM.
    file1 : str
        RDM file, either from Dice (.txt/.h5) or PySCF (.npy).
    file2 : str
        RDM file, either from Dice (.txt/.h5) or PySCF (.npy).
    tol : float
        Tolerance for L2 error.
    """

    rdm_1 = read_rdm(test, file1)
    rdm_2 = read_rdm(test, file2)

    if rdm_1.shape != rdm_2.shape:
        raise ValueError("The RDMs you specified don't have matching dimensions")

    l2_norm = np.linalg.norm(rdm_1 - rdm_2)
    error_per_element = l2_norm / rdm_1.size

    rdm_type = f"{int(rdm_1.ndim/2)}RDM"

    if error_per_element > float(tol):
        msg = f"\t\033[91mFailed\033[00m  {rdm_type} Test: Error per Element: {error_per_element:.3e}\n"
        msg += f"\t                   L2-Norm of Error: {l2_norm:.3e}\n"
        msg += f"\t                   L\u221E-Norm of Error: {np.max(np.abs(rdm_1 - rdm_2)):.3e}\n"
        print(msg)
    else:
        msg = f"\t\033[92mPassed\033[00m {rdm_type} Test: Error per Element: {error_per_element:.3e}"
        print(msg)


if __name__ == "__main__":
    import argparse

    # Read in command line args
    # fmt: off
    parser = argparse.ArgumentParser(description="Compare spatial RDMs")
    parser.add_argument("--test", help="Which RDM to test", type=int, required=True)
    parser.add_argument("--file1", help="RDM file 1", type=str, required=True)
    parser.add_argument("--file2", help="RMD file 2", type=str, required=True)
    parser.add_argument("--rtol", help="Relative tolerance when comparing relative L-2 norm", type=float, required=True)
    args = parser.parse_args()
    # fmt: on

    test_rdm(args.test, args.file1, args.file2, args.rtol)
