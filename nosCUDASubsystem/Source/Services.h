// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#pragma once
#include <driver_types.h>

#include "Nodos/Plugin.hpp"
#include "Nodos/PluginAPI.h"
#include "nosSysCuda/nosCUDASubsystem.h"
#include "nosSysCuda/Types_generated.h"

#define CHECK_CUDA_RT_ERROR(cudaErr)	\
	if (cudaErr != cudaSuccess) {	\
		nosEngine.LogE("CUDA RT failed with error: %s from line: [%s:%d]", cudaGetErrorString(cudaErr), __FILE__, __LINE__);	\
		return NOS_RESULT_FAILED; \
	}

#define CHECK_CUDA_RT_ERROR_LOG_ONLY(cudaErr) \
	if (cudaErr != cudaSuccess) { \
		nosEngine.LogE("CUDA RT failed with error: %s from line: [%s:%d]", cudaGetErrorString(cudaErr), __FILE__, __LINE__);	\
	}

#define CHECK_CUDA_DRIVER_ERROR(cuRes)	\
	if (cuRes != CUDA_SUCCESS) {	\
		const char* errorStr = nullptr; \
		cuGetErrorString(cuRes, &errorStr); \
		if(errorStr != nullptr){\
			nosEngine.LogE("CUDA Driver failed with error: %s [%s:%d]", errorStr, __FILE__, __LINE__);	\
		}\
		else{\
			nosEngine.LogE("CUDA Driver failed with unknown error. [%s:%d]", __FILE__, __LINE__);\
		}\
		return NOS_RESULT_FAILED; \
	}
#define CHECK_VALID_ARGUMENT(ptrArg)	\
	if (!ptrArg) {	\
		return NOS_RESULT_INVALID_ARGUMENT; \
	}
