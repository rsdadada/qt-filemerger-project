FROM ubuntu:22.04

# Avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential build tools, git, and Qt6 development libraries
# qt6-base-dev should include core, gui, widgets, testlib
# We add qt6-svg-dev explicitly as SVG components were identified by windeployqt
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    git \
    qt6-base-dev \
    qt6-tools-dev \
    libqt6svg6-dev \
    # Clean up apt cache to reduce image size
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set the working directory in the container
WORKDIR /app

# Copy the application source code into the container
# This line is useful if you want to build the image with a "frozen" version of the code.
# For active development, mounting a volume (explained in README) is often preferred.
COPY . /app/

# Default command when the container starts (provides a shell for interactive use)
CMD ["bash"] 