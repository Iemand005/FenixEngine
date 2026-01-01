#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <glad/glad.h>
#include <vector>

class WireframeDebugRenderer : public JPH::DebugRendererSimple
{
public:
    WireframeDebugRenderer() { Initialize(); }
    
    void Draw(const float* viewProj)
    {
        if (mLineVertices.empty()) return;
        
        // Minimal OpenGL rendering
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        
        // Simple line rendering (modern OpenGL minimal)
        static GLuint vao = 0, vbo = 0;
        static bool initialized = false;
        
        if (!initialized) {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(0);
            initialized = true;
        }
        
        // Upload and draw
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 
                    mLineVertices.size() * sizeof(Vertex),
                    mLineVertices.data(), GL_DYNAMIC_DRAW);
        
        // Use your existing shader or fixed-function
        // (Assuming you have a basic shader bound)
        glDrawArrays(GL_LINES, 0, mLineVertices.size());
        
        // Clear for next frame
        mLineVertices.clear();
    }
};