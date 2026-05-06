#include "World.hpp"


namespace vox {

/**
 * Get the the vertexes of every face of the voxel. Voxel has 6 faces, every face is made 
 * by 2 triangles, so a total of 24 vertexes, drawing the triangles CW order
 *
 * @param relativeOrigin the 3D position of the Voxel (default is (0, 0, 0))
 *
 * @return a vector of 24 vec3
 */
std::vector<vec3> getVertexRelative( vec3 const& relativeOrigin ) {
	std::vector<vec3> voxelVertexes(VERTEX_PER_VOXEL);
	for (ui32 i=0; i<VERTEX_PER_VOXEL; i++) {
		// add 0.5 (half size of a voxel) of every coor so that the position is in the exact center
		voxelVertexes[i].x = VOXEL_VERTEXES_ATLAS[i].pos.x + VOXEL_SIZE * 0.5f + relativeOrigin.x;
		voxelVertexes[i].y = VOXEL_VERTEXES_ATLAS[i].pos.y + VOXEL_SIZE * 0.5f + relativeOrigin.y;
		voxelVertexes[i].z = VOXEL_VERTEXES_ATLAS[i].pos.z + VOXEL_SIZE * 0.5f + relativeOrigin.z;
	}
	return voxelVertexes;
}

// std::vector<ve::VulkanModel::Vertex> getVertexRelative( vec3 const& relativeOrigin ) {
// 	std::vector<ve::VulkanModel::Vertex> voxelVertexes(VERTEX_PER_VOXEL);
// 	for (ui32 i=0; i<VERTEX_PER_VOXEL; i++) {
// 		// add 0.5 (half size of a voxel) of every coor so that the position is in the exact center
// 		voxelVertexes[i].pos.x = VOXEL_VERTEXES[i].pos.x + VOXEL_SIZE * 0.5f + relativeOrigin.x;
// 		voxelVertexes[i].pos.y = VOXEL_VERTEXES[i].pos.y + VOXEL_SIZE * 0.5f + relativeOrigin.y;
// 		voxelVertexes[i].pos.z = VOXEL_VERTEXES[i].pos.z + VOXEL_SIZE * 0.5f + relativeOrigin.z;
// 		voxelVertexes[i].normal = VOXEL_VERTEXES[i].normal;
// 		voxelVertexes[i].textureUv = VOXEL_VERTEXES[i].textureUv;
// 	}
// 	return voxelVertexes;
// }

std::vector<ve::VulkanModel::Vertex> getVertexAtlasRelative( vec3 const& relativeOrigin ) {
	std::vector<ve::VulkanModel::Vertex> voxelVertexes(VERTEX_PER_VOXEL);
	for (ui32 i=0; i<VERTEX_PER_VOXEL; i++) {
		// add 0.5 (half size of a voxel) of every coor so that the position is in the exact center
		voxelVertexes[i].pos.x = VOXEL_VERTEXES_ATLAS[i].pos.x + VOXEL_SIZE * 0.5f + relativeOrigin.x;
		voxelVertexes[i].pos.y = VOXEL_VERTEXES_ATLAS[i].pos.y + VOXEL_SIZE * 0.5f + relativeOrigin.y;
		voxelVertexes[i].pos.z = VOXEL_VERTEXES_ATLAS[i].pos.z + VOXEL_SIZE * 0.5f + relativeOrigin.z;
		voxelVertexes[i].normal = VOXEL_VERTEXES_ATLAS[i].normal;
		voxelVertexes[i].textureUv = VOXEL_VERTEXES_ATLAS[i].textureUv;
	}
	return voxelVertexes;
}

/**
 * Get the the indexes of every face of the voxel. Voxel has 6 faces, every face is made 
 * by 2 triangles, so a total of 36 indexes, drawing the triangles CW order
 *
 * @param start the starting value of the face indexes (default is 0)
 *
 * @return a vector of 36 uin32_t starting from the offset value
 */
IndexVector getIndexRelative( ui32 start ) {
	IndexVector indexes(INDEX_PER_VOXEL);

	for (ui32 i=0; i<INDEX_PER_VOXEL; i++)
		indexes[i] = start + VOXEL_VERTEX_INDEXES[i];
	return indexes;
}

}	// namespace vox
