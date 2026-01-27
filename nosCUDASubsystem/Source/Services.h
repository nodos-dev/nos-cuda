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
	do{							\
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
		}						\
	}while(0)
#define CHECK_VALID_ARGUMENT(ptrArg)	\
	do{							\
		if (ptrArg == nullptr) {	\
			return NOS_RESULT_FAILED; \
		}						\
	}while(0)
#define CHECK_IS_SUPPORTED(result, propertyName)	\
	do{ \
		if (result != 1) {\
			nosEngine.LogE("Property %s is not suported by current CUDA version.",#propertyName); \
			return NOS_RESULT_FAILED; \
		} \
	} while (0);
#define CHECK_CONTEXT_SWITCH() \
	do{\
	   ContextSwitch();\
	} while (0);

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

private:
	StreamObject(cudaStream_t handle);
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

nosResult CopyBuffers(nosCudaBufferInfo* source, nosCudaBufferInfo* destination);
nosResult CopyBuffersAsync(nosCudaStreamObject stream, nosCudaBufferInfo* source, nosCudaBufferInfo* destination);
nosResult AddCallback(nosCudaStreamObject stream, nosCudaCallbackFunction callback, void* callbackData);
nosResult WaitExternalSemaphore(nosCudaStreamObject stream, nosCudaExternalSemaphore extSem, uint64_t value);
nosResult SignalExternalSemaphore(nosCudaStreamObject stream, nosCudaExternalSemaphore extSem, uint64_t value);

nosResult CreateBufferOnCuda(nosCudaBufferInfo* cudaBuffer, uint64_t size);
nosResult CreateShareableBufferOnCuda(nosCudaBufferInfo* cudaBuffer, uint64_t size); //Exportable
nosResult CreateBufferOnManagedMemory(nosCudaBufferInfo* cudaBuffer, uint64_t size);
//Allocates in Unified Memory Space 
nosResult CreateBufferPinned(nosCudaBufferInfo* cudaBuffer, uint64_t size); //Pinned memory in RAM 
nosResult InitBuffer(void* source, uint64_t size, nosCudaMemoryType type, nosCudaBufferInfo* destination);
nosResult CreateBuffer(nosCudaBufferInfo* cudaBuffer, uint64_t size); //Pinned memory in RAM 
nosResult GetCudaBufferFromAddress(uint64_t address, nosCudaBufferInfo* outBuffer);
//In case you lost the cuda buffer (hope not)

nosResult DestroyBuffer(nosCudaBufferInfo* cudaBuffer); //Allocates in Unified Memory Space

nosResult ImportExternalMemoryAsCudaBuffer(uint64_t Handle,
                                           size_t BlockSize,
                                           size_t AllocationSize,
                                           size_t Offset,
                                           nosCudaExternalMemoryHandleType handleType,
                                           nosCudaBufferInfo* outBuffer);
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
}
}