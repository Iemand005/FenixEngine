class RenderDevice {
public:
    virtual ~RenderDevice() = default;
	virtual void Init() = 0;
    // virtual VertexBuffer* CreateVertexBuffer(void* data, size_t size) = 0;
    // virtual Texture* CreateTexture(const std::string& path) = 0;
    // virtual void DrawIndexed(uint32_t indexCount) = 0;
    virtual void SubmitFrame() = 0;
};