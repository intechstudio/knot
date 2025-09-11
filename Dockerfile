FROM debian:trixie

RUN apt update

# Dependencies of esp-idf
RUN apt -y install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone esp-idf
RUN git clone -b v5.3.1 --recursive https://github.com/espressif/esp-idf.git

# Install tools used by esp-idf for esp32s3
WORKDIR /esp-idf
RUN ./install.sh esp32s3
WORKDIR /

ENV IDF_PATH=/esp-idf
ENTRYPOINT ["/esp-idf/tools/docker/entrypoint.sh"]

# Install pre-commit from pip
RUN python3 -m pip install --break-system-packages pre-commit
RUN pre-commit --version

# Copy pre-commit hooks and create a git directory,
# to allow missing environments of hooks to be installed
COPY ./.pre-commit-config.yaml /
RUN git init
RUN pre-commit install-hooks

# Add /project as a safe git repository
RUN git config --global --add safe.directory /project

CMD ["bash"]
