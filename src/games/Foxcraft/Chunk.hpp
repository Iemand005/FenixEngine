
#pragma once

#include <memory>

enum class BlockType : short {
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3,
    Cobblestone = 4,
};

class Chunk {
private:
    std::vector<BlockType> blocks;
    static constexpr int WIDTH = 16, HEIGHT = 20, DEPTH = 16;

public:
    Chunk() : blocks(WIDTH * HEIGHT * DEPTH, BlockType::Air) {}

    BlockType GetBlock(int x, int y, int z) const {
        return blocks[x * HEIGHT * DEPTH + y * DEPTH + z];
    }

    void SetBlock(int x, int y, int z, BlockType type) {
        blocks[x * HEIGHT * DEPTH + y * DEPTH + z] = type;
    }

    bool NeedsFace(fe::PlaneDirection diection) {
        switch (diection) {

        }
    }
};
