FROM gocv/opencv:4.9.0

WORKDIR /home/gagarin-hack

COPY detector/ ./detector/
COPY docker/entry.sh /
RUN chmod 755 /entry.sh

RUN mkdir -p detector/build-docker
RUN cmake -Sdetector -Bdetector/build-docker \ 
    -DFFMPEG_LIB_DIR=/usr/lib/x86_64-linux-gnu \ 
    -DFFMPEG_INCLUDES_DIR=/usr/include/x86_64-linux-gnu \ 
    -DOpenCV_LIB_DIR=/usr/local/lib \ 
    -DOpenCV_INCLUDES_DIR=/usr/local/include/opencv4 \ 
    detector/
RUN make -C detector/build-docker

ENTRYPOINT ["/entry.sh"]
