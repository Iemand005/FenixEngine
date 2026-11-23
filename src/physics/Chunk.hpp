
enum class BlockType : short {
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3,
    Cobblestone = 4,
};

class Chunk {
private:
    BlockType blocks[16][16][256];
public:
    BlockType (*GetBlocks())[16][256] {
        return blocks;
    }
};
