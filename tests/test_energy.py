#!/usr/bin/python

import struct
import h5py


def run_test(shci_hdf5_file: str, trusted_energy_file: str, tol: float):
    # Calculated
    f = h5py.File(shci_hdf5_file, "r")
    calc_e = f["total_energies"][0]

    # Trusted
    file2 = open(trusted_energy_file, "rb")
    given_e = struct.unpack("d", file2.read(8))[0]

    # Comparison
    if abs(given_e - calc_e) > tol:
        print("\t", given_e, "-", calc_e, " > ", tol)
        print("\t\033[91mFailed\033[00m Energy Test....")
    else:
        print("\t\033[92mPassed\033[00m Energy Test....")


if __name__ == "__main__":
    import argparse

    # Read in command line args
    # fmt: off
    parser = argparse.ArgumentParser(description="Compare SHCI energy to a known value")
    parser.add_argument("--hdf5_file", help="SHCI HDF5 data file.", type=str, required=True)
    parser.add_argument("--trusted_energy_file", help="Trusted SHCI energy as a binary file.", type=str, required=True)
    parser.add_argument("--tol", help="Absolute tolerance when comparing energies", type=float, required=True)
    args = parser.parse_args()
    # fmt: on

    run_test(args.hdf5_file, args.trusted_energy_file, args.tol)
