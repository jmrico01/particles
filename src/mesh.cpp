#include "mesh.h"

#include <stdio.h>
#include <map>

#include "km_math.h"
#include "km_debug.h"
#include "opengl_funcs.h"
#include "ogl_base.h"

#define OBJ_LINE_MAX 512

struct Vertex
{
    uint32 halfEdge;
    Vec3 pos;
    Vec3 normal;
    Vec3 color;
    float32 avgEdgeLength;
};

struct Face
{
    uint32 halfEdge;
    Vec3 normal;
    float32 area;
};

struct HalfEdge
{
    uint32 next;
    uint32 twin;
    uint32 vertex;
    uint32 face;
};

struct HalfEdgeMesh
{
    DynamicArray<Vertex> vertices;
    DynamicArray<Face> faces;
    DynamicArray<HalfEdge> halfEdges;
};

internal int GetNextLine(const char* src, char* dst, int dstLen)
{
    int read = 0;
    const char* s = src;
    while (*s != '\n' && *s != '\0' && read < dstLen - 1) {
        dst[read++] = *(s++);
    }
    dst[read] = '\0';

    if (read == 0 && *s == '\0') {
        return -1;
    }
    return read;
}

internal Vec3 ParseVec3(char* str)
{
    Vec3 result = { 0.0f, 0.0f, 0.0f };
    char* el = str;
    int i = 0;
    for (char* c = str; *c != '\0'; c++) {
        if (*c == ' ' || *c == '\n') {
            char* endptr = el;
            *c = '\0';
            result.e[i++] = (float32)strtod(el, &endptr);
            if (i == 3) {
                break;
            }
            // TODO: check error
            el = c + 1;
        }
    }

    return result;
}

internal Vec2 ParseVec2(char* str)
{
    Vec2 result = { 0.0f, 0.0f };
    char* el = str;
    while (*el == ' ') {
        el++;
    }
    int i = 0;
    for (char* c = str; *c != '\0'; c++) {
        if (*c == ' ' || *c == '\n') {
            char* endptr = el;
            *c = '\0';
            result.e[i++] = (float32)strtod(el, &endptr);
            if (i == 2) {
                break;
            }
            // TODO: check error
            el = c + 1;
        }
    }

    return result;
}

internal Vec3 NormalFromTriangle(Vec3 v0, Vec3 v1, Vec3 v2)
{
    Vec3 a = v1 - v0;
    Vec3 b = v2 - v0;
    return Normalize(Cross(a, b));
}
internal Vec3 NormalFromTriangle(const Vec3 triangle[3])
{
    Vec3 a = triangle[1] - triangle[0];
    Vec3 b = triangle[2] - triangle[0];
    return Normalize(Cross(a, b));
}
internal void FacesOnVertex(const HalfEdgeMesh& mesh, uint32 v,
    DynamicArray<uint32>& out)
{
    uint32 edge = mesh.vertices[v].halfEdge;
    uint32 e = edge;
    do {
        out.Append(mesh.halfEdges[e].face);
        uint32 twin = mesh.halfEdges[e].twin;
        e = mesh.halfEdges[twin].next;
    } while (e != edge);
}

// Computes normal for the face f, assuming all vertices in f are coplanar.
internal Vec3 ComputeFaceNormal(const HalfEdgeMesh& mesh, uint32 f)
{
    Vec3 triangle[3];
    int numEdges = 0;
    uint32 edge = mesh.faces[f].halfEdge;
    uint32 e = edge;
    do {
        triangle[numEdges++] = mesh.vertices[mesh.halfEdges[e].vertex].pos;
        if (numEdges >= 3) {
            break;
        }
        e = mesh.halfEdges[e].next;
    } while (e != edge);

    return NormalFromTriangle(triangle);
}
internal void ComputeFaceNormals(HalfEdgeMesh* mesh)
{
    for (uint32 f = 0; f < mesh->faces.size; f++) {
        Vec3 normal = ComputeFaceNormal((const HalfEdgeMesh&)*mesh, f);
        mesh->faces[f].normal = normal;
    }
}
internal void ComputeVertexNormals(HalfEdgeMesh* mesh)
{
    DynamicArray<uint32> faces;
    faces.Init();
    for (uint32 v = 0; v < mesh->vertices.size; v++) {
        Vec3 sumNormals = Vec3::zero;
        FacesOnVertex((const HalfEdgeMesh&)*mesh, v, faces);
        for (uint32 i = 0; i < faces.size; i++) {
            sumNormals += mesh->faces[faces[i]].normal;
        }
        mesh->vertices[v].normal = Normalize(sumNormals);

        faces.Clear();
    }

    faces.Free();
}

