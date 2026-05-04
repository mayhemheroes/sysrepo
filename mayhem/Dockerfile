FROM --platform=linux/amd64 ubuntu:22.04 as builder

# Configure apt with retries for transient network failures
RUN echo "Acquire::Retries \"5\";" > /etc/apt/apt.conf.d/80retries && \
    apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential cmake git libpcre2-dev clang

RUN git clone https://github.com/CESNET/libyang
WORKDIR /libyang/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/install
RUN make -j8
RUN make install

COPY . /repo
WORKDIR /repo/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/install
RUN make -j8
RUN make install
RUN clang++ /repo/mayhem/fuzz_sr_log_syslog.cpp -fsanitize=fuzzer,address -I /install/include/ /install/lib/libsysrepo.so /install/lib/libyang.so -o /fuzz

FROM ubuntu:22.04 as package
COPY --from=builder /install /install
COPY --from=builder /fuzz /fuzz_sr_log_syslog
ENV LD_LIBRARY_PATH=/install/lib
