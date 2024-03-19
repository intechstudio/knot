# Use the base image
FROM docker.io/espressif/idf:v5.1.2

# Install pico sdk required dependencies
RUN apt update && \
    apt install -y git python3 cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential xxd && \
    mkdir -p pico && \
    cd pico && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch master && \
    cd pico-sdk/ && \
    git submodule update --init && \
    cd ..

# Set working directory
WORKDIR /

ENV PICO_SDK_PATH=/pico/pico-sdk

# Define default command
CMD ["bash"]