internal HalfEdgeMesh HalfEdgeMeshFromObj(const ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    HalfEdgeMesh mesh;
    mesh.vertices.Init();
    mesh.faces.Init();
    mesh.halfEdges.Init();

    DEBUGReadFileResult objFile = DEBUGPlatformReadFile(thread, fileName);
    if (!objFile.data) {
        DEBUG_PRINT("Failed to open OBJ file at: %s\n", fileName);
        return mesh;
    }

    char* fileStr = (char*)objFile.data;
    int read;
    char line[OBJ_LINE_MAX];
    DynamicArray<int> faceInds;
    faceInds.Init();

    while ((read = GetNextLine(fileStr, line, OBJ_LINE_MAX)) >= 0) {
        fileStr += read + 1;
        // TODO: I'm too lazy to fix this
        line[read] = '\n';
        line[read + 1] = '\0';
        //DEBUG_PRINT("line: %s", line);
        if (line[0] == '#') {
            continue;
        }
        else if (line[0] == 'v') {
            if (line[1] == ' ') {
                Vertex vertex;
                vertex.pos = ParseVec3(&line[2]);
                vertex.halfEdge = 0; // This is set later
                vertex.color = Vec3::one;
                mesh.vertices.Append(vertex);
            }
            else {
                // idk
            }
        }
        else if (line[0] == 'f') {
            char* el = &line[2];
            for (char* c = &line[2]; *c != '\0'; c++) {
                if (*c == ' ' || *c == '\n') {
                    char* endptr = el;
                    *c = '\0';
                    faceInds.Append((int)strtol(el, &endptr, 10));
                    // TODO check error
                    el = c + 1;
                }
            }
            faceInds.Append(-1);
        }
        else {
            // idk
        }
    }

    std::map<std::pair<uint32, uint32>, uint32> edgeMap;

    uint32 faceVerts = 0;
    for (uint32 i = 0; i < faceInds.size; i++) {
        if (faceInds[i] == -1 || i == faceInds.size - 1) {
            // Create face
            Face face;
            face.halfEdge = mesh.halfEdges.size - 1;
            mesh.faces.Append(face);
            
            faceVerts = 0;
            continue;
        }

        uint32 vertSrc = faceInds[i] - 1;
        uint32 vertDst = faceInds[i + 1] - 1;
        if (faceInds[i + 1] == -1) {
            vertDst = faceInds[i - faceVerts] - 1;
        }

        // Create edge, i -> i + 1
        // Last edge will have wrong "next"
        std::pair<int, int> forward(vertSrc, vertDst);
        std::pair<int, int> backward(vertDst, vertSrc);
        HalfEdge he;
        he.next = mesh.halfEdges.size + 1;
        if (faceInds[i + 1] == -1) {
            he.next = mesh.halfEdges.size - faceVerts;
        }
        auto edgeBackward = edgeMap.find(backward);
        if (edgeBackward != edgeMap.end()) {
            he.twin = edgeBackward->second;
            mesh.halfEdges[edgeBackward->second].twin =
                mesh.halfEdges.size;
        }
        else {
            he.twin = 0;
        }
        he.vertex = vertDst;
        he.face = mesh.faces.size;

        // Assign forward edge i -> i + 1 to vertex i
        mesh.vertices[vertSrc].halfEdge = mesh.halfEdges.size;

        // Add edge to edgeMap and mesh.halfEdges array
        auto res = edgeMap.insert(
            std::make_pair(forward, mesh.halfEdges.size));
        if (!res.second) {
            DEBUG_PRINT("ERROR: Edge already in edgeMap\n");
        }
        mesh.halfEdges.Append(he);
        faceVerts++;
    }

    faceInds.Free();
    ComputeFaceNormals(&mesh);
    ComputeVertexNormals(&mesh);

    DEBUGPlatformFreeFileMemory(thread, &objFile);
    
    return mesh;
}

internal void FreeHalfEdgeMesh(HalfEdgeMesh* mesh)
{
    mesh->vertices.Free();
    mesh->faces.Free();
    mesh->halfEdges.Free();
}

internal inline float32 ComputeTriangleArea(
    Vec3 v0, Vec3 v1, Vec3 v2)
{
    Vec3 v0v1 = v1 - v0;
    Vec3 v0v2 = v2 - v0;
    return Mag(Cross(v0v1, v0v2)) / 2.0f;
}

Mesh LoadMeshFromObj(const ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    Mesh mesh;
    mesh.triangles.Init();

    DEBUGReadFileResult objFile = DEBUGPlatformReadFile(thread, fileName);
    if (!objFile.data) {
        DEBUG_PRINT("Failed to open OBJ file at: %s\n", fileName);
        return mesh;
    }

    char* fileStr = (char*)objFile.data;
    int read;
    char line[OBJ_LINE_MAX];
    DynamicArray<Vec3> vertices;
    vertices.Init();
    DynamicArray<Vec2> uvs;
    uvs.Init();
    DynamicArray<Vec3> normals;
    normals.Init();

    DynamicArray<int> faceVertInds;
    faceVertInds.Init();
    DynamicArray<int> faceUVInds;
    faceUVInds.Init();
    DynamicArray<int> faceNormInds;
    faceNormInds.Init();

    while ((read = GetNextLine(fileStr, line, OBJ_LINE_MAX)) >= 0) {
        fileStr += read + 1;
        // TODO: I'm too lazy to fix this
        line[read] = '\n';
        line[read + 1] = '\0';
        //DEBUG_PRINT("line: %s", line);
        if (line[0] == '#') {
            continue;
        }
        else if (line[0] == 'v') {
            if (line[1] == ' ') {
                int start = 2;
                while (line[start] == ' ') {
                    start++;
                }
                Vec3 v = ParseVec3(&line[start]);
                vertices.Append(v);
            }
            else if (line[1] == 't') {
                int start = 3;
                while (line[start] == ' ') {
                    start++;
                }
                Vec2 uv = ParseVec2(&line[start]);
                // TODO: uvs are flipped vertically here
                // BMPs should probably be flipped vertically instead
                uv.y = 1.0f - uv.y;
                uvs.Append(uv);
            }
            else if (line[1] == 'n') {
                int start = 3;
                while (line[start] == ' ') {
                    start++;
                }
                Vec3 n = ParseVec3(&line[start]);
                normals.Append(Normalize(n));
            }
            else {
                // idk
            }
        }
        else if (line[0] == 'f') {
            char* vert = &line[2];
            char* uv = &line[2];
            char* norm = &line[2];
            for (char* c = &line[2]; *c != '\0'; c++) {
                if (*c == '/') {
                    if (uv == vert) {
                        uv = c + 1;
                        *c = '\0';
                    }
                    else if (norm == vert) {
                        norm = c + 1;
                        *c = '\0';
                    }
                    else {
                        // idk
                    }
                }
                else if (*c == ' ' || *c == '\n') {
                    *c = '\0';
                    char* endptr = vert; // for strtol
                    faceVertInds.Append((int)strtol(vert, &endptr, 10) - 1);
                    // TODO: check error
                    if (uv != vert) {
                        endptr = uv;
                        faceUVInds.Append((int)strtol(uv, &endptr, 10) - 1);
                    }
                    if (norm != vert) {
                        endptr = norm;
                        faceNormInds.Append((int)strtol(norm, &endptr, 10) - 1);
                    }

                    vert = c + 1;
                    uv = c + 1;
                    norm = c + 1;
                }
            }
            faceVertInds.Append(-1);
            faceUVInds.Append(-1);
            faceNormInds.Append(-1);
        }
        else {
            // idk
        }
    }

    bool computedVertexNormals = false;
    if (normals.size == 0) {
        computedVertexNormals = true;
        HalfEdgeMesh halfEdgeMesh = HalfEdgeMeshFromObj(thread, fileName,
            DEBUGPlatformReadFile,
            DEBUGPlatformFreeFileMemory);
        DEBUG_ASSERT(halfEdgeMesh.vertices.size == vertices.size);
        for (int v = 0; v < (int)halfEdgeMesh.vertices.size; v++) {
            normals.Append(halfEdgeMesh.vertices[v].normal);
        }
        FreeHalfEdgeMesh(&halfEdgeMesh);
    }

    DynamicArray<int> faceVerts;
    faceVerts.Init();
    DynamicArray<int> faceUVs;
    faceUVs.Init();
    DynamicArray<int> faceNormals;
    faceNormals.Init();
    DEBUG_ASSERT(vertices.size != uvs.size
        || faceVertInds.size == faceUVInds.size);
    /*DEBUG_ASSERT(vertices.size != normals.size
        || faceVertInds.size == faceNormInds.size);*/
    for (int i = 0; i < (int)faceVertInds.size; i++) {
        if (faceVertInds[i] == -1) {
            DEBUG_ASSERT(uvs.size == 0 || faceUVInds[i] == -1);
            //DEBUG_ASSERT(normals.size == 0 || faceNormInds[i] == -1);
            DEBUG_ASSERT(faceVerts.size >= 3);
            Vec3 flatNormal = NormalFromTriangle(
                vertices[faceVerts[0]],
                vertices[faceVerts[1]],
                vertices[faceVerts[2]]
            );
            for (int v = 1; v < (int)faceVerts.size - 1; v++) {
                Vec3 v1 = vertices[faceVerts[v]];
                Vec3 v2 = vertices[faceVerts[v + 1]];
                Triangle triangle;
                triangle.v[0] = vertices[faceVerts[0]];
                triangle.v[1] = vertices[faceVerts[v]];
                triangle.v[2] = vertices[faceVerts[v + 1]];
                if (uvs.size > 0) {
                    triangle.uv[0] = uvs[faceUVs[0]];
                    triangle.uv[1] = uvs[faceUVs[v]];
                    triangle.uv[2] = uvs[faceUVs[v + 1]];
                }
                else {
                    triangle.uv[0] = Vec2::zero;
                    triangle.uv[1] = Vec2::zero;
                    triangle.uv[2] = Vec2::zero;
                }
                if (!computedVertexNormals) {
                    triangle.n[0] = normals[faceNormals[0]];
                    triangle.n[1] = normals[faceNormals[v]];
                    triangle.n[2] = normals[faceNormals[v + 1]];
                }
                else {
                    // triangle.n[0] = normals[faceVerts[0]];
                    // triangle.n[1] = normals[faceVerts[v]];
                    // triangle.n[2] = normals[faceVerts[v + 1]];
                    triangle.n[0] = flatNormal;
                    triangle.n[1] = flatNormal;
                    triangle.n[2] = flatNormal;
                }
                triangle.area = ComputeTriangleArea(
                    triangle.v[0], triangle.v[1], triangle.v[2]);
                mesh.triangles.Append(triangle);
            }
            faceVerts.Clear();
            faceUVs.Clear();
            faceNormals.Clear();
        }
        else {
            faceVerts.Append(faceVertInds[i]);
            if (uvs.size > 0) {
                faceUVs.Append(faceUVInds[i]);
            }
            if (!computedVertexNormals) {
                faceNormals.Append(faceNormInds[i]);
            }
        }
    }
    faceVerts.Free();
    faceNormals.Free();
    faceUVs.Free();

    faceVertInds.Free();
    faceUVInds.Free();
    faceNormInds.Free();

    vertices.Free();
    uvs.Free();
    normals.Free();

    DEBUGPlatformFreeFileMemory(thread, &objFile);

    // NOTE: must free mesh after this
    return mesh;
}

void FreeMesh(Mesh* mesh)
{
    mesh->triangles.Free();
}

MeshGL LoadMeshGL(const ThreadContext* thread, const Mesh& mesh,
    DEBUGPlatformReadFileFunc DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc DEBUGPlatformFreeFileMemory)
{
    MeshGL meshGL;

    DynamicArray<Vec3> vertices;
    vertices.Init(mesh.triangles.size * 3);
    DynamicArray<Vec3> normals;
    normals.Init(mesh.triangles.size * 3);
    for (uint32 t = 0; t < mesh.triangles.size; t++) {
        vertices.Append(mesh.triangles[t].v[0]);
        vertices.Append(mesh.triangles[t].v[1]);
        vertices.Append(mesh.triangles[t].v[2]);

        normals.Append(mesh.triangles[t].n[0]);
        normals.Append(mesh.triangles[t].n[1]);
        normals.Append(mesh.triangles[t].n[2]);
    }

    glGenVertexArrays(1, &meshGL.vertexArray);
    glBindVertexArray(meshGL.vertexArray);

    glGenBuffers(1, &meshGL.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, meshGL.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size * sizeof(Vec3),
        vertices.data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glGenBuffers(1, &meshGL.normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, meshGL.normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size * sizeof(Vec3),
        normals.data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glBindVertexArray(0);

    meshGL.programID = LoadShaders(thread,
        "shaders/model.vert",
        "shaders/model.frag",
        DEBUGPlatformReadFile,
        DEBUGPlatformFreeFileMemory);

    meshGL.vertexCount = (int)vertices.size;

    vertices.Free();
    normals.Free();

    return meshGL;
}

void FreeMeshGL(MeshGL* meshGL)
{
    glDeleteBuffers(1, &meshGL->vertexBuffer);
    glDeleteBuffers(1, &meshGL->normalBuffer);
    glDeleteProgram(meshGL->programID);
    glDeleteVertexArrays(1, &meshGL->vertexArray);
}

void DrawMeshGL(const MeshGL& meshGL, Mat4 proj, Mat4 view, Vec4 color)
{
    GLint loc;
    glUseProgram(meshGL.programID);
    
    Mat4 model = Mat4::one;
    Mat4 mvp = proj * view * model;
    loc = glGetUniformLocation(meshGL.programID, "mvp");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp.e[0][0]);
    loc = glGetUniformLocation(meshGL.programID, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &model.e[0][0]);
    loc = glGetUniformLocation(meshGL.programID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &view.e[0][0]);
    loc = glGetUniformLocation(meshGL.programID, "color");
    glUniform4fv(loc, 1, &color.e[0]);

    glBindVertexArray(meshGL.vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, meshGL.vertexCount);
    glBindVertexArray(0);
}
