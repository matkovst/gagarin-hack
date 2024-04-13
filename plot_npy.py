import os, sys
import numpy as np
from matplotlib import pyplot as plt

if __name__ == "__main__":
    argc = len(sys.argv)
    if argc < 2:
        print("Specify path to .npy file")
        sys.exit(0)

    sample_path = str(sys.argv[1])
    samples = np.load(sample_path)
    for sample in samples:
        plt.plot(sample)
    plt.show()
