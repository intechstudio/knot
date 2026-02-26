FROM debian:trixie

RUN apt update

# Dependencies of esp-idf
RUN apt -y install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone esp-idf
RUN git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git

# Install tools used by esp-idf for esp32s3
WORKDIR /esp-idf
RUN ./install.sh esp32s3 > install-sh.log 2>&1
WORKDIR /

ENV IDF_PATH=/esp-idf
ENTRYPOINT ["/esp-idf/tools/docker/entrypoint.sh"]

# Install pre-commit from pip
RUN python3 -m pip install --break-system-packages pre-commit

# Pre-install hook environments using a bind mount so the config file is
# available during the build but never written into the image layer.
RUN --mount=type=bind,source=.pre-commit-config.yaml,target=/tmp/.pre-commit-config.yaml \
    cd /tmp && git init && \
    pre-commit install-hooks --config /tmp/.pre-commit-config.yaml && \
    rm -rf /tmp/.git

# Add /project as a safe git repository
RUN git config --global --add safe.directory /project

CMD ["bash"]
