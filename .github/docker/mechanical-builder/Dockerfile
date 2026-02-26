FROM docker.io/continuumio/miniconda3:latest

ENV DEBIAN_FRONTEND=noninteractive

# Runtime dependencies:
#   libfuse2     - required to run --appimage-extract
#   xvfb, x11vnc - virtual display for FreeCAD (FreeCADGui / TechDrawGui need it)
#   openscad     - renders STL files to PNG previews
RUN apt-get update && apt-get install -y \
    wget \
    libfuse2 \
    xvfb \
    x11vnc \
    openscad \
    && rm -rf /var/lib/apt/lists/*

# Download FreeCAD AppImage and extract it in-place.
# --appimage-extract unpacks the embedded squashfs without needing FUSE at runtime.
RUN wget -q https://github.com/FreeCAD/FreeCAD/releases/download/1.0.2/FreeCAD_1.0.2-conda-Linux-x86_64-py311.AppImage \
        -O /tmp/freecad.AppImage && \
    chmod +x /tmp/freecad.AppImage && \
    cd /tmp && /tmp/freecad.AppImage --appimage-extract && \
    mv /tmp/squashfs-root /opt/freecad && \
    rm /tmp/freecad.AppImage

# Wrapper script so 'freecad' is on PATH and AppRun can locate its own APPDIR.
RUN printf '#!/bin/bash\nexport APPDIR=/opt/freecad\nexec /opt/freecad/AppRun "$@"\n' \
        > /usr/local/bin/freecad && \
    chmod +x /usr/local/bin/freecad

# pythonocc-core for STEP → STL conversion. Version 7.7.2 requires Python
# <=3.12, but the base image pins Python 3.13, so install into a separate env.
RUN conda install -n base -c conda-forge mamba --yes && \
    mamba create -n occenv -c conda-forge python=3.12 pythonocc-core=7.7.2 --yes && \
    conda clean --all --yes
