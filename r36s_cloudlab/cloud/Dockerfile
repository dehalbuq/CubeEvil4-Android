# Build with: docker buildx build --platform linux/arm64 -t r36s-cloudlab -f cloud/Dockerfile .
# Use host CPU emulation (QEMU) or native ARM64. No real Mali GPU is emulated.
FROM --platform=linux/arm64 debian:trixie-slim
ENV DEBIAN_FRONTEND=noninteractive EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    clang lld cmake ninja-build build-essential python3 pkg-config \
    libegl-dev libgles-dev libgl1-mesa-dri libsdl2-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /lab
COPY . /lab/r36s_cloudlab
CMD ["sh", "r36s_cloudlab/cloud/run-cloud-tests.sh"]
