
enum class BlockType : short {
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3,
    Cobblestone = 4,
};

class Chunk {
private:
    BlockType blocks[16][128][16];
public:
    BlockType (&GetBlocks())[16][128][16] {
        return blocks;
    }
};
