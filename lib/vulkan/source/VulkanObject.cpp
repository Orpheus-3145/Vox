#include "VulkanObject.hpp"
#include "stb_image.h"

#include <iostream>
#include <cassert>


namespace ve {

uint32_t	VulkanObject::currentID = 0U;

void VulkanObject::rotate( vec3 const& axis, float angle ) noexcept
{
	if (this->transformationApplied == false)
		this->transformationApplied = true;

	this->_rotation = quat::product(quat(angle, axis), this->_rotation);	// second factor is applied first, first as last
	this->_rotation.normalize();
}

void VulkanObject::translate( vec3 const& translation ) noexcept
{
	if (this->transformationApplied == false)
		this->transformationApplied = true;

	this->_translation += translation;
}

void VulkanObject::scale( vec3 const& scale ) noexcept
{
	if (this->transformationApplied == false)
		this->transformationApplied = true;

	if (this->uniformScale == true and (scale.x != scale.y or scale.y != scale.z))
		this->uniformScale = false;

	this->_scale.x *= scale.x;
	this->_scale.y *= scale.y;
	this->_scale.z *= scale.z;
}

void VulkanObject::scale( float scale ) noexcept
{
	if (this->transformationApplied == false)
		this->transformationApplied = true;

	this->_scale.x *= scale;
	this->_scale.y *= scale;
	this->_scale.z *= scale;
}

void VulkanObject::bindBuffer(VkCommandBuffer commandBuffer) const noexcept
{
	assert(this->model != nullptr && "Model not set");
	this->model->bindBuffer(commandBuffer);
};

void VulkanObject::draw(VkCommandBuffer commandBuffer) const noexcept
{
	assert(this->model != nullptr && "Model not set");
	this->model->draw(commandBuffer);
};

std::shared_ptr<VulkanModel>	VulkanObject::getModel() const noexcept
{
	assert(this->model != nullptr && "Model not set");
	return this->model;
};

mat4 VulkanObject::getModelMatrix(bool columnMajor) const noexcept
{
	if (this->transformationApplied == false)
	{
		return mat4::idMat();
	}

	// model = T × R × S
	mat4 rotMatrix = this->_rotation.getMatrix();

	if (columnMajor == true)
	{
		return mat4(
			{this->_scale.x * rotMatrix[0][0],  this->_scale.x * rotMatrix[0][1],  this->_scale.x * rotMatrix[0][2],  0.0f},
			{this->_scale.y * rotMatrix[1][0],  this->_scale.y * rotMatrix[1][1],  this->_scale.y * rotMatrix[1][2],  0.0f},
			{this->_scale.z * rotMatrix[2][0],  this->_scale.z * rotMatrix[2][1],  this->_scale.z * rotMatrix[2][2],  0.0f},
			{this->_translation.x,             this->_translation.y,             this->_translation.z,             1.0f}
		);
	}
	else
	{
		return mat4(
			{this->_scale.x * rotMatrix[0][0],  this->_scale.y * rotMatrix[1][0],  this->_scale.z * rotMatrix[2][0],  this->_translation.x},
			{this->_scale.x * rotMatrix[0][1],  this->_scale.y * rotMatrix[1][1],  this->_scale.z * rotMatrix[2][1],  this->_translation.y},
			{this->_scale.x * rotMatrix[0][2],  this->_scale.y * rotMatrix[1][2],  this->_scale.z * rotMatrix[2][2],  this->_translation.z},
			{0.0f,                                      0.0f,                                      0.0f,                                      1.0f}
		);
	}
}

mat4 VulkanObject::getNormalMatrix(bool columnMajor) const noexcept
{
	// normalMatrix = (Model⁻¹)ᵀ = ((T × R × S)⁻¹)ᵀ = (S⁻¹ * R⁻¹ * T⁻¹)ᵀ = (T⁻¹)ᵀ * (R⁻¹)ᵀ * (S⁻¹)ᵀ but:
	// Transpose matrix: since it moves points, doesn't affect the normal so can be removed
	// Rotation matrix: is orthogonal so  R⁻¹ = Rᵀ --> (R⁻¹)ᵀ = (Rᵀ)ᵀ = R
	// Scale matrix: is diagonal (and therefore symmetric) so Sᵀ = S --> (S⁻¹)ᵀ = S⁻¹ also the inverse of a diagonal matrix
	//		is a matrix which elements are the inverse of the elements of the original matrix, finally, if the scaling is uniform
	//		i.e scale.x = scale.y = scale.z the whole matrix can be ignored
	// therefore: (Model⁻¹)ᵀ = R * [S⁻¹]
	if (this->transformationApplied == false)
	{
		return mat4::idMat();
	}

	mat4 normal = this->_rotation.getMatrix();

	if (this->uniformScale == false)
	{
		const float idx = 1.0f / this->_scale.x;
		const float idy = 1.0f / this->_scale.y;
		const float idz = 1.0f / this->_scale.z;

		normal[0][0] *= idx;
		normal[0][1] *= idx;
		normal[0][2] *= idx;
		normal[1][0] *= idy;
		normal[1][1] *= idy;
		normal[1][2] *= idy;
		normal[2][0] *= idz;
		normal[2][1] *= idz;
		normal[2][2] *= idz;
	}

	if (columnMajor == true)
	{
		return normal;
	}
	else
	{
		return normal.transpose();
	}
}

mat4 VulkanObject::getNormalViewMatrix(const mat4& viewNoTranslation, bool columnMajor) const noexcept
{
	// normalMatrix = ((View * Model)⁻¹)ᵀ = ((Model)⁻¹ * (View)⁻¹)ᵀ = (View⁻¹)ᵀ * (Model⁻¹)ᵀ = (View⁻¹)ᵀ * R * [S⁻¹] ( --> see getNormalMatrix() ) but:
	// ViewMatrix: if it has no translation it has only rotation features, so is again orthogonal
	//		so  View⁻¹ = Viewᵀ --> (View⁻¹)ᵀ = (Viewᵀ)ᵀ = View
	// therefore: normalMatrix = ViewNoTrans * R * [S⁻¹]

	if (this->transformationApplied == false)
	{
		return viewNoTranslation;
	}

	mat4 normalView = this->getNormalMatrix(columnMajor) * viewNoTranslation;
	if (columnMajor == true)
	{
		return normalView;
	}
	else
	{
		return normalView.transpose();
	}
}

MeshlayoutDescription VulkanObject::getVboLayout() const noexcept
{
	assert(this->model != nullptr && "Model not set");
	return this->model->getVboLayout();
}

ImageInfo	loadImage(const std::string& imagePath)
{
	ImageInfo	imageInfo = {};

	imageInfo.imageData = stbi_load(imagePath.c_str(), &imageInfo.width, &imageInfo.height, &imageInfo.channels, STBI_rgb_alpha);
	if (imageInfo.imageData == nullptr)
	{
		throw std::runtime_error("Failed to load image: " + imagePath);
	}
	std::cout << "Loaded image: " << imagePath << " (" << imageInfo.width << "x" << imageInfo.height << ", " << imageInfo.channels << " channels)" << std::endl;
	return imageInfo;
}

}	// namespace ve