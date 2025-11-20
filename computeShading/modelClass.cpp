#include "modelClass.hpp"
using namespace std;
//contructor from just name of file
ModelObject::ModelObject(const string& meshPath) {
    loadMesh(meshPath);
    uploadMesh();

    GLuint vs = compile(GL_VERTEX_SHADER, kDefaultVS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kDefaultFS);
    program_ = link(vs, fs);

    uView_ = glGetUniformLocation(program_, "view");
    uProj_ = glGetUniformLocation(program_, "projection");
}

static void checkCompile(GLuint sh, GLenum type) {
    GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(sh, 2048, nullptr, log);
        cerr << "Shader compile error (" << (type==GL_VERTEX_SHADER?"VS":"FS") << "):\n" << log << "\n";
        throw runtime_error("Shader compile failed");
    }
}
static void checkLink(GLuint prog) {
    GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(prog, 2048, nullptr, log);
        cerr << "Program link error:\n" << log << "\n";
        throw runtime_error("Program link failed");
    }
}

//basic vertex
const char* ModelObject::kDefaultVS = R"(#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Per-instance data comes from SSBOs, not vertex attributes
layout(std430, binding = 0) readonly buffer Matrices {
    mat4 worldMats[];
};

layout(std430, binding = 2) readonly buffer Visible {
    uint visibleIndices[];
};

out vec3 vNormal;

uniform mat4 view;
uniform mat4 projection;

void main() {
    uint inst     = uint(gl_InstanceID);
    uint matIndex = visibleIndices[inst];   // index into worldMats
    mat4 iModel   = worldMats[matIndex];

    vNormal    = mat3(transpose(inverse(iModel))) * aNormal;
    gl_Position = projection * view * iModel * vec4(aPos, 1.0);
}
)";


//basic frag with shading
const char* ModelObject::kDefaultFS = R"(#version 430 core
in vec3 vNormal;
out vec4 FragColor;
void main() {
    vec3 n = normalize(vNormal);
    float k = 0.5 + 0.5 * n.z;
    FragColor = vec4(vec3(k), 1.0);
}
)";


//works or frag and vertex
GLuint ModelObject::compile(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    checkCompile(sh, type);
    return sh;
}

//links shader to program
GLuint ModelObject::link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    checkLink(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

//importer
void ModelObject::loadMesh(const string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes
    );
    if (!scene || !scene->mNumMeshes) {
        throw runtime_error("Assimp failed to load: " + path);
    }
    const aiMesh* m = scene->mMeshes[0];
    interleaved_.reserve(m->mNumVertices * 6);
    for (unsigned i = 0; i < m->mNumVertices; ++i) {
        const aiVector3D& p = m->mVertices[i];
        const aiVector3D& n = m->mNormals[i];

        // === NEW: update bounds ===
        bboxMin_.x = min(bboxMin_.x, p.x);
        bboxMin_.y = min(bboxMin_.y, p.y);
        bboxMin_.z = min(bboxMin_.z, p.z);
        bboxMax_.x = max(bboxMax_.x, p.x);
        bboxMax_.y = max(bboxMax_.y, p.y);
        bboxMax_.z = max(bboxMax_.z, p.z);

        interleaved_.insert(end(interleaved_), {p.x,p.y,p.z, n.x,n.y,n.z});
    }
    indices_.reserve(m->mNumFaces * 3);
    for (unsigned f = 0; f < m->mNumFaces; ++f) {
        const aiFace& face = m->mFaces[f];
        if (face.mNumIndices == 3) {
            indices_.push_back(face.mIndices[0]);
            indices_.push_back(face.mIndices[1]);
            indices_.push_back(face.mIndices[2]);
        }
    }
}

//send mesh to gpu
void ModelObject::uploadMesh() {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, interleaved_.size() * sizeof(float), interleaved_.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned), indices_.data(), GL_STATIC_DRAW);

    // pos (0), normal (1)
    const GLsizei stride = sizeof(float) * 6;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float)*3));

    glBindVertexArray(0);
}

void ModelObject::setupInstanceBuffer() {
    if (!instanceVbo_) glGenBuffers(1, &instanceVbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);
    glBufferData(GL_ARRAY_BUFFER, instanceMats_.size() * sizeof(glm::mat4), instanceMats_.data(), GL_DYNAMIC_DRAW);

    // mat4 takes 4 attribute locations (2..5)
    constexpr GLuint baseLoc = 2;
    constexpr GLsizei vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(baseLoc + i);
        const uintptr_t offset = static_cast<uintptr_t>(i) * static_cast<uintptr_t>(vec4Size);
        glVertexAttribPointer(baseLoc + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              reinterpret_cast<void*>(offset));
        glVertexAttribDivisor(baseLoc + i, 1);
    }
    glBindVertexArray(0);

    instanceCount_ = static_cast<GLsizei>(instanceMats_.size());
}

//destructor
ModelObject::~ModelObject() {
    if (program_)     glDeleteProgram(program_);
    if (instanceVbo_) glDeleteBuffers(1, &instanceVbo_);
    if (ebo_)         glDeleteBuffers(1, &ebo_);
    if (vbo_)         glDeleteBuffers(1, &vbo_);
    if (vao_)         glDeleteVertexArrays(1, &vao_);
}

//gets camera for render
void ModelObject::bindCamera(const glm::mat4* viewPtr, const glm::mat4* projPtr) {
    view_ = viewPtr;
    proj_ = projPtr;
}


void ModelObject::setInstanceTransforms(const vector<glm::mat4>& transforms) {
    instanceMats_ = transforms;
    if (instanceMats_.empty()) {
        instanceMats_.push_back(glm::mat4(1.0f)); // ensure at least one
    }
    setupInstanceBuffer();
}

void ModelObject::render() {
    // We now rely on the indirect command buffer, not instanceCount_
    if (!program_ || !vao_ || !view_ || !proj_ || indirectBuffer_ == 0) return;

    glUseProgram(program_);
    glUniformMatrix4fv(uView_, 1, GL_FALSE, glm::value_ptr(*view_));
    glUniformMatrix4fv(uProj_, 1, GL_FALSE, glm::value_ptr(*proj_));

    glBindVertexArray(vao_);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer_);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ModelObject::setVisibleCount(GLuint visibleCount) {
    if (indirectBuffer_ == 0) {
        return; // initIndirect not called yet
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer_);

    // Only update the instanceCount field inside the command
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER,
                    offsetof(DrawElementsIndirectCommand, instanceCount),
                    sizeof(GLuint),
                    &visibleCount);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void ModelObject::initIndirect(GLsizei maxInstances) {
    // Create the indirect draw buffer if it doesn't exist yet
    if (indirectBuffer_ == 0) {
        glGenBuffers(1, &indirectBuffer_);
    }

    // Set up the initial draw command.
    // We draw all indices in the mesh, but with instanceCount = 0
    // until culling tells us how many instances are visible.
    DrawElementsIndirectCommand cmd{};
    cmd.count         = static_cast<GLuint>(indices_.size());  // indices per instance
    cmd.instanceCount = 0;                                     // will be set via setVisibleCount()
    cmd.firstIndex    = 0;                                     // EBO starts at 0
    cmd.baseVertex    = 0;                                     // VBO starts at 0
    cmd.baseInstance  = 0;                                     // first instance ID

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer_);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
                 sizeof(DrawElementsIndirectCommand),
                 &cmd,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    (void)maxInstances; // currently unused, avoids a warning
}
