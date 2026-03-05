#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class JPH::DebugRendererSimple;

class BasicDebugRenderer : public JPH::DebugRendererSimple
{
public:
    struct DebugVertex
    {
        glm::vec3 position;
        glm::vec4 color;
    };

    struct RenderBatch
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        std::vector<DebugVertex> vertices;
        GLenum drawMode = GL_TRIANGLES;
    };

    BasicDebugRenderer()
    {
        InitializeShaders();
        InitializeBatches();
    }

    virtual ~BasicDebugRenderer()
    {
        glDeleteProgram(shaderProgram);
        for (auto& batch : batches)
        {
            glDeleteVertexArrays(1, &batch.second.vao);
            glDeleteBuffers(1, &batch.second.vbo);
        }
    }

    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
    {
        auto& batch = batches[GL_LINES];
        
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
        auto& batch = batches[GL_TRIANGLES];
        
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
        // 简单实现：将文本顶点添加到线条批次中
        // 注意：完整的文本渲染需要字体纹理，这里只绘制简单的线框字符
        auto& batch = batches[GL_LINES];
        glm::vec3 pos(inPosition.GetX(), inPosition.GetY(), inPosition.GetZ());
        glm::vec4 color(inColor.r, inColor.g, inColor.b, inColor.a);
        
        // 这里只是一个占位实现
        // 实际应用中，您可能需要使用freetype库或预渲染的字形
        DrawSimpleString(batch, pos, std::string(inString), color, inHeight);
    }

    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
    {
        glUseProgram(shaderProgram);
        
        // 设置uniform
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        
        // 渲染每个批次
        for (auto& pair : batches)
        {
            auto& batch = pair.second;
            if (batch.vertices.empty())
                continue;
            
            glBindVertexArray(batch.vao);
            glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
            glBufferData(GL_ARRAY_BUFFER, batch.vertices.size() * sizeof(DebugVertex), 
                        batch.vertices.data(), GL_STREAM_DRAW);
            
            glDrawArrays(batch.drawMode, 0, static_cast<GLsizei>(batch.vertices.size()));
        }
        
        // 清除顶点数据以备下一帧使用
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
    GLuint shaderProgram = 0;
    std::map<GLenum, RenderBatch> batches;

    void InitializeShaders()
    {
        const char* vertexShaderSource = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec4 aColor;
            
            uniform mat4 uView;
            uniform mat4 uProjection;
            
            out vec4 vColor;
            
            void main()
            {
                vColor = aColor;
                gl_Position = uProjection * uView * vec4(aPos, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            in vec4 vColor;
            out vec4 FragColor;
            
            void main()
            {
                FragColor = vColor;
            }
        )";

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void InitializeBatches()
    {
        // 初始化线条批次
        batches[GL_LINES].drawMode = GL_LINES;
        SetupVAO(batches[GL_LINES]);
        
        // 初始化三角形批次
        batches[GL_TRIANGLES].drawMode = GL_TRIANGLES;
        SetupVAO(batches[GL_TRIANGLES]);
    }

    void SetupVAO(RenderBatch& batch)
    {
        glGenVertexArrays(1, &batch.vao);
        glGenBuffers(1, &batch.vbo);
        
        glBindVertexArray(batch.vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), 
                             (void*)offsetof(DebugVertex, position));
        glEnableVertexAttribArray(0);
        
        // 颜色属性
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), 
                             (void*)offsetof(DebugVertex, color));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void DrawSimpleString(RenderBatch& batch, const glm::vec3& pos, 
                         const std::string& text, const glm::vec4& color, float height)
    {
        // 简单的线框字符渲染（仅用于演示）
        // 实际项目中应该使用更完善的文本渲染系统
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
        // 绘制字符的简单线框表示（这里只绘制'A'作为示例）
        glm::vec3 bottomLeft = pos;
        glm::vec3 bottomRight = pos + glm::vec3(width, 0, 0);
        glm::vec3 topLeft = pos + glm::vec3(0, height, 0);
        glm::vec3 topRight = pos + glm::vec3(width, height, 0);
        glm::vec3 middleLeft = pos + glm::vec3(0, height * 0.5f, 0);
        glm::vec3 middleRight = pos + glm::vec3(width, height * 0.5f, 0);
        
        // 绘制三角形轮廓
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
};