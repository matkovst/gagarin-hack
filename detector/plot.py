from matplotlib import pyplot as plt

data = [
    147.095, 147.315, 147.243, 147.271, 147.169, 147.01, 147.426, 147.707, 149.149, 148.645, 147.962, 147.264, 147.95, 147.19, 147.528, 147.228, 58.9111, 47.5723, 178.323, 147.935
]

plt.plot(data)
# plt.plot(data_mean)
# [plt.axvline(x = outlier, color = 'red') for outlier in outliers]
plt.show()