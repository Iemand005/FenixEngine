
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
    std::unique_ptr<BlockType[16][256][16]> blocks;

public:
    Chunk() : blocks(new BlockType[16][256][16]()) {
        std::memset(blocks.get(), 0, 16 * 256 * 16 * sizeof(BlockType));
    }

    BlockType GetBlock(int x, int y, int z) const {
        return (*blocks)[x][y][z];
    }

    void SetBlock(int x, int y, int z, BlockType type) {
        (*blocks)[x][y][z] = type;
    }
};
