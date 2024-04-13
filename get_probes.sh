#!/bin/bash

ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/anomaly/0.mp4 > data/Train/anomaly/0.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/anomaly/2.mp4 > data/Train/anomaly/2.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/anomaly/3.mp4 > data/Train/anomaly/3.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/anomaly/4.mp4 > data/Train/anomaly/4.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/anomaly/5.mp4 > data/Train/anomaly/5.ffprobe

ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/0.mp4 > data/Train/not_anomaly/0.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/1.mp4 > data/Train/not_anomaly/1.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/2.mp4 > data/Train/not_anomaly/2.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/3.mp4 > data/Train/not_anomaly/3.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/4.mp4 > data/Train/not_anomaly/4.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/5.mp4 > data/Train/not_anomaly/5.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/6.mp4 > data/Train/not_anomaly/6.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/7.mp4 > data/Train/not_anomaly/7.ffprobe
ffprobe -v quiet -show_entries frame=pkt_size,pkt_pts_time,key_frame data/Train/not_anomaly/8.mp4 > data/Train/not_anomaly/8.ffprobe
