
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

    BlockType GetBlock(glm::vec3 pos) const {
        return GetBlock(pos.x, pos.y, pos.z);
    }

    void SetBlock(int x, int y, int z, BlockType type) {
        blocks[x * HEIGHT * DEPTH + y * DEPTH + z] = type;
    }

    glm::vec3 GetOffsetAt(glm::vec3 pos, fe::PlaneDirection diection) {
        switch (diection) {

        }
    }

    bool NeedsFace(glm::vec3 pos, fe::PlaneDirection diection) {
        return GetBlock(GetOffsetAt(pos, diection));
    }
};
