#!/usr/bin/python

import numpy as np
import h5py

def test_rdm_nelec(
    shci_data_file: str,
    fcidump_file: str,
    tol: float
):

    # Get Nelec from FCIDUMP
    with open(fcidump_file, "r") as f:
        line = f.readline()
        nelec = int(line.split(",")[1].split("=")[1])

    # Get RDMs
    f = h5py.File(shci_data_file, "r")
    oneRDM =   np.asarray(f["spatial_rdm/1RDM_0_0"])
    twoRDM =   np.asarray(f["spatial_rdm/2RDM_0_0"])
    threeRDM = np.asarray(f["spatial_rdm/3RDM_0_0"]).transpose(0,1,2,5,4,3)

    # Check that the shapes match
    norb = oneRDM.shape[0]
    if twoRDM.shape != (norb,) * 4:
        raise ValueError(
            "Shape of spatial2RDM {} doesn't match the shape of spatial1RDM {}".format(
                twoRDM.shape, oneRDM.shape
            )
        )
    if threeRDM.shape != (norb,) * 6:
        raise ValueError(
            "Shape of spatial3RDM {} doesn't match the shape of spatial1RDM {}".format(
                threeRDM.shape, oneRDM.shape
            )
        )

    #
    # Check Nelec
    #
    nelec_from_1RDM = np.einsum("ii", oneRDM)

    dm1_from_2RDM = np.einsum("ijkj", twoRDM) / (nelec - 1)
    nelec_from_2RDM = np.einsum("ii", dm1_from_2RDM)

    dm2_from_3RDM = np.einsum("ijklmk", threeRDM) / (nelec - 2)
    dm1_from_3RDM = np.einsum("ijkj", dm2_from_3RDM) / (nelec - 1)
    nelec_from_3RDM = np.einsum("ii", dm1_from_3RDM)

    # Check 1RDM nelec
    if abs(nelec - nelec_from_1RDM) > tol:
        print(
            "\t\033[91mFailed\033[00m  RDM NELEC from spatial1RDM {:14.10f} != {:d} from FCIDUMP".format(
                nelec_from_1RDM, nelec
            )
        )
    else:
        print("\t\033[92mPassed\033[00m RDM NELEC from spatial1RDM")

    # Check 2RDM nelec
    if abs(nelec - nelec_from_2RDM) > tol:
        print(
            "\t\033[91mFailed\033[00m  RDM NELEC from spatial2RDM {:14.10f} != {:d} from FCIDUMP".format(
                nelec_from_2RDM, nelec
            )
        )
    else:
        print("\t\033[92mPassed\033[00m RDM NELEC from spatial2RDM")

    # Check 3RDM nelec
    if abs(nelec - nelec_from_3RDM) > tol:
        print(
            "\t\033[91mFailed\033[00m  RDM NELEC from spatial3RDM {:14.10f} != {:d} from FCIDUMP".format(
                nelec_from_3RDM, nelec
            )
        )
    else:
        print("\t\033[92mPassed\033[00m RDM NELEC from spatial3RDM")


if __name__ == "__main__":
    import argparse

    # Read in command line args
    # fmt: off
    parser = argparse.ArgumentParser(description="Compare spatial RDMs")
    parser.add_argument("--shci_data_file", help="HDF5 file from Dice", type=str, required=True)
    parser.add_argument("--fcidump", help="FCIDUMP from PySCF/MOLPRO", type=str, required=True)
    parser.add_argument("--tol", help="Absolute tolerance when comparing electrons from RDMs", type=float, required=True)
    args = parser.parse_args()
    # fmt: on

    test_rdm_nelec(args.shci_data_file, args.fcidump, args.tol)
