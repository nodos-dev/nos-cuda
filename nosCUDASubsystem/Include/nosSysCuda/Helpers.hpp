// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#pragma once

#include "nosCUDASubsystem.h"
#include "nosSysCuda/Types_generated.h"
#include <Nodos/Plugin.hpp>

extern nosEngineServices nosEngine;

namespace nos
{
	NOS_DEFINE_FOREIGN_STATIC_OBJECT_TYPE_INFO(nos::sys::cuda::Buffer,
		nos::sys::cuda::Buffer::GetFullyQualifiedName());
	NOS_DEFINE_FOREIGN_STATIC_OBJECT_TYPE_INFO(nos::sys::cuda::Stream,
		nos::sys::cuda::Stream::GetFullyQualifiedName());
} // namespace nos

namespace nos::sys::cuda
{
namespace internal
{
template <typename T>
concept CudaOrVoid = std::is_same_v<T, nosCudaSubsystem> || std::is_same_v<T, void>;

template <CudaOrVoid cudaType = void>
[[nodiscard]]
inline nosCudaSubsystem* GetCudaSubsystem(cudaType* cuda = {})
{
	if constexpr (std::is_same_v<cudaType, nosCudaSubsystem>)
		return cuda;
	else
		return nosCuda;
}
} // namespace internal

template <internal::CudaOrVoid cudaType = void>
inline TypedObjectRef<Buffer> CreateBuffer(nosCudaBufferCreateInfo const& createInfo, cudaType* cuda = nullptr)
{
	auto cu = internal::GetCudaSubsystem(cuda);
	TypedObjectRef<Buffer> buffer{};
	cu->CreateBuffer(const_cast<nosCudaBufferCreateInfo*>(&createInfo), &buffer.GetStorage());
	return buffer;
}

template <internal::CudaOrVoid cudaType = void>
inline std::optional<nosCudaBufferInfo> GetBufferInfo(nosCudaBufferObject buffer, cudaType* cuda = nullptr)
{
	auto cu = internal::GetCudaSubsystem(cuda);
	nosCudaBufferInfo info{};
	if (cu->GetCudaBufferInfo(buffer, &info) != NOS_RESULT_SUCCESS)
		return std::nullopt;
	return info;
}

template <internal::CudaOrVoid cudaType = void>
inline std::optional<nosCudaBufferInfo> GetBufferInfo(TypedObjectRef<Buffer> const& buffer, cudaType* cuda = nullptr)
{
	return GetBufferInfo(buffer.GetObjectId(), cuda);
}

template <internal::CudaOrVoid cudaType = void>
inline Result<ObjectRef, nosResult> CreateStream(cudaType* cuda = nullptr)
{
	auto cu = internal::GetCudaSubsystem(cuda);
	ObjectRef ref{};
	auto res = cu->CreateStream(&ref.GetStorage());
	if (res != NOS_RESULT_SUCCESS)
		return Error(res);
	return Ok(ref);
}
} // namespace nos::sys::cuda
