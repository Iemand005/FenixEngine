#!/bin/bash
# Exit immediately if any command fails
set -e

echo "🚀 Initializing and updating git submodules..."
git submodule update --init --recursive

echo "🔄 Updating package lists..."
sudo apt update

echo "📦 Installing system build tools, SDL3 dependencies, and Vulkan SDK..."
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libasound2-dev \
    libpulse-dev \
    libvulkan-dev \
    vulkan-tools \
    mesa-vulkan-drivers
sudo apt-get update && sudo apt-get install -y libvulkan-dev vulkan-tools mesa-vulkan-drivers libvulkan1


echo "✅ Environment setup complete! You are ready to build FenixEngine."