#define CHECK_IS_SUPPORTED(result, propertyName)	\
	if (result != 1) {\
		nosEngine.LogE("Property %s is not suported by current CUDA version.",#propertyName); \
		return NOS_RESULT_FAILED; \
	} 
#define CHECK_CONTEXT_SWITCH() \
	ContextSwitch();

namespace nos::sys::cuda
{
NOS_REGISTER_NAME_SPACED(StreamTypeName, nos::sys::cuda::Stream::GetFullyQualifiedName())
NOS_REGISTER_NAME_SPACED(BufferTypeName, nos::sys::cuda::Buffer::GetFullyQualifiedName())

struct StreamObject
{
	cudaStream_t Handle;
	~StreamObject();
	static Result<std::unique_ptr<StreamObject>, nosResult> Create();
	EngineBuffer Serialize() const;
	StreamObject(cudaStream_t handle);
};

struct BufferObject
{
	nosCudaBufferInfo Info{};
	~BufferObject();
	EngineBuffer Serialize() const;
	static Result<std::unique_ptr<BufferObject>, nosResult> Create(const nosCudaBufferCreateInfo& createInfo);
	nos::sys::cuda::BufferElementType ElementType = nos::sys::cuda::BufferElementType::ELEMENT_TYPE_UNDEFINED;
	nosCudaExternalMemoryHandleType ExternalMemoryHandleType = NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32;
	uint64_t ExternalMemoryPid = 0;
	bool HasExternalMemoryHandleType = false;
};

void Bind(nosCudaSubsystem* subsys);

nosResult CreateCudaContext(nosCudaContext* cudaContext, int device, nosCudaContextFlags flags);
nosResult DestroyCudaContext(nosCudaContext cudaContext); // Use with caution! Destroys the context

nosResult Initialize(int device); //Initialize CUDA Runtime

nosResult SetContext(nosCudaContext cudaContext);
//Sets and initializes the given CUDA Context for the calling Plugin as current for any subsequent calls from that Plugin
nosResult SetCurrentContextToPrimary(); //Sets the current context as the primary context of CUDA Subsystem

nosResult GetCurrentContext(nosCudaContext* cudaContext); //Retrieve the primary CUDA Context of CUDA Subsystem
nosResult GetCudaVersion(nosCudaVersion* versionInfo); //CUDA version
nosResult GetDeviceCount(int* deviceCount); //Number of GPUs
nosResult GetDeviceProperties(int device, nosCudaDeviceProperties* deviceProperties); //device cant exceed deviceCount

nosResult CreateCudaEvent(nosCudaEvent* cudaEvent, nosCudaEventFlags flags);
nosResult DestroyCudaEvent(nosCudaEvent cudaEvent);

nosResult LoadKernelModuleFromPtx(const char* ptxPath, nosCudaModule* outModule);
//Loads .ptx files only. //TODO: This should be extended to char arrays and .cu files.
nosResult LoadKernelModuleFromCString(const char* ptxString,
                                      const char* errorLogBuffer,
                                      uint64_t errorLogBufferSize,
                                      nosCudaModule* outModule);

nosResult GetModuleKernelFunction(const char* functionName,
                                  nosCudaModule cudaModule,
                                  nosCudaKernelFunction* outFunction);

nosResult LaunchModuleKernelFunction(nosCudaStreamObject stream,
                                     nosCudaKernelFunction outFunction,
                                     nosCudaKernelLaunchConfig config,
                                     void** arguments,
                                     nosCudaCallbackFunction callback,
                                     void* callbackData);

nosResult CreateStreamObject(nosObjectReference* outStreamObject);
nosResult WaitStream(nosCudaStreamObject stream); //Waits all commands in the stream to be completed
nosCudaResult QueryStream(nosCudaStreamObject stream); //

nosCudaResult GetLastError(); //Waits all commands in the stream to be completed

nosResult AddEventToStream(nosCudaStreamObject stream, nosCudaEvent measureEvent);
nosResult WaitCudaEvent(nosCudaEvent waitEvent);
nosResult QueryCudaEvent(nosCudaEvent waitEvent, nosCudaEventStatus* eventStatus);
//Must be called before enqueueing the operation to stream
nosResult GetCudaEventElapsedTime(nosCudaStreamObject stream, nosCudaEvent theEvent, float* elapsedTime);
//Get elapsed time between now and the measureEvent

nosResult CopyBuffer(nosCudaBufferObject srcObj, nosCudaBufferObject dstObj);
nosResult CopyFromHost(void* source, nosCudaBufferObject dstObj, uint64_t size);
nosResult CopyBufferAsync(nosCudaStreamObject stream, nosCudaBufferObject srcObj, nosCudaBufferObject dstObj);
nosResult AddCallback(nosCudaStreamObject stream, nosCudaCallbackFunction callback, void* callbackData);
nosResult WaitExternalSemaphore(nosCudaStreamObject stream, nosCudaExternalSemaphore extSem, uint64_t value);
nosResult SignalExternalSemaphore(nosCudaStreamObject stream, nosCudaExternalSemaphore extSem, uint64_t value);

nosResult CreateBuffer(nosCudaBufferCreateInfo* createInfo, nosObjectReference* outCudaBufferObject);
nosResult GetCudaBufferFromAddress(uint64_t address, nosObjectReference* outCudaBufferObject);
nosResult GetCudaBufferInfo(nosCudaBufferObject buffer, nosCudaBufferInfo* outBufferInfo);
//In case you lost the cuda buffer (hope not)

nosResult ImportExternalMemoryAsCudaBuffer(nosCudaBufferImportInfo* importInfo,
                                           uint64_t blockSize,
                                           nosObjectReference* outCudaBufferObject);
nosResult ImportExternalSemaphore(uint64_t handle,
                                  nosCudaExternalSemaphoreHandleType handleType,
                                  nosCudaExternalSemaphore* extSem);

nosResult CreateCuRandState(nosCudaRandState* state, uint64_t count);
nosResult DestroyCuRandState(nosCudaRandState* state);

nosResult ContextSwitch();

namespace type
{
nosResult ConstructStreamObject(nosBuffer buffer, nosForeignHandle* outForeignHandle, nosBuffer* outSerializedData);
void ReleaseStreamObject(nosForeignHandle foreignHandle);
nosResult InitializeStreamPinObject(nosFbShowAs showAs, nosUUID pinId, nosBuffer constructorBuffer);
void OnStreamInputPinDisconnected(const nosOnInputPinDisconnectedParams* params);

nosResult ConstructBufferObject(nosBuffer buffer, nosForeignHandle* outForeignHandle, nosBuffer* outSerializedData);
void ReleaseBufferObject(nosForeignHandle foreignHandle);
nosResult InitializeBufferPinObject(nosFbShowAs showAs, nosUUID pinId, nosBuffer constructorBuffer);
void OnBufferInputPinDisconnected(const nosOnInputPinDisconnectedParams* params);
}
}
