
#pragma once

#include <memory>

#include <Primitives.hpp>

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
        if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH)
            return BlockType::Air;
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
        fe::Mesh mesh;

        for(int x = 0; x < WIDTH; x++) {
            for(int y = 0; y < HEIGHT; y++) {
                for(int z = 0; z < DEPTH; z++) {
                    BlockType block = GetBlock(x, y, z);

                    if(block == BlockType::Air) continue;

                    std::vector<fe::PlaneDirection> visibleFaces;
                    for(auto direction : {fe::PlaneDirection::Front, fe::PlaneDirection::Back,
                        fe::PlaneDirection::Left, fe::PlaneDirection::Right,
                        fe::PlaneDirection::Top, fe::PlaneDirection::Bottom}) {
                        if(NeedsFace(glm::vec3(x, y, z), direction)) {
                            visibleFaces.push_back(direction);
                        }
                        }

                        if(!visibleFaces.empty()) {
                            fe::Mesh cubeMesh = fe::Primitives::GenerateCube(visibleFaces, 1.0f);

                            for(auto& vertex : cubeMesh.vertices) {
                                vertex.position += glm::vec3(x, y, z);
                            }

                            int indexOffset = mesh.vertices.size();
                            mesh.vertices.insert(mesh.vertices.end(),
                                                 cubeMesh.vertices.begin(),
                                                 cubeMesh.vertices.end());

                            for(auto index : cubeMesh.indices) {
                                mesh.indices.push_back(index + indexOffset);
                            }
                        }
                }
            }
        }

        return mesh;
    }
};
