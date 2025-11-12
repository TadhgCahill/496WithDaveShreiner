#include "sceneBuilderClass.hpp"
#include <iostream>
#include <random>
using namespace std;

static void checkGLErrOnce(const char* where) {
    for (GLenum e = glGetError(); e != GL_NO_ERROR; e = glGetError()) {
        std::cerr << "[GL ERR] " << "\n";
        break; // only print first to avoid spam
    }
}

static GLuint compileShader_(GLenum type, const char* src) { //jut for compute rn
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        string log(len, '\0'); glGetShaderInfoLog(sh, len, nullptr, log.data());
        cerr << "Shader compile failed:\n" << log << endl;
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint linkProgram_(GLuint cs) { //attatch compute shader and program
    GLuint prog = glCreateProgram();
    glAttachShader(prog, cs);
    glLinkProgram(prog);
    GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        string log(len, '\0'); glGetProgramInfoLog(prog, len, nullptr, log.data());
        cerr << "Program link failed:\n" << log << endl;
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

//initialize window and camera
sceneBuilderClass::sceneBuilderClass() {
    windowInit(1280, 720, "Instanced Scene");
    setCamera(60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);

    //for compute shader
    buildCullProgram_();
    // Allocate UBOs (frustum + aabb)
    glGenBuffers(1, &uboFrustum_);
    glBindBuffer(GL_UNIFORM_BUFFER, uboFrustum_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 6, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboFrustum_); // binding=4 in shader

    glGenBuffers(1, &uboAabb_);
    glBindBuffer(GL_UNIFORM_BUFFER, uboAabb_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 2, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 5, uboAabb_); // binding=5 in shader

    glm::vec4 packInit[2] = { glm::vec4(aabbMinOS_, 0.0f), glm::vec4(aabbMaxOS_, 0.0f) };
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(packInit), packInit);

}

GLFWwindow* sceneBuilderClass::windowInit(int width, int height, string name) {
    if (!glfwInit()) { cerr << "GLFW init failed\n"; return nullptr; }

    // Compute shaders need OpenGL 4.3+
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (!window) { cerr << "GLFW window creation failed\n"; return nullptr; }
    glfwMakeContextCurrent(window);

    // Important for GLEW with core profile
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { cerr << "GLEW init failed\n"; return nullptr; }

    glEnable(GL_DEPTH_TEST);
    return window;
}

void sceneBuilderClass::setCamera(float fovDeg, float aspect, float zNear, float zFar) {
    projection = glm::perspective(glm::radians(fovDeg), aspect, zNear, zFar);
}

void sceneBuilderClass::cameraRotate(glm::mat4& view){
    const float radius = 10.0f;
    float camX = sin(glfwGetTime()) * radius;
    float camZ = cos(glfwGetTime()) * radius;
    view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
}

//add generic model
void sceneBuilderClass::addObject(const shared_ptr<ModelObject>& obj) {
    objects_.push_back(obj);
}

//should this be with models or is vector<glm::mat4>& fine?
void sceneBuilderClass::setInstanceTransforms(const vector<glm::mat4>& mats) {
    allInstances_ = mats;
    maxInstances_ = static_cast<GLsizei>(allInstances_.size());

    // Create/resize SSBOs for inputs/outputs
    if (!ssboMatrices_) glGenBuffers(1, &ssboMatrices_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboMatrices_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * maxInstances_, allInstances_.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboMatrices_); // binding=0

    if (!ssboVisible_) glGenBuffers(1, &ssboVisible_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVisible_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * maxInstances_, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboVisible_); // binding=2

    if (!ssboCounter_) glGenBuffers(1, &ssboCounter_);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ssboCounter_);
    GLuint zero = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, ssboCounter_); // binding=3

    // Also forward the *full* instance list to your models initially (render path will compact per frame)
    for (auto& obj : objects_) {
        obj->setInstanceTransforms(allInstances_);
    }
}

void sceneBuilderClass::bindCameraPointers() {
    for (auto& o : objects_) {
        if (auto* m = dynamic_cast<ModelObject*>(o.get())) {
            m->bindCamera(&view, &projection);
        }
    }
}

void sceneBuilderClass::run() {
    if (!window) return;

    // --- one-time setup / state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    // Ensure base bindings are set
    if (uboFrustum_) glBindBufferBase(GL_UNIFORM_BUFFER,        4, uboFrustum_);
    if (uboAabb_)    glBindBufferBase(GL_UNIFORM_BUFFER,        5, uboAabb_);
    if (ssboMatrices_) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboMatrices_);
    if (ssboVisible_)  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboVisible_);
    if (ssboCounter_)  glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, ssboCounter_);

    // Cached workgroup count (recompute if maxInstances_ changes)
    auto groupsPerDispatch = [this]() -> GLuint {
        return maxInstances_ ? (GLuint)((maxInstances_ + 127) / 128) : 0u;
    };
    GLuint cachedGroups = groupsPerDispatch();

    // Debug toggles
    bool disableCulling = false;           // press 'C' to toggle
    double startTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Toggle culling with 'C'
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            static bool latch = false;
            if (!latch) { disableCulling = !disableCulling; latch = true; }
        } else {
            static bool latch = true; latch = false;
        }

        // If instance count changed elsewhere, refresh groups
        GLuint neededGroups = groupsPerDispatch();
        if (neededGroups != cachedGroups) cachedGroups = neededGroups;

        // camera default
        if (glm::length(glm::vec3(view[3])) == 0.0f) {
            view = glm::lookAt(glm::vec3(0.0f, 4.0f, 400.0f),
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
        }

        bindCameraPointers();// dont need anywhere else

        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update frustum planes each frame
        glm::vec4 planes[6];
        updateFrustumPlanes_(planes);
        if (uboFrustum_) {
            glBindBuffer(GL_UNIFORM_BUFFER, uboFrustum_);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(planes), planes);
        }

        std::vector<glm::mat4> matsToDraw;

        if (!disableCulling && cullProgram_ && maxInstances_ > 0 && cachedGroups > 0) {
            // Reset counter
            if (ssboCounter_) {
                GLuint zero = 0;
                glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ssboCounter_);
                glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
                glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, ssboCounter_);
            }

            // Bind bases (harmless if already bound)
            if (ssboMatrices_) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboMatrices_);
            if (ssboVisible_)  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboVisible_);
            if (uboFrustum_)   glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboFrustum_);
            if (uboAabb_)      glBindBufferBase(GL_UNIFORM_BUFFER, 5, uboAabb_);

            glUseProgram(cullProgram_);
            glDispatchCompute(cachedGroups, 1, 1);

            // Ensure the writes are visible to subsequent CPU reads and draw
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                            GL_ATOMIC_COUNTER_BARRIER_BIT  |
                            GL_BUFFER_UPDATE_BARRIER_BIT);

            // Read visible count
            GLuint visCount = 0;
            if (ssboCounter_) {
                glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ssboCounter_);
                glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &visCount);
            }

            // Debug print for first few secs
            if (glfwGetTime() - startTime < 4.0) {
                std::cerr << "[dbg] cullProgram=" << (int)(cullProgram_ != 0)
                          << " maxInstances=" << maxInstances_
                          << " groups=" << cachedGroups
                          << " visible=" << visCount
                          << " culling=" << (!disableCulling) << "\n";
                checkGLErrOnce("after compute");
            }

        } else {
            // Culling disabled or compute unavailable → draw everything
            matsToDraw = allInstances_;
            if (glfwGetTime() - startTime < 4.0) {
                std::cerr << "[dbg] culling disabled or cullProgram==0, drawing all instances ("
                          << matsToDraw.size() << ")\n";
            }
        }

        // Draw objects
        for (auto& obj : objects_) {
            obj->setInstanceTransforms(matsToDraw); // uploads per-instance buffer
            obj->render();
        }

        glfwSwapBuffers(window);
    }
}

void sceneBuilderClass::buildCullProgram_() {
    // One thread per instance. Using AABB culling derived from object-space AABB.
    // We transform center & extents with |M3x3| for a tight world-space AABB proxy.
    static const char* kCullCS = R"(#version 430
layout(local_size_x = 128) in;

// Inputs
layout(std430, binding = 0) readonly buffer Matrices { mat4 worldMats[]; };

// Outputs
layout(std430, binding = 2) writeonly buffer Visible { uint visibleIndices[]; };
layout(binding = 3, offset = 0) uniform atomic_uint visCount;

// Frustum planes
layout(std140, binding = 4) uniform Frustum {
    vec4 planes[6]; // n.xyz, d
};

// Object-space AABB for the STL mesh used by all instances
layout(std140, binding = 5) uniform AABB {
    vec4 aabbMinOS; // xyz + 0
    vec4 aabbMaxOS; // xyz + 0
};

bool aabbInFrustum(mat4 M, vec3 minOS, vec3 maxOS) {
    // Compute world-space center & extents of the OBB projected to an AABB
    vec3 centerOS = 0.5 * (minOS + maxOS);
    vec3 extentOS = 0.5 * (maxOS - minOS);

    // World transform
    vec3 centerWS = (M * vec4(centerOS, 1.0)).xyz;

    // 3x3 linear part
    mat3 A = mat3(M);
    mat3 absA = mat3(
        abs(A[0][0]), abs(A[0][1]), abs(A[0][2]),
        abs(A[1][0]), abs(A[1][1]), abs(A[1][2]),
        abs(A[2][0]), abs(A[2][1]), abs(A[2][2])
    );

    vec3 extentWS = absA * extentOS;

    // Plane-slab test: reject if box is completely outside any plane
    for (int i = 0; i < 6; ++i) {
        vec3 n = planes[i].xyz;
        float d = planes[i].w;

        // Signed distance of center to plane
        float s = dot(n, centerWS) + d;

        // Projected radius of AABB onto plane normal
        vec3 an = vec3(abs(n.x), abs(n.y), abs(n.z));
        float r = dot(an, extentWS);

        if (s < -r) return false; // outside
    }
    return true; // inside or intersects
}

void main() {
    uint idx = gl_GlobalInvocationID.x;

    // Optional: bounds check in case dispatch is rounded up
    if (idx >= worldMats.length()) return;

    vec3 minOS = aabbMinOS.xyz;
    vec3 maxOS = aabbMaxOS.xyz;

    if (aabbInFrustum(worldMats[idx], minOS, maxOS)) {
        uint outIdx = atomicCounterIncrement(visCount);
        visibleIndices[outIdx] = idx;
    }
}
)";

    GLuint cs = compileShader_(GL_COMPUTE_SHADER, kCullCS);
    cullProgram_ = linkProgram_(cs);
    glDeleteShader(cs);

    // Storage buffers are created on demand in setInstanceTransforms()
    if (!cullProgram_) {
        std::cerr << "[compute] link failed; disabling GPU culling this run.\n";
    }
}

void sceneBuilderClass::updateFrustumPlanes_(glm::vec4 planes[6]) const {
    // Extract planes from VP = projection * view (clip space), classic method
    glm::mat4 VP = projection * view;

    auto r0 = glm::row(VP, 0);
    auto r1 = glm::row(VP, 1);
    auto r2 = glm::row(VP, 2);
    auto r3 = glm::row(VP, 3);

    glm::vec4 eq[6] = {
        r3 + r0,  // left
        r3 - r0,  // right
        r3 + r1,  // bottom
        r3 - r1,  // top
        r3 + r2,  // near
        r3 - r2   // far
    };

    // normalize planes
    for (int i = 0; i < 6; ++i) {
        glm::vec3 n = glm::vec3(eq[i]);
        float invLen = 1.0f / glm::length(n);
        planes[i] = eq[i] * invLen;
    }
}

vector<glm::mat4> sceneBuilderClass::makeInstanceTransforms(
    size_t count,
    const string& layout,
    float spacing,
    float radius,
    const glm::vec3& boxMin,
    const glm::vec3& boxMax)
{
    vector<glm::mat4> mats;
    mats.reserve(count);
    if (count == 0) return mats;

    
    mt19937 rng(12345u);
    uniform_real_distribution<float> jitter(-0.25f, 0.25f);
    uniform_real_distribution<float> scaleJit(0.90f, 1.10f);
    uniform_real_distribution<float> yawJit(-glm::pi<float>(), glm::pi<float>());
    uniform_real_distribution<float> u01(0.0f, 1.0f);
    uniform_real_distribution<float> rx(boxMin.x, boxMax.x);
    uniform_real_distribution<float> ry(boxMin.y, boxMax.y);
    uniform_real_distribution<float> rz(boxMin.z, boxMax.z);

    auto makeTRS = [&](const glm::vec3& p, float yaw, const glm::vec3& s) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, p);
        // Yaw-only random rotation (keeps instances upright)
        M = glm::rotate(M, yaw, glm::vec3(0, 1, 0));
        M = glm::scale(M, s);
        return M;
    };

    if (layout == "grid") {
        // 3D grid, centered around origin
        const int side = static_cast<int>(ceil(pow(double(count), 1.0/3.0)));
        size_t emitted = 0;
        for (int z = 0; z < side && emitted < count; ++z) {
            for (int y = 0; y < side && emitted < count; ++y) {
                for (int x = 0; x < side && emitted < count; ++x) {
                    glm::vec3 pos(
                        (x - side/2) * spacing + jitter(rng),
                        (y - side/2) * spacing + jitter(rng),
                        (z - side/2) * spacing + jitter(rng));
                    glm::vec3 sc(scaleJit(rng), scaleJit(rng), scaleJit(rng));
                    float yaw = yawJit(rng) * 0.1f; // small, keep grid readable
                    mats.push_back(makeTRS(pos, yaw, sc));
                    ++emitted;
                }
            }
        }
    }
    
    else {
        // Fallback to grid if unknown layout keyword
        const int side = static_cast<int>(ceil(pow(double(count), 1.0/3.0)));
        size_t emitted = 0;
        for (int z = 0; z < side && emitted < count; ++z) {
            for (int y = 0; y < side && emitted < count; ++y) {
                for (int x = 0; x < side && emitted < count; ++x) {
                    glm::vec3 pos(
                        (x - side/2) * spacing + jitter(rng),
                        (y - side/2) * spacing + jitter(rng),
                        (z - side/2) * spacing + jitter(rng));
                    glm::vec3 sc(scaleJit(rng), scaleJit(rng), scaleJit(rng));
                    float yaw = yawJit(rng) * 0.1f;
                    mats.push_back(makeTRS(pos, yaw, sc));
                    ++emitted;
                }
            }
        }
    }

    return mats;
}

void sceneBuilderClass::setModelBounds(const glm::vec3& minOS, const glm::vec3& maxOS) {
    aabbMinOS_ = minOS;
    aabbMaxOS_ = maxOS;
    hasModelBounds_ = true;

    // Write to UBO now so compute shader reads valid data
    glm::vec4 pack[2] = { glm::vec4(aabbMinOS_, 0.0f), glm::vec4(aabbMaxOS_, 0.0f) };
    glBindBuffer(GL_UNIFORM_BUFFER, uboAabb_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(pack), pack);
    glBindBufferBase(GL_UNIFORM_BUFFER, 5, uboAabb_); // ensure bound
}
