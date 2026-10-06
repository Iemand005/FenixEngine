#!/bin/bash
# Exit immediately if any command fails
set -e

echo "🚀 Initializing and updating git submodules..."
git submodule update --init --recursive

echo "🔄 Updating package lists..."
sudo apt update

echo "📦 Installing system build tools, SDL3 dependencies, and Vulkan SDK..."
sudo apt-get update
sudo apt install -y \
    build-essential cmake pkg-config \
    libasound2-dev libpulse-dev \
    libvulkan-dev vulkan-tools mesa-vulkan-drivers libvulkan1 \
    libwayland-dev wayland-protocols extra-cmake-modules \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libxkbcommon-dev




echo "✅ Environment setup complete! You are ready to build FenixEngine."
