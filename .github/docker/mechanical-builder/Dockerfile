FROM continuumio/miniconda3:latest

# System packages needed at runtime:
#   xvfb, x11vnc - virtual display for FreeCAD (required even in --hidden mode
#                  because the scripts use FreeCADGui / TechDrawGui)
#   openscad     - renders STL files to PNG previews
RUN apt-get update && apt-get install -y \
    xvfb \
    x11vnc \
    openscad \
    && rm -rf /var/lib/apt/lists/*

# Install mamba for a faster solver, then install FreeCAD and pythonocc-core
# together so conda resolves a compatible set of dependencies in one pass.
#   freecad          - replaces the slow snap install; includes all workbenches
#                      (PartDesign, TechDraw, etc.) used by freecad_export.py
#   pythonocc-core   - used by pythonocc_step_to_stl.py for STEP → STL conversion
RUN conda install -n base -c conda-forge mamba --yes && \
    mamba install -n base -c conda-forge \
        freecad \
        pythonocc-core=7.7.2 \
        --yes && \
    conda clean --all --yes
