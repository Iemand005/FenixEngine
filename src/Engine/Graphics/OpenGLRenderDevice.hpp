
#pragma once

#include "IRenderDevice.hpp"

class OpenGLRenderDevice : public IRenderDevice {
	// void uploadMesh(Mesh<VertexType>& mesh) {
    //     auto glBuffers = std::make_unique<OpenGLGPUBuffers>();

    //     glGenVertexArrays(1, &glBuffers->vao);
    //     glGenBuffers(1, &glBuffers->vbo);
    //     glGenBuffers(1, &glBuffers->ebo);

    //     glBindVertexArray(glBuffers->vao);

    //     glBindBuffer(GL_ARRAY_BUFFER, glBuffers->vbo);
    //     glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(VertexType), mesh.vertices.data(), GL_STATIC_DRAW);

    //     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glBuffers->ebo);
    //     glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

    //     int vertexStride = sizeof(VertexType);
    //     auto layout = VertexType::getLayout();

    //     for (const auto& attr : layout) {
    //         glVertexAttribPointer(
    //             attr.location, 
    //             attr.components,
    //             GL_FLOAT, 
    //             GL_FALSE, 
    //             vertexStride, 
    //             (void*)attr.offset
    //         );
    //         glEnableVertexAttribArray(attr.location);
    //     }

    //     glBindVertexArray(0);

    //     mesh.gpuBuffers = std::move(glBuffers);
    // }

    void Init(fe::IWindow *window) override {

    }



    void DrawMesh(const fe::Mesh<>& mesh) override {
        if (mesh.gpuBuffers) {
            mesh.gpuBuffers->bind();
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        }
    }

    void SubmitFrame() override {

    }
};