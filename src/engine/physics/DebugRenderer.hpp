// Example header
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

class DebugRenderer : public JPH::DebugRendererSimple
{
public:
    DebugRenderer();
    virtual ~DebugRenderer();

    // Override all five pure virtual functions
    void DrawLine(JPH::RVec3Arg inFrom, ...) override {

    };
    void DrawTriangle(JPH::RVec3Arg inV1, ...) override;
    Batch CreateTriangleBatch(const Triangle *inTriangles, ...) override;
    Batch CreateTriangleBatch(const Vertex *inVertices, ...) override;
    void DrawGeometry(JPH::RMat44Arg inModelMatrix, ...) override;
    void DrawText3D(JPH::RVec3Arg inPosition, ...) override;

    void RenderAllBatches(); // Your function to flush batched data
private:
    // Manage your OpenGL buffers, shaders, and batch data here
};