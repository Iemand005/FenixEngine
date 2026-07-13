class VulkanDevice : public RenderDevice {
public:
    VertexBuffer* CreateVertexBuffer(void* data, size_t size) override {
        return new VulkanVertexBuffer(data, size); // Uses vkCreateBuffer, vkBindBufferMemory
    }
};