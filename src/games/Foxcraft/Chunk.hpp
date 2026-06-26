
enum class BlockType : short {
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3,
    Cobblestone = 4,
};

class Chunk {
private:
    std::vector<std::vector<std::vector<BlockType>>> blocks;

public:
    Chunk() : blocks(16, std::vector<std::vector<BlockType>>(128, std::vector<BlockType>(16, BlockType::Air))) {}

    BlockType GetBlock(int x, int y, int z) const {
        return blocks[x][y][z];
    }

    void SetBlock(int x, int y, int z, BlockType type) {
        blocks[x][y][z] = type;
    }
};
