from PIL import Image
import numpy as np
import matplotlib.pyplot as plt

arr = np.loadtxt("fractal_openmp_output.out", delimiter=",", dtype=np.uint8)
plt.imsave("openmp.png", arr, cmap='inferno')