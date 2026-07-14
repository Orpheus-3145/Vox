#include "World.hpp"


namespace vox {

std::vector<ve::VulkanModel::Vertex> getVertexRelative( vec3 const& relativeOrigin ) {
	std::vector<ve::VulkanModel::Vertex> voxelVertexes(VERTEX_PER_VOXEL);
	for (ui32 i=0; i<VERTEX_PER_VOXEL; i++) {
		// add 0.5 (half size of a voxel) of every coor so that the position is in the exact center
		voxelVertexes[i].pos.x = VOXEL_VERTEXES[i].pos.x + VOXEL_SIZE * 0.5f + relativeOrigin.x;
		voxelVertexes[i].pos.y = VOXEL_VERTEXES[i].pos.y + VOXEL_SIZE * 0.5f + relativeOrigin.y;
		voxelVertexes[i].pos.z = VOXEL_VERTEXES[i].pos.z + VOXEL_SIZE * 0.5f + relativeOrigin.z;
		voxelVertexes[i].normal = VOXEL_VERTEXES[i].normal;
		voxelVertexes[i].textureUv = VOXEL_VERTEXES[i].textureUv;
	}
	return voxelVertexes;
}

std::vector<ve::VulkanModel::Vertex> getVertexAtlasRelative( vec3 const& relativeOrigin ) {
	std::vector<ve::VulkanModel::Vertex> voxelVertexes(VERTEX_PER_VOXEL);
	for (ui32 i=0; i<VERTEX_PER_VOXEL; i++) {
		// add 0.5 (half size of a voxel) of every coor so that the position is in the exact center
		voxelVertexes[i].pos.x = VOXEL_VERTEXES_ATLAS[i].pos.x + relativeOrigin.x;
		voxelVertexes[i].pos.y = VOXEL_VERTEXES_ATLAS[i].pos.y + relativeOrigin.y;
		voxelVertexes[i].pos.z = VOXEL_VERTEXES_ATLAS[i].pos.z + relativeOrigin.z;
		voxelVertexes[i].normal = VOXEL_VERTEXES_ATLAS[i].normal;
		voxelVertexes[i].textureUv = VOXEL_VERTEXES_ATLAS[i].textureUv;
	}
	return voxelVertexes;
}

IndexVector getIndexRelative( ui32 start ) {
	IndexVector indexes(INDEX_PER_VOXEL);

	for (ui32 i=0; i<INDEX_PER_VOXEL; i++)
	{
		indexes[i] = start + VOXEL_VERTEX_INDEXES[i];
	}
	return indexes;
}

std::unique_ptr<ve::VulkanModel> createVoxelModel( ve::VulkanDevice& vulkanDevice, vec3 const& relativePos )
{
	return std::make_unique<ve::VulkanModel>(vulkanDevice, getVertexRelative(relativePos), getIndexRelative());
}

std::unique_ptr<ve::VulkanModel> createVoxelAtlasModel( ve::VulkanDevice& vulkanDevice, vec3 const& relativePos )
{
	return std::make_unique<ve::VulkanModel>(vulkanDevice, getVertexAtlasRelative(relativePos), getIndexRelative(), 0U, ve::MeshLayout::VERTEX);
}

}	// namespace vox
