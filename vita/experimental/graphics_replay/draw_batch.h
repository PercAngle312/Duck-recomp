#pragma once
#include <cstddef>
constexpr size_t DuckMaxTriangles=1024,DuckMaxVertexFloats=64;
inline bool duck_vertex_batch_valid(size_t floats,size_t length,size_t triangles){
 return floats>0&&floats<=DuckMaxVertexFloats&&triangles>0&&triangles<=DuckMaxTriangles&&length==triangles*3*floats;
}
