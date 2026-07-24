#include "World.hpp"


namespace vox {

VertexVector getVertexRelative( vec3 const& relativeOrigin )
{
	VertexVector voxelVertexes(ve::VERTEX_PER_VOXEL);
	for (ui32 i=0; i<ve::VERTEX_PER_VOXEL; i++)
	{
		voxelVertexes[i].pos.x = VOXEL_VERTEXES[i].pos.x + relativeOrigin.x;
		voxelVertexes[i].pos.y = VOXEL_VERTEXES[i].pos.y + relativeOrigin.y;
		voxelVertexes[i].pos.z = VOXEL_VERTEXES[i].pos.z + relativeOrigin.z;
		voxelVertexes[i].normal = VOXEL_VERTEXES[i].normal;
		voxelVertexes[i].textureUv = VOXEL_VERTEXES[i].textureUv;
		voxelVertexes[i].textureIndex = VOXEL_VERTEXES[i].textureIndex;
	}
	return voxelVertexes;
}

VertexVector getVertexAtlasRelative( vec3 const& relativeOrigin )
{
	VertexVector voxelVertexes(ve::VERTEX_PER_VOXEL);
	for (ui32 i=0; i<ve::VERTEX_PER_VOXEL; i++)
	{
		voxelVertexes[i].pos.x = VOXEL_VERTEXES_ATLAS[i].pos.x + relativeOrigin.x;
		voxelVertexes[i].pos.y = VOXEL_VERTEXES_ATLAS[i].pos.y + relativeOrigin.y;
		voxelVertexes[i].pos.z = VOXEL_VERTEXES_ATLAS[i].pos.z + relativeOrigin.z;
		voxelVertexes[i].normal = VOXEL_VERTEXES_ATLAS[i].normal;
		voxelVertexes[i].textureUv = VOXEL_VERTEXES_ATLAS[i].textureUv;
		voxelVertexes[i].textureIndex = VOXEL_VERTEXES_ATLAS[i].textureIndex;
	}
	return voxelVertexes;
}

IndexVector getIndexRelative( ui32 start )
{
	IndexVector indexes(ve::INDEX_PER_VOXEL);

	for (ui32 i=0; i<ve::INDEX_PER_VOXEL; i++)
	{
		indexes[i] = start + ve::VOXEL_INDEXES[i];
	}
	return indexes;
}

std::unique_ptr<ve::VulkanModel> createVoxelModel( ve::VulkanDevice& vulkanDevice, vec3 const& relativePos )
{
	return std::make_unique<ve::VulkanModel>(vulkanDevice, getVertexRelative(relativePos), getIndexRelative(), 0U, ve::DEFAULT_MODEL_LAYOUT);
}

std::unique_ptr<ve::VulkanModel> createVoxelAtlasModel( ve::VulkanDevice& vulkanDevice, vec3 const& relativePos )
{
	return std::make_unique<ve::VulkanModel>(vulkanDevice, getVertexAtlasRelative(relativePos), getIndexRelative(), 0U, ve::ONLY_VERTEX_LAYOUT);
}

}	// namespace vox
