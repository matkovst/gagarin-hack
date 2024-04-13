import math
import os, sys
import numpy as np
from datetime import datetime
from matplotlib import pyplot as plt

to_kbytes = lambda bts : bts / 1024.0

N = 12
T_stdev = 3.0
T_grad = to_kbytes(5000)

argc = len(sys.argv)
if argc < 2:
    print("Specify path to .ffprobe file")
    sys.exit(0)

ffprobe_path = str(sys.argv[1])
ffprobe_filename = os.path.basename(ffprobe_path)

# Прочитать файл
key_frames = []
pkt_pts_times = []
pkt_sizes = []
bitrate = []
kbit_s = 0
bitrate_sec = 0.0
with open(ffprobe_path, 'r') as f:
    for line in f:
        spl = line.split('=')
        if len(spl) < 2:
            continue
        key, value = spl[0], spl[1]
        if key == "key_frame":
            key_frames.append(int(value))
        if key == "pkt_pts_time":
            pkt_pts_times.append(float(value))
            if float(value) > (bitrate_sec+4):
                bitrate_sec = math.floor(float(value))
                bitrate.append(kbit_s)
                kbit_s = 0.0
        if key == "pkt_size":
            pkt_sizes.append( to_kbytes(int(value)) )
            kbit_s += (8*int(value))

# Посчитать характеристики

gop_size = int(math.ceil(len(key_frames) / sum(key_frames)))

key_pkt_times = []
key_pkt_sizes = []
for is_key, pkt_time, pkt_size in zip(key_frames, pkt_pts_times, pkt_sizes):
    if is_key:
        key_pkt_times.append(pkt_time)
        key_pkt_sizes.append(pkt_size)

key_pkt_min_size = min(key_pkt_sizes)
key_pkt_max_size = max(key_pkt_sizes)

print(f"{ffprobe_path} # keyframes = {len(key_pkt_times)}")
print(f"{ffprobe_path} size range (kB) = {(key_pkt_max_size - key_pkt_min_size)}")

# Посчитать градиент

key_pkt_size_grads = [0]
for i in range(1, len(key_pkt_sizes)):
    g = key_pkt_sizes[i] - key_pkt_sizes[i-1]
    key_pkt_size_grads.append(g)

# Посчитать плавающие статистики

forgetting = 2.0 / (N + 1)
forgetting_inv = (1.0 - forgetting)
key_running_means = [float(key_pkt_sizes[0])]
key_running_stdev2s = [1.0]
anomalies = [0]
for i, size in enumerate(key_pkt_sizes[1:], 1):
    d = size - key_running_means[i-1]
    m = key_running_means[i-1] + forgetting*d
    s2 = forgetting_inv*key_running_stdev2s[i-1] + forgetting*d*d
    key_running_means.append(m)
    key_running_stdev2s.append(s2)
    if i > N:
        high_g = abs(key_pkt_size_grads[i]) > T_grad
        outlier = d*d > T_stdev*T_stdev*key_running_stdev2s[i-1]
        # if outlier:
        #     print(key_pkt_size_grads[i])
        anomalies.append(int(high_g and outlier))
    else:
        anomalies.append(0)

# Нарезать образцы сигнала

sample_size = 20
sample_wing = sample_size // 2
samples = []
for i, anomaly in enumerate(anomalies):
    if not anomaly:
        continue

    if i < sample_wing:
        samples.append(key_pkt_sizes[:i+sample_wing])
    elif (len(key_pkt_sizes) - i) < sample_wing:
        samples.append(key_pkt_sizes[i-(sample_size-(len(key_pkt_sizes) - i)):])
    else:
        samples.append(key_pkt_sizes[i-sample_wing:i+sample_wing])

with open(ffprobe_path[:-7] + 'samples.npy', 'wb') as f:
    np.save(f, samples)

# Построить графики

key_pkt_datetimes = []
for pkt_time in key_pkt_times:
    key_pkt_datetimes.append(datetime.fromtimestamp(pkt_time).strftime("%M:%S"))

fig = plt.figure()
fig_w = min(128, 48.0 * (len(key_pkt_times) / 100.0))
fig_h = min(6, 6.0 * ((key_pkt_max_size - key_pkt_min_size) / to_kbytes(25000.0)))
fig.set_size_inches(fig_w, fig_h)
ax = fig.add_subplot(111)
ax.set_title(f'Статистика опорных кадров ({ffprobe_path}) | GOP = {gop_size}')
ax.set_xlabel('Время')
ax.set_ylabel('kB', rotation = 0)
ax.axhline(y = 0, color = 'black', linestyle = '-', label = 'origin')
ax.plot(key_pkt_times, key_pkt_sizes, '.-', color = 'red', label = 'I-frame kB')
ax.plot(key_pkt_times, key_pkt_size_grads, '.-', color = 'green', label = '∇ I-frame kB')
ax.plot(key_pkt_times, key_running_means, '--', color = 'violet', label = f'μ(N={N})')
ax.fill_between(key_pkt_times, 
                np.float32(key_running_means) - np.sqrt(key_running_stdev2s), 
                np.float32(key_running_means) + np.sqrt(key_running_stdev2s), 
                color = 'violet', 
                alpha = 0.2,
                label = f'μ±σ')
# ax.plot(key_pkt_times, np.int32(anomalies)*np.float32(key_running_means), '-', color = 'red', label = f'μ(N={N})')
[ax.axvline(x = pkt_time, color = 'red') for pkt_time, anomaly in zip(key_pkt_times, anomalies) if anomaly]
#ax.set_xticks(np.arange(min(key_pkt_times), max(key_pkt_times)+1, 2), rotation=90)
ax.set_xticks(key_pkt_times)
ax.set_xticklabels(key_pkt_datetimes, rotation=90, ha='right')
ax.set_yticks(np.arange(min(key_pkt_sizes), max(key_pkt_sizes)+1, to_kbytes(2048)))
ax.legend()
fig.savefig(ffprobe_path[:-7] + "png")

# bitrate_fig = plt.figure()
# fig_w = min(128, 48.0 * (len(key_pkt_times) / 100.0))
# fig_h = min(6, 6.0 * ((key_pkt_max_size - key_pkt_min_size) / 25000.0))
# bitrate_fig.set_size_inches(fig_w, fig_h)
# ax = bitrate_fig.add_subplot(111)
# ax.set_title(f'Статистика битрейт ({ffprobe_path}) | GOP = {gop_size}')
# ax.set_xlabel('Время')
# ax.set_ylabel('kbit/s', rotation = 0)
# ax.plot(bitrate, '.-', color = 'red')
# # ax.legend()
# bitrate_fig.savefig(ffprobe_path[:-7] + "bitrate.png")