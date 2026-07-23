#pragma once

#include <vector>
#include <string>
#include <map>

#include <glm/glm.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include "../Graphics/IRenderDevice.hpp"

class BasicDebugRenderer : public JPH::DebugRendererSimple
{
public:
    static bool& DebugRenderingEnabled()
    {
        static bool enabled = false;
        return enabled;
    }

    struct DebugVertex
    {
        glm::vec3 position;
        glm::vec4 color;
    };

    struct RenderBatch
    {
        std::vector<DebugVertex> vertices;
        int drawMode = 0;
    };

    BasicDebugRenderer(fe::IRenderDevice* device)
        : device_(device)
    {
        batches[0].drawMode = 0;
        batches[1].drawMode = 1;
    }

    virtual ~BasicDebugRenderer() = default;

    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
    {
        auto& batch = batches[0];

        DebugVertex v1, v2;
        v1.position = glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        v2.position = glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ());

        v1.color = glm::vec4(inColor.r, inColor.g, inColor.b, inColor.a);
        v2.color = v1.color;

        batch.vertices.push_back(v1);
        batch.vertices.push_back(v2);
    }

    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
                              JPH::ColorArg inColor, ECastShadow inCastShadow) override
    {
        auto& batch = batches[1];

        DebugVertex v1, v2, v3;
        v1.position = glm::vec3(inV1.GetX(), inV1.GetY(), inV1.GetZ());
        v2.position = glm::vec3(inV2.GetX(), inV2.GetY(), inV2.GetZ());
        v3.position = glm::vec3(inV3.GetX(), inV3.GetY(), inV3.GetZ());

        glm::vec4 color(inColor.r, inColor.g, inColor.b, inColor.a);
        v1.color = color;
        v2.color = color;
        v3.color = color;

        batch.vertices.push_back(v1);
        batch.vertices.push_back(v2);
        batch.vertices.push_back(v3);
    }

    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString,
                            JPH::ColorArg inColor, float inHeight) override
    {
        auto& batch = batches[0];
        glm::vec3 pos(inPosition.GetX(), inPosition.GetY(), inPosition.GetZ());
        glm::vec4 color(inColor.r, inColor.g, inColor.b, inColor.a);
        DrawSimpleString(batch, pos, std::string(inString), color, inHeight);
    }

    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
    {
        if (!DebugRenderingEnabled()) {
            Clear();
            return;
        }

        for (auto& pair : batches)
        {
            auto& batch = pair.second;
            if (batch.vertices.empty())
                continue;

            if (batch.drawMode == 0) {
                device_->DrawDebugLines(
                    reinterpret_cast<const float*>(batch.vertices.data()),
                    static_cast<int>(batch.vertices.size()),
                    viewMatrix, projectionMatrix);
            } else {
                device_->DrawDebugTriangles(
                    reinterpret_cast<const float*>(batch.vertices.data()),
                    static_cast<int>(batch.vertices.size()),
                    viewMatrix, projectionMatrix);
            }
        }

        Clear();
    }

    void Clear()
    {
        for (auto& pair : batches)
        {
            pair.second.vertices.clear();
        }
    }

private:
    fe::IRenderDevice* device_ = nullptr;
    std::map<int, RenderBatch> batches;

    void DrawSimpleString(RenderBatch& batch, const glm::vec3& pos,
                          const std::string& text, const glm::vec4& color, float height)
    {
        float charWidth = height * 0.5f;
        float spacing = charWidth * 0.2f;

        for (size_t i = 0; i < text.length(); ++i)
        {
            glm::vec3 charPos = pos + glm::vec3(i * (charWidth + spacing), 0, 0);
            DrawWireframeChar(batch, charPos, text[i], color, charWidth, height);
        }
    }

    void DrawWireframeChar(RenderBatch& batch, const glm::vec3& pos, char c,
                           const glm::vec4& color, float width, float height)
    {
        glm::vec3 bottomLeft = pos;
        glm::vec3 bottomRight = pos + glm::vec3(width, 0, 0);
        glm::vec3 topLeft = pos + glm::vec3(0, height, 0);
        glm::vec3 topRight = pos + glm::vec3(width, height, 0);
        glm::vec3 middleLeft = pos + glm::vec3(0, height * 0.5f, 0);
        glm::vec3 middleRight = pos + glm::vec3(width, height * 0.5f, 0);

        AddLine(batch, bottomLeft, bottomRight, color);
        AddLine(batch, bottomLeft, topLeft, color);
        AddLine(batch, topLeft, topRight, color);
        AddLine(batch, middleLeft, middleRight, color);
        AddLine(batch, topLeft, bottomRight, color);
    }

    void AddLine(RenderBatch& batch, const glm::vec3& from, const glm::vec3& to,
                 const glm::vec4& color)
    {
        DebugVertex v1, v2;
        v1.position = from;
        v2.position = to;
        v1.color = color;
        v2.color = color;
        batch.vertices.push_back(v1);
        batch.vertices.push_back(v2);
    }

public:
    static void DrawImGuiToggle(const char* label)
    {
    }
};
