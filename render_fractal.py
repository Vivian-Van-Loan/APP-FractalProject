from PIL import Image
import numpy as np
import matplotlib.pyplot as plt
import sys

arr = np.fromfile("fractal.out", dtype=np.uint8)
plt.imsave("fractal.png", arr, cmap='inferno')

arr = arr.reshape(int(sys.argv[1]), int(sys.argv[2]))
image = Image.fromarray(arr, mode='L')
image.show()
