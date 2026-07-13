void uploadMesh(Mesh<VertexType>& mesh) {
        auto glBuffers = std::make_unique<OpenGLGPUBuffers>();

        glGenVertexArrays(1, &glBuffers->vao);
        glGenBuffers(1, &glBuffers->vbo);
        glGenBuffers(1, &glBuffers->ebo);

        glBindVertexArray(glBuffers->vao);

        // Vertex Buffer vullen
        glBindBuffer(GL_ARRAY_BUFFER, glBuffers->vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(VertexType), mesh.vertices.data(), GL_STATIC_DRAW);

        // Index Buffer vullen
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glBuffers->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

        // DYNAMISCHE ATTRIBUTEN CONFIGURATIE
        int vertexStride = sizeof(VertexType);
        auto layout = VertexType::getLayout(); // Haalt de layout op van de specifieke struct

        for (const auto& attr : layout) {
            glVertexAttribPointer(
                attr.location, 
                attr.components, // Dit wordt nu dynamisch 2 of 3!
                GL_FLOAT, 
                GL_FALSE, 
                vertexStride, 
                (void*)attr.offset // De exacte byte-offset via offsetof
            );
            glEnableVertexAttribArray(attr.location);
        }

        glBindVertexArray(0); // Unbind

        // Geef buffers aan de mesh
        mesh.gpuBuffers = std::move(glBuffers);
    }