import numpy as np
import h5py

f = h5py.File("shci_data.h5", "r")
# Check the keys
print(f.keys())
print(f["spatial_rdm"].keys())
print(f["spin_rdm"].keys())

dset = f["ci_vectors"]
# print(dset)

# Check normalization (and orthogonality)
ci_vectors = np.asarray(dset)
print(f"CI Vectors shape {ci_vectors.shape}")
# print(ci_vectors)

dets = np.asarray(f["determinants"])
print(f"Determinants shape {dets.shape}")
# print(dets)

# print(f["spatial 2 RDM"])
spat1RDM = np.asarray(f["spatial_rdm/1RDM_0_0"])
pyscf_spat1RDM = np.load("../H2He_rdm/pyscf_1RDM.npy")
print("Spatial 1RDM Error",np.linalg.norm(spat1RDM-pyscf_spat1RDM))

spat2RDM = np.asarray(f["spatial_rdm/2RDM_0_0"])
pyscf_spat2RDM = np.load("../H2He_rdm/pyscf_2RDM.npy").transpose(0,2,1,3)
print("Spatial 2RDM Error", np.linalg.norm(spat2RDM-pyscf_spat2RDM))

print(f"spatial 2RDM Shape {spat2RDM.shape}")

spin2RDM = np.asarray(f["spin_rdm/2RDM_0_0"])
print(f"spin 2RDM Shape {spin2RDM.shape}")

# unpacked_spin2RDM = np.zeros((spat1RDM.shape[0]*2,)*4)

# n_spin_orbs = spat1RDM.shape[0] * 2
# n_pairs = n_spin_orbs * (n_spin_orbs + 1)/2
# il = np.tril_indices(n_pairs)
# unpacked_spin2RDM[il,il] = spin2RDM

