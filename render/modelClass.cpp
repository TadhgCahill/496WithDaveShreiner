#include "modelClass.hpp"
using namespace std;

/*---------------------------vectorClass--------------------------------- */
vertexClass::vertexClass(){// sort out view and projection after
    vertexShaderString = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

/* Per-instance transform (mat4 uses 4 locations) */
layout (location = 2) in mat4 iModel;

out vec3 vNormal;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat3 normalMat = mat3(transpose(inverse(iModel)));
    vNormal = normalMat * aNormal;
    gl_Position = projection * view * iModel * vec4(aPos, 1.0);
}
)";
}

void vertexClass::importer(string filename){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filename,
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices
    );

    aiMesh* mesh = scene->mMeshes[0]; // assume one mesh

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * 6); // pos + normal
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D p = mesh->mVertices[i];
        aiVector3D n = mesh->mNormals[i];
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    }

    
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout: position (0), normal (1)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void vertexClass::modelRotate(glm::mat4& model){
    float t = static_cast<float>(glfwGetTime());
    model = glm::rotate(model, t, glm::vec3(0.0f, 1.0f, 0.0f)); // t is radians/sec*time; fine for a spin
}

/*---------------------------fragmentClass--------------------------------- */

fragmentClass::fragmentClass(){
    fragShaderString = R"(#version 330 core
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 0.6, 0.2, 1.0); // orange
    }
    )";
}

/*#version 330 core
in vec3 vNormal;
out vec4 FragColor;

void main()
{
    // simple lambert-ish look
    vec3 n = normalize(vNormal);
    float k = 0.5 + 0.5 * n.z;
    FragColor = vec4(vec3(k), 1.0);
}
*/

