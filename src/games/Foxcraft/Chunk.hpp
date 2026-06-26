
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

    glm::vec3 GetOffsetAt(glm::vec3 pos, fe::PlaneDirection direction) {
        switch (direction) {
            case fe::PlaneDirection::Front:
                return pos + glm::vec3(0, 0, 1);
            case fe::PlaneDirection::Back:
                return pos + glm::vec3(0, 0, -1);
            case fe::PlaneDirection::Right:
                return pos + glm::vec3(1, 0, 0);
            case fe::PlaneDirection::Left:
                return pos + glm::vec3(-1, 0, 0);
            case fe::PlaneDirection::Top:
                return pos + glm::vec3(0, 1, 0);
            case fe::PlaneDirection::Bottom:
                return pos + glm::vec3(0, -1, 0);
        }
        return pos;
    }

    bool NeedsFace(glm::vec3 pos, fe::PlaneDirection direction) {
        glm::vec3 offsetPos = GetOffsetAt(pos, direction);
        BlockType neighbor = GetBlock(offsetPos);
        return neighbor == BlockType::Air;
    }

    fe::Mesh GenerateMesh() {

    }
};
