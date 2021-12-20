#!/bin/bash
# This script controls the SHCI crontab test jobs.

printf "\n\nRunning Tests for SHCI/SHCISCF\n"
printf "======================================================\n"

MPICOMMAND="mpirun -np 12"
HCIPATH="../../Dice"
HERE=`pwd`

## Clean up
cd ${HERE}
./clean.sh

# O2 SHCI tests.
cd ${HERE}/o2_stoc
printf "...running o2_stoc\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=1e-5
python ../test_spatial_rdm.py --test=2 --file1=shci_data.h5 --file2=trusted2RDM.txt --rtol=1.e-8

cd ${HERE}/o2_det
printf "...running o2_det\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=1.0e-6

cd ${HERE}/o2_det_trev
printf "...running o2_det_trev\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=1.0e-6

cd ${HERE}/o2_det_trev_direct
printf "...running o2_det_trev_direct\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=1.0e-6

cd ${HERE}/o2_det_direct
printf "...running o2_det_direct\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=1.0e-6

cd ${HERE}/restart
printf "...running restart test\n"
$MPICOMMAND $HCIPATH input2.dat > output2.dat
$MPICOMMAND $HCIPATH input3.dat > output3.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=5e-5

cd ${HERE}/restart_trev
printf "...running restart test\n"
$MPICOMMAND $HCIPATH input2.dat > output2.dat
$MPICOMMAND $HCIPATH input3.dat > output3.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=5e-5

cd ${HERE}/fullrestart
printf "...running full restart test\n"
$MPICOMMAND $HCIPATH input2.dat > output2.dat
$MPICOMMAND $HCIPATH input3.dat > output3.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=5e-5

cd ${HERE}/ref_det
printf "...running reference determinant tests\n"
$MPICOMMAND $HCIPATH input1.dat > output1.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=e_spin_0.e --tol=5e-5 
$MPICOMMAND $HCIPATH input2.dat > output2.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=e_spin_2.e --tol=5e-5 
$MPICOMMAND $HCIPATH input3.dat > output3.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=e_spin_1.e --tol=5e-5 
$MPICOMMAND $HCIPATH input4.dat > output4.dat
python ${HERE}/test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=e_spin_3.e --tol=5e-5 

cd ${HERE}/spin1rdm
printf "...running spin1RDM tests\n"
$HCIPATH input.dat > output.dat
python ${HERE}/test_spin1RDM.py --shci_data_file=shci_data.h5 --rtol=1e-8
$MPICOMMAND $HCIPATH input.dat > output.dat
python ${HERE}/test_spin1RDM.py --shci_data_file=shci_data.h5 --rtol=1e-8

cd ${HERE}/H2He_rdm
printf "...running H2He RDM Test\n"
$HCIPATH input.dat > output.dat
python ${HERE}/test_spatial_rdm.py --test=1 --file1=shci_data.h5 --file2=pyscf_1RDM.npy --rtol=1.e-9
python ${HERE}/test_spatial_rdm.py --test=2 --file1=shci_data.h5 --file2=pyscf_2RDM.npy --rtol=1.e-9
python ${HERE}/test_spatial_rdm.py --test=3 --file1=shci_data.h5 --file2=pyscf_3RDM.npy --rtol=1.e-9
python ${HERE}/test_spin1RDM.py --shci_data_file=shci_data.h5 --rtol=1e-9
python ${HERE}/test_rdm_nelec.py --shci_data_file=shci_data.h5 --fcidump=../integrals/H2H2_FCIDUMP_nosym --tol=5e-8

cd ${HERE}/h2o_trev_direct_rdm
printf "...running H2O Trev Direct RDM Test\n"
$HCIPATH test_input.dat > output.dat
python ${HERE}/test_spatial_rdm.py --test=2 --file1=shci_data.h5 --file2=pyscf_2RDM.npy --rtol=2.e-8

cd ${HERE}/h2co_rdm
printf "...running H2CO RDM Test\n"
$HCIPATH test_input.dat > output.dat
python ${HERE}/test_spatial_rdm.py --test=1 --file1=shci_data.h5 --file2=pyscf_1RDM.npy --rtol=1e-8
python ${HERE}/test_spatial_rdm.py --test=2 --file1=shci_data.h5 --file2=pyscf_2RDM.npy --rtol=1e-9
python ${HERE}/test_spatial_rdm.py --test=3 --file1=shci_data.h5 --file2=pyscf_3RDM.npy --rtol=1e-10
python ${HERE}/test_rdm_nelec.py --fcidump=../integrals/h2co_FCIDUMP_nosymm --shci_data_file=shci_data.h5 --tol=1e-11

cd ${HERE}/lowest_energy_det
printf "...running test for finding the lowest energy determinants\n"
printf "        Running tests on Spin=0"
bash run_spin=0_tests.sh "${MPICOMMAND}" "${HCIPATH}"
printf "        Running tests on Spin=1"
bash run_spin=1_tests.sh "${MPICOMMAND}" "${HCIPATH}"
printf "        Running tests on Spin=2"
bash run_spin=2_tests.sh "${MPICOMMAND}" "${HCIPATH}"
printf "        Running tests on Spin=3"
bash run_spin=3_tests.sh "${MPICOMMAND}" "${HCIPATH}"
printf "        Running tests on Spin=4"
bash run_spin=4_tests.sh "${MPICOMMAND}" "${HCIPATH}"


#
# Larger/Longer Tests
#

# Mn(salen) tests.
cd ${HERE}/mn_salen_stoc
printf "...running mn_salen_stoc\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=8.0e-5

cd ${HERE}/mn_salen_seed
printf "...running mn_salen_seed\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=8.0e-5

# Cr2 Tests
cd ${HERE}/cr2_dinfh_rdm
printf "...running cr2_dinfh_rdm\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=8.0e-5
python ../test_spatial_rdm.py --file1=shci_data.h5 --file2=trusted2RDM.txt --rtol=1.e-9

cd ${HERE}/cr2_dinfh_trev_rdm
printf "...running cr2_dinfh_trev_rdm\n"
$MPICOMMAND $HCIPATH > output.dat
python ../test_energy.py --hdf5_file=shci_data.h5 --trusted_energy_file=trusted_hci.e --tol=8.0e-5
python ../test_spatial_rdm.py --file1=shci_data.h5 --file2=trusted2RDM.txt --rtol=1.e-9



## Clean up
cd ${HERE}
./clean.sh
