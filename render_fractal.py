#import PIL
from PIL import Image
import numpy as np
import sys

arr = np.fromfile("fractal.out", dtype=np.uint8)
arr = arr.reshape(int(sys.argv[1]), int(sys.argv[2]))
#print(arr)

image = Image.fromarray(arr, mode='L')
image.show()
