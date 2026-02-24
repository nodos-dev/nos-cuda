// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include "Services.h"
#include <cstring>
#include "Common.hpp"
// SDK
#include <Nodos/PluginAPI.h>
#include <cuda_runtime.h>
#include <cuda.h>
#include <curand_kernel.h>
#include "Globals.h"

namespace nos::sys::cuda
{
nosResult GetCallingPluginId(nos::Name* ID);

void Bind(nosCudaSubsystem* subsys)
{

	subsys->CreateCudaContext = CreateCudaContext;
	subsys->DestroyCudaContext = DestroyCudaContext;

	subsys->Initialize = Initialize;
	subsys->SetContext = SetContext;
	subsys->SetCurrentContextToPrimary = SetCurrentContextToPrimary;
	subsys->GetCurrentContext = GetCurrentContext;
	subsys->GetCudaVersion = GetCudaVersion;
	subsys->GetDeviceCount = GetDeviceCount;
	subsys->GetDeviceProperties = GetDeviceProperties;

	subsys->CreateStream = CreateStreamObject;
	subsys->CreateCudaEvent = CreateCudaEvent;
	subsys->DestroyCudaEvent = DestroyCudaEvent;

	subsys->LoadKernelModuleFromPtx = LoadKernelModuleFromPtx;
	subsys->LoadKernelModuleFromCString = LoadKernelModuleFromCString;
	subsys->GetModuleKernelFunction = GetModuleKernelFunction;
	subsys->LaunchModuleKernelFunction = LaunchModuleKernelFunction;

	subsys->WaitStream = WaitStream;
	subsys->QueryStream = QueryStream;

	subsys->GetLastError = GetLastError;

	subsys->AddEventToStream = AddEventToStream;
	subsys->WaitCudaEvent = WaitCudaEvent;
	subsys->QueryCudaEvent = QueryCudaEvent;
	subsys->GetCudaEventElapsedTime = GetCudaEventElapsedTime;
	subsys->WaitExternalSemaphore = WaitExternalSemaphore;
	subsys->SignalExternalSemaphore = SignalExternalSemaphore;

	subsys->AddCallback = AddCallback;

	subsys->CreateBuffer = CreateBuffer;
	subsys->CopyBuffer = CopyBuffer;
	subsys->CopyFromHost = CopyFromHost;
	subsys->CopyBufferAsync = CopyBufferAsync;
	subsys->GetCudaBufferFromAddress = GetCudaBufferFromAddress;
	subsys->GetCudaBufferInfo = GetCudaBufferInfo;

	subsys->ImportExternalSemaphore = ImportExternalSemaphore;
	subsys->ImportExternalMemoryAsCudaBuffer = ImportExternalMemoryAsCudaBuffer;

	subsys->CreateCuRandState = CreateCuRandState;
}

nosResult NOSAPI_CALL CreateCudaContext(nosCudaContext* cudaContext, int device, nosCudaContextFlags flags)
{
	nos::Name ID{};
	nosResult nosRes = GetCallingPluginId(&ID);
	if (nosRes != NOS_RESULT_SUCCESS)
		return nosRes;
	CUcontext theContext = {};
	CUresult cuRes = cuCtxCreate(&theContext, flags, device);
	CHECK_CUDA_DRIVER_ERROR(cuRes);
	cuRes = cuCtxPopCurrent(reinterpret_cast<CUcontext*>(&ActiveContext));
	CHECK_CUDA_DRIVER_ERROR(cuRes);
	(*cudaContext) = reinterpret_cast<nosCudaContext>(theContext);
	IDContextMap[ID] = (*cudaContext);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL DestroyCudaContext(nosCudaContext cudaContext)
{
	if (cudaContext == PrimaryContext)
		return NOS_RESULT_SUCCESS;

	CUresult cuRes = cuCtxDestroy(reinterpret_cast<CUcontext>(cudaContext));
	CHECK_CUDA_DRIVER_ERROR(cuRes);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL Initialize(int device)
{
	if (device == CurrentDevice && PrimaryContext != nullptr)
	{
		//Already initialized
		return NOS_RESULT_SUCCESS;
	}

	//We will initialize CUDA Runtime explicitly, Driver API will also be initialized implicitly
	cudaError res = cudaSuccess;
	CHECK_CUDA_RT_ERROR(res);
	//DRIVER API Initialization
	CUresult cuRes;

	cuRes = cuInit(0);
	CHECK_CUDA_DRIVER_ERROR(cuRes);
#if CUDA_VERSION >= 12000
	res = cudaSetDevice(device);
	CHECK_CUDA_RT_ERROR(res);
	cuRes = cuCtxSetFlags(CU_CTX_COREDUMP_ENABLE);
	CHECK_CUDA_DRIVER_ERROR(cuRes);
	std::string CoreDumpFile = std::string(nosEngine.Plugin->RootFolderPath) + "/CoreDump.txt";
	size_t Size = CoreDumpFile.size();
	cuRes = cuCoredumpSetAttribute(CU_COREDUMP_FILE, &CoreDumpFile, &Size);
#else
	res = cudaFree(0); //explicit initialization pre CUDA 12.0
#endif

	/*cuRes = cuCtxCreate(reinterpret_cast<CUcontext*>(&PrimaryContext), 0, device);
	CHECK_CUDA_DRIVER_ERROR(cuRes);
	
	cuRes = cuCtxSetCurrent(reinterpret_cast<CUcontext>(PrimaryContext));
	CHECK_CUDA_DRIVER_ERROR(cuRes);*/
	CUcontext tempPrimary = {};
	cuRes = cuCtxGetCurrent(&tempPrimary);
	PrimaryContext = tempPrimary;
	ActiveContext = PrimaryContext;

	CurrentDevice = device;
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL SetContext(nosCudaContext cudaContext)
{
	CUresult cuRes = cuCtxSetCurrent(reinterpret_cast<CUcontext>(cudaContext));
	CHECK_CUDA_DRIVER_ERROR(cuRes);
	ActiveContext = cudaContext;
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL SetCurrentContextToPrimary()
{
	CUcontext currentCtx = {};
	cuCtxGetCurrent(&currentCtx);
	if (currentCtx != PrimaryContext)
	{
		ActiveContext = PrimaryContext;
		CUresult cuRes = cuCtxSetCurrent(reinterpret_cast<CUcontext>(ActiveContext));
		CHECK_CUDA_DRIVER_ERROR(cuRes);
	}
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL GetCurrentContext(nosCudaContext* cudaContext)
{
	(*cudaContext) = reinterpret_cast<nosCudaContext>(PrimaryContext);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL GetCudaVersion(nosCudaVersion* versionInfo)
{
	int cudaVersion = 0;
	cudaError res = cudaDriverGetVersion(&cudaVersion);
	CHECK_CUDA_RT_ERROR(res);

	versionInfo->Major = cudaVersion / 1000;
	versionInfo->Minor = (cudaVersion - (cudaVersion / 1000) * 1000) / 10;
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL GetDeviceCount(int* deviceCount)
{
	cudaError res = cudaSuccess;
	res = cudaGetDeviceCount(deviceCount);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL GetDeviceProperties(int device, nosCudaDeviceProperties* deviceProperties)
{
	CHECK_VALID_ARGUMENT(deviceProperties);
	cudaDeviceProp deviceProp = {};
	cudaError res = cudaGetDeviceProperties(&deviceProp, device);
	CHECK_CUDA_RT_ERROR(res);
	deviceProperties->ComputeCapabilityMajor = deviceProp.major;
	deviceProperties->ComputeCapabilityMinor = deviceProp.minor;
	memcpy(deviceProp.uuid.bytes, deviceProperties->DeviceUUID, UUID_SIZE);
	memcpy(deviceProp.name, deviceProperties->Name, DEVICE_NAME_SIZE);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL CreateCudaEvent(nosCudaEvent* cudaEvent, nosCudaEventFlags flags)
{
	CHECK_CONTEXT_SWITCH();
	cudaEvent_t event;
	cudaError res = cudaEventCreate(&event, flags);
	CHECK_CUDA_RT_ERROR(res);
	(*cudaEvent) = event;
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL DestroyCudaEvent(nosCudaEvent cudaEvent)
{
	CHECK_CONTEXT_SWITCH();
	cudaError res = cudaEventDestroy(reinterpret_cast<cudaEvent_t>(cudaEvent));
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL LoadKernelModuleFromPtx(const char* ptxPath, nosCudaModule* outModule)
{
	CHECK_CONTEXT_SWITCH();
	CUmodule cuModule;
	CUresult res = cuModuleLoad(&cuModule, ptxPath);
	CHECK_CUDA_DRIVER_ERROR(res);
	(*outModule) = cuModule;
	return NOS_RESULT_SUCCESS;
}

nosResult LoadKernelModuleFromCString(const char* ptxString,
                                      const char* errorLogBuffer,
                                      uint64_t errorLogBufferSize,
                                      nosCudaModule* outModule)
{
	CHECK_CONTEXT_SWITCH();
	CUmodule cuModule;
	CUjit_option options[3];
	void* values[3];

	options[0] = CU_JIT_ERROR_LOG_BUFFER;
	values[0] = (void*)errorLogBuffer;
	options[1] = CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES;
	values[1] = (void*)errorLogBufferSize;
	options[2] = CU_JIT_TARGET_FROM_CUCONTEXT;
	values[2] = 0;

	CUresult res = cuModuleLoadDataEx(&cuModule, ptxString, 3, options, values);
	CHECK_CUDA_DRIVER_ERROR(res);
	(*outModule) = cuModule;
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL GetModuleKernelFunction(const char* functionName,
                                              nosCudaModule cudaModule,
                                              nosCudaKernelFunction* outFunction)
{
	CHECK_CONTEXT_SWITCH();
	CUfunction cuFunction = NULL;;
	CUresult res = cuModuleGetFunction(&cuFunction, reinterpret_cast<CUmodule>(cudaModule), functionName);
	CHECK_CUDA_DRIVER_ERROR(res);
	(*outFunction) = cuFunction;
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL LaunchModuleKernelFunction(nosCudaStreamObject stream,
                                                 nosCudaKernelFunction outFunction,
                                                 nosCudaKernelLaunchConfig config,
                                                 void** arguments,
                                                 nosCudaCallbackFunction callback,
                                                 void* callbackData)
{
	CHECK_CONTEXT_SWITCH();
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	CUresult res = cuLaunchKernel(reinterpret_cast<CUfunction>(outFunction),
	                              config.GridDimensions.x,
	                              config.GridDimensions.y,
	                              config.GridDimensions.z,
	                              config.BlockDimensions.x,
	                              config.BlockDimensions.y,
	                              config.BlockDimensions.z,
	                              config.DynamicMemorySize,
	                              streamObject->Handle,
	                              arguments,
	                              0);
	CHECK_CUDA_DRIVER_ERROR(res);
	return AddCallback(stream, callback, callbackData);
}

nosResult CreateStreamObject(nosObjectReference* outStreamObject)
{
	auto res = StreamObject::Create();
	if (auto* err = res.Error())
		return *err;
	auto* streamObj = (*res).release();
	auto serializedData = streamObj->Serialize();
	return nosEngine.ObjectAPI->CreateObjectForForeignHandle(NSN_StreamTypeName, streamObj, serializedData, outStreamObject);
}

nosResult NOSAPI_CALL WaitStream(nosCudaStreamObject stream)
{
	CHECK_CONTEXT_SWITCH();
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	cudaError res = cudaStreamSynchronize(streamObject->Handle);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosCudaResult QueryStream(nosCudaStreamObject stream)
{
	CHECK_CONTEXT_SWITCH();
	if (auto* streamObject = nos::GetForeignHandle<StreamObject>(stream))
	{
		cudaError res = cudaStreamQuery(streamObject->Handle);
		return static_cast<nosCudaResult>(res);
	}
	return NOS_CUDA_ERROR_INVALID_VALUE;
}

nosCudaResult GetLastError()
{
	CHECK_CONTEXT_SWITCH();
	return static_cast<nosCudaResult>(cudaGetLastError());
}

nosResult NOSAPI_CALL AddEventToStream(nosCudaStreamObject stream, nosCudaEvent measureEvent)
{
	CHECK_CONTEXT_SWITCH();
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	cudaError res = cudaEventRecord(static_cast<cudaEvent_t>(measureEvent),
	                                streamObject->Handle);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL WaitCudaEvent(nosCudaEvent waitEvent)
{
	CHECK_CONTEXT_SWITCH();
	cudaError res = cudaEventSynchronize(static_cast<cudaEvent_t>(waitEvent));
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL QueryCudaEvent(nosCudaEvent waitEvent, nosCudaEventStatus* eventStatus)
{
	CHECK_CONTEXT_SWITCH();
	cudaError res = cudaEventQuery(reinterpret_cast<cudaEvent_t>(waitEvent));

	if (res == cudaErrorNotReady)
	{
		(*eventStatus) = NOS_CUDA_EVENT_STATUS_NOT_READY;
		return NOS_RESULT_SUCCESS;
	}
	else if (res == cudaSuccess)
	{
		(*eventStatus) = NOS_CUDA_EVENT_STATUS_READY;
		return NOS_RESULT_SUCCESS;
	}

	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL GetCudaEventElapsedTime(nosCudaStreamObject stream, nosCudaEvent theEvent, float* elapsedTime)
{
	CHECK_CONTEXT_SWITCH();
	cudaEvent_t endEvent;

	cudaError res = cudaEventCreate(&endEvent);
	CHECK_CUDA_RT_ERROR(res);

	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	res = cudaEventRecord(endEvent, streamObject->Handle);
	CHECK_CUDA_RT_ERROR(res);

	res = cudaEventSynchronize(endEvent);
	CHECK_CUDA_RT_ERROR(res);

	res = cudaEventElapsedTime(elapsedTime, static_cast<cudaEvent_t>(theEvent), endEvent);
	CHECK_CUDA_RT_ERROR(res);

	//TODO: Destroy endEvent
	return NOS_RESULT_SUCCESS;
}

nosResult CopyCudaMemory(void* sourceAddress, void* dstAddress, uint64_t safeCopySize, nosCudaMemoryType srcMemType, nosCudaMemoryType dstMemType)
{
	cudaMemcpyKind kind{};
	switch (srcMemType)
	{
	case NOS_CUDA_MEMORY_TYPE_HOST: {
		kind = dstMemType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_DEVICE: {
		kind = dstMemType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyDeviceToDevice : cudaMemcpyDeviceToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_MANAGED: {
		kind = dstMemType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	default: return NOS_RESULT_INVALID_ARGUMENT;
	}
	cudaError res = cudaMemcpy(dstAddress, sourceAddress, safeCopySize, kind);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL CopyBuffer(nosCudaBufferObject srcObj, nosCudaBufferObject dstObj)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(srcObj);
	CHECK_VALID_ARGUMENT(dstObj);
	
	auto srcBuf = nos::GetForeignHandle<BufferObject>(srcObj);
	auto dstBuf = nos::GetForeignHandle<BufferObject>(dstObj);

	auto& source = srcBuf->Info;
	auto& destination = dstBuf->Info;
	
	if (source.MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED || destination.MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED)
	{
		nosEngine.LogE("Invalid memory type for CUDA memcopy operation.");
		return NOS_RESULT_FAILED;
	}
	if (source.AllocationSize != destination.AllocationSize)
	{
		nosEngine.LogW("nosCUDABuffers have size mismatch, trimming will be performed for copying.");
	}

	size_t safeCopySize = std::min(source.AllocationSize, destination.BlockSize);
	return CopyCudaMemory(reinterpret_cast<void*>(source.Address),
	                      reinterpret_cast<void*>(destination.Address),
	                      safeCopySize,
	                      source.MemoryType,
	                      destination.MemoryType);
}

nosResult NOSAPI_CALL CopyFromHost(void* source, nosCudaBufferObject dstObj, uint64_t size)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(source);
	CHECK_VALID_ARGUMENT(dstObj);

	auto dstBuf = nos::GetForeignHandle<BufferObject>(dstObj);
	auto& destination = dstBuf->Info;

	if (destination.MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED)
	{
		nosEngine.LogE("Invalid memory type for CUDA memcopy operation.");
		return NOS_RESULT_FAILED;
	}
	if (size > destination.BlockSize)
	{
		nosEngine.LogW(
			"Copy size is larger than destination buffer block size, trimming will be performed for copying.");
	}

	return CopyCudaMemory(source,
	                      reinterpret_cast<void*>(destination.Address),
	                      std::min(size, destination.BlockSize),
	                      NOS_CUDA_MEMORY_TYPE_HOST,
	                      destination.MemoryType);
}

nosResult CopyBufferAsync(nosCudaStreamObject stream, nosCudaBufferObject srcObj, nosCudaBufferObject dstObj)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(srcObj);
	CHECK_VALID_ARGUMENT(dstObj);
	
	auto srcBuf = nos::GetForeignHandle<BufferObject>(srcObj);
	auto dstBuf = nos::GetForeignHandle<BufferObject>(dstObj);

	auto& source = srcBuf->Info;
	auto& destination = dstBuf->Info;

	if (source.MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED || destination.MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED)
	{
		nosEngine.LogE("Invalid memory type for CUDA memcopy operation.");
		return NOS_RESULT_FAILED;
	}
	if (source.AllocationSize != destination.AllocationSize)
	{
		nosEngine.LogW("nosCUDABuffers have size mismatch, trimming will be performed for copying.");
	}

	cudaError res = cudaSuccess;
	size_t safeCopySize = std::min(source.AllocationSize, destination.BlockSize);
	cudaMemcpyKind kind{};
	switch (source.MemoryType)
	{
	case NOS_CUDA_MEMORY_TYPE_HOST: {
		kind = destination.MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_DEVICE: {
		kind = destination.MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyDeviceToDevice : cudaMemcpyDeviceToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_MANAGED: {
		kind = destination.MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	default: return NOS_RESULT_INVALID_ARGUMENT;
	}
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	res = cudaMemcpyAsync(reinterpret_cast<void*>(destination.Address),
						  reinterpret_cast<void*>(source.Address),
						  safeCopySize,
						  kind,
						  streamObject->Handle);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL AddCallback(nosCudaStreamObject stream, nosCudaCallbackFunction callback, void* callbackData)
{
	CHECK_CONTEXT_SWITCH();
	if (callback == nullptr || callbackData == nullptr)
		return NOS_RESULT_SUCCESS;
	
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	CUresult res = cuLaunchHostFunc(streamObject->Handle, callback, callbackData);
	CHECK_CUDA_DRIVER_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult WaitExternalSemaphore(nosCudaStreamObject stream, nosCudaExternalSemaphore extSem, uint64_t value)
{
	CHECK_CONTEXT_SWITCH();
	cudaExternalSemaphoreWaitParams params = {};
	params.flags = 0;
	params.params.fence.value = value;

	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	cudaError res = cudaWaitExternalSemaphoresAsync(reinterpret_cast<cudaExternalSemaphore_t*>(&extSem),
	                                                &params,
	                                                1,
	                                                streamObject->Handle);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult SignalExternalSemaphore(nosCudaStreamObject stream, nosCudaExternalSemaphore extSem, uint64_t value)
{
	CHECK_CONTEXT_SWITCH();
	cudaExternalSemaphoreSignalParams params = {};
	params.flags = 0;
	params.params.fence.value = value;
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	cudaError res = cudaSignalExternalSemaphoresAsync(reinterpret_cast<cudaExternalSemaphore_t*>(&extSem),
	                                                  &params,
	                                                  1,
	                                                  streamObject->Handle);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

static nosResult CreateBufferOnCudaInternal(nosCudaBufferInfo* cudaBuffer, uint64_t size)
{
	CHECK_CONTEXT_SWITCH();
	uint64_t addr = NULL;
	cudaError_t res = cudaMalloc((void**)&addr, size);
	CHECK_CUDA_RT_ERROR(res);
	cudaBuffer->Address = addr;
	cudaBuffer->ShareInfo.ShareableHandle = NULL;
	cudaBuffer->ShareInfo.CreateHandle = NULL;
	cudaBuffer->ShareInfo.ReferenceCount = 0;
	cudaBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_DEVICE;
	cudaBuffer->BlockSize = size;
	cudaBuffer->AllocationSize = size;
	cudaBuffer->Offset = 0;
	cudaBuffer->IsImported = false;
	cudaBuffer->ImportedCudaExternalMemory = 0;
	cudaBuffer->ImportedExternalHandle = 0;
	// ResManager tracks BufferObject instances, not raw buffer infos.
	return NOS_RESULT_SUCCESS;
}

static nosResult CreateShareableBufferOnCudaInternal(nosCudaBufferInfo* cudaBuffer, uint64_t size)
{
	CHECK_CONTEXT_SWITCH();
	CUdevice dev;
	int supportsVMM = 0, supportsWin32 = 0;

	CUresult status = CUDA_SUCCESS;

	status = cuCtxGetDevice(&dev);
	CHECK_CUDA_DRIVER_ERROR(status);

	cuDeviceGetAttribute(&supportsVMM, CU_DEVICE_ATTRIBUTE_VIRTUAL_MEMORY_MANAGEMENT_SUPPORTED, dev);
	CHECK_IS_SUPPORTED(supportsVMM, VIRTUAL_MEMORY_MANAGEMENT);

	cuDeviceGetAttribute(&supportsWin32,
	                     CU_DEVICE_ATTRIBUTE_HANDLE_TYPE_WIN32_HANDLE_SUPPORTED,
	                     dev);
	CHECK_IS_SUPPORTED(supportsWin32, HANDLE_TYPE_WIN32_HANDLE);

	CUmemAllocationProp prop;

	memset(&prop, 0, sizeof(prop));

	prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
	prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
	prop.location.id = (int)dev;
	prop.win32HandleMetaData = SecurityDescriptor::GetDefaultSecurityDescriptor();
	prop.requestedHandleTypes = CU_MEM_HANDLE_TYPE_WIN32;

	size_t chunk_sz;
	status = cuMemGetAllocationGranularity(&chunk_sz, &prop, CU_MEM_ALLOC_GRANULARITY_MINIMUM);
	CHECK_CUDA_DRIVER_ERROR(status);
	const size_t aligned_size = ((size + chunk_sz - 1) / chunk_sz) * chunk_sz;

	CUmemGenericAllocationHandle allocHandle = NULL;

	status = cuMemCreate(&allocHandle, aligned_size, &prop, 0);
	CHECK_CUDA_DRIVER_ERROR(status);

	CUdeviceptr address = 0ULL;
	status = cuMemAddressReserve(&address, (aligned_size), 0ULL, 0ULL, 0ULL);
	CHECK_CUDA_DRIVER_ERROR(status);

	status = cuMemMap(address, aligned_size, 0, allocHandle, 0);
	//We should unmap this somehow after each usage and map again before usages
	CHECK_CUDA_DRIVER_ERROR(status);

	CUmemAccessDesc accessDesc = {};
	memset(&accessDesc, 0, sizeof(&accessDesc));
	accessDesc.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
	accessDesc.location.id = dev;
	accessDesc.flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
	// Make the address accessible: should we give api users to flesibiltiy to choose?
	status = cuMemSetAccess(address, aligned_size, &accessDesc, 1);
	CHECK_CUDA_DRIVER_ERROR(status);

	uint64_t shareableHandle = 0;
	status = cuMemExportToShareableHandle((void*)&shareableHandle, allocHandle, CU_MEM_HANDLE_TYPE_WIN32, 0);
	CHECK_CUDA_DRIVER_ERROR(status);

	cudaBuffer->BlockSize = aligned_size;
	cudaBuffer->AllocationSize = size;
	cudaBuffer->ShareInfo.CreateHandle = allocHandle;
	cudaBuffer->ShareInfo.ShareableHandle = shareableHandle;
	cudaBuffer->ShareInfo.ReferenceCount = 1;
	cudaBuffer->Address = address;
	cudaBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_DEVICE;
	cudaBuffer->Offset = 0;
	cudaBuffer->IsImported = false;
	cudaBuffer->ImportedCudaExternalMemory = 0;
	cudaBuffer->ImportedExternalHandle = 0;
	// ResManager tracks BufferObject instances, not raw buffer infos.
	return NOS_RESULT_SUCCESS;

}

static nosResult CreateBufferOnManagedMemoryInternal(nosCudaBufferInfo* cudaBuffer, uint64_t size)
{
	CHECK_CONTEXT_SWITCH();
	uint64_t addr = NULL;
	cudaError res = cudaMallocManaged((void**)&addr, size);
	CHECK_CUDA_RT_ERROR(res);
	cudaBuffer->Address = addr;
	cudaBuffer->ShareInfo.ShareableHandle = NULL;
	cudaBuffer->ShareInfo.CreateHandle = NULL;
	cudaBuffer->ShareInfo.ReferenceCount = 0;
	cudaBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_MANAGED;
	cudaBuffer->AllocationSize = size;
	cudaBuffer->BlockSize = size;
	cudaBuffer->Offset = 0;
	cudaBuffer->IsImported = false;
	cudaBuffer->ImportedCudaExternalMemory = 0;
	cudaBuffer->ImportedExternalHandle = 0;
	// ResManager tracks BufferObject instances, not raw buffer infos.

	return NOS_RESULT_SUCCESS;
}

static nosResult CreateBufferPinnedInternal(nosCudaBufferInfo* cudaBuffer, uint64_t size)
{
	CHECK_CONTEXT_SWITCH();
	uint64_t addr = NULL;
	cudaError res = cudaMallocHost((void**)&addr, size);
	CHECK_CUDA_RT_ERROR(res);
	cudaBuffer->Address = addr;
	cudaBuffer->ShareInfo.ShareableHandle = NULL;
	cudaBuffer->ShareInfo.CreateHandle = NULL;
	cudaBuffer->ShareInfo.ReferenceCount = 0;
	cudaBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_HOST;
	cudaBuffer->AllocationSize = size;
	cudaBuffer->BlockSize = size;
	cudaBuffer->Offset = 0;
	cudaBuffer->IsImported = false;
	cudaBuffer->ImportedCudaExternalMemory = 0;
	cudaBuffer->ImportedExternalHandle = 0;
	// ResManager tracks BufferObject instances, not raw buffer infos.

	return nosResult();
}

static nosResult ImportExternalMemoryAsCudaBufferInternal(uint64_t Handle,
                                                         size_t BlockSize,
                                                         size_t AllocationSize,
                                                         size_t Offset,
                                                         nosCudaExternalMemoryHandleType handleType,
                                                         nosCudaBufferInfo* outBuffer)
{
	CHECK_CONTEXT_SWITCH();
	//cuCtxSetCurrent(reinterpret_cast<CUcontext>(PrimaryContext));
	cudaExternalMemory_t externalMemory;
	cudaExternalMemoryHandleDesc desc;
	memset(&desc, 0, sizeof(desc));
	switch (handleType)
	{
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEFD: desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32: desc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32KMT: desc.type = cudaExternalMemoryHandleTypeOpaqueWin32Kmt;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12HEAP: desc.type = cudaExternalMemoryHandleTypeD3D12Heap;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12RESOURCE: desc.type = cudaExternalMemoryHandleTypeD3D12Resource;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11RESOURCE: desc.type = cudaExternalMemoryHandleTypeD3D11Resource;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11RESOURCEKMT: desc.type = cudaExternalMemoryHandleTypeD3D11ResourceKmt;
		break;
	case NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_NVSCIBUF: desc.type = cudaExternalMemoryHandleTypeNvSciBuf;
		break;
	}
	//TODO: do it in switch case
	desc.handle.win32.handle = reinterpret_cast<void*>(Handle);
	desc.handle.win32.name = NULL;
	desc.size = BlockSize;

	cudaError res = cudaImportExternalMemory(&externalMemory, &desc);
	CHECK_CUDA_RT_ERROR(res);

	void* pointer = nullptr;

	cudaExternalMemoryBufferDesc bufferDesc;
	bufferDesc.flags = 0; // must be zero
	bufferDesc.offset = Offset;
	bufferDesc.size = AllocationSize;

	res = cudaExternalMemoryGetMappedBuffer(&pointer, externalMemory, &bufferDesc);
	CHECK_CUDA_RT_ERROR(res);

	//Clean resources in case of re-importing
	//DestroyBuffer(outBuffer);

	outBuffer->Address = reinterpret_cast<uint64_t>(pointer);
	outBuffer->BlockSize = BlockSize;
	outBuffer->AllocationSize = AllocationSize;
	outBuffer->Offset = Offset;
	outBuffer->IsImported = true;
	outBuffer->ImportedCudaExternalMemory = reinterpret_cast<uint64_t>(externalMemory);
	outBuffer->ImportedExternalHandle = Handle;
	outBuffer->ShareInfo.CreateHandle = NULL;
	outBuffer->ShareInfo.ShareableHandle = NULL;
	outBuffer->ShareInfo.ReferenceCount = 0;
	outBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_DEVICE;
	// ResManager tracks BufferObject instances, not raw buffer infos.
	return NOS_RESULT_SUCCESS;
}

static nosResult CreateBufferResource(const nosCudaBufferCreateInfo& createInfo, nosCudaBufferInfo* info)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(info);
	*info = {};

	switch (createInfo.Type)
	{
	case NOS_CUDA_BUFFER_CREATION_TYPE_DEVICE:
		return createInfo.Device.Export ? CreateShareableBufferOnCudaInternal(info, createInfo.Size)
		                                : CreateBufferOnCudaInternal(info, createInfo.Size);
	case NOS_CUDA_BUFFER_CREATION_TYPE_HOST_PAGE_LOCKED: {
		auto res = CreateBufferPinnedInternal(info, createInfo.Size);
		if (res == NOS_RESULT_SUCCESS)
		{
			info->AllocationSize = createInfo.Size;
			info->BlockSize = createInfo.Size;
			info->IsImported = false;
		}
		return res;
	}
	case NOS_CUDA_BUFFER_CREATION_TYPE_MANAGED: {
		auto res = CreateBufferOnManagedMemoryInternal(info, createInfo.Size);
		if (res == NOS_RESULT_SUCCESS)
		{
			info->AllocationSize = createInfo.Size;
			info->BlockSize = createInfo.Size;
			info->IsImported = false;
		}
		return res;
	}
	case NOS_CUDA_BUFFER_CREATION_TYPE_IMPORTED:
		return ImportExternalMemoryAsCudaBufferInternal(createInfo.Imported.ExternalMemoryHandle,
		                                                createInfo.Size,
		                                                createInfo.Imported.AllocationSize,
		                                                createInfo.Imported.Offset,
		                                                createInfo.Imported.ExternalMemoryHandleType,
		                                                info);
	default:
		return NOS_RESULT_INVALID_ARGUMENT;
	}
}

nosResult CreateBuffer(nosCudaBufferCreateInfo* createInfo, nosObjectReference* outCudaBufferObject)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(createInfo);
	CHECK_VALID_ARGUMENT(outCudaBufferObject);

	auto res = BufferObject::Create(*createInfo);
	if (auto* err = res.Error())
		return *err;

	auto* bufferObj = (*res).release();
	auto serializedData = bufferObj->Serialize();
	auto result = nosEngine.ObjectAPI->CreateObjectForForeignHandle(NSN_BufferTypeName,
	                                                                bufferObj,
	                                                                serializedData,
	                                                                outCudaBufferObject);
	if (result == NOS_RESULT_SUCCESS)
		ResManager.Add(bufferObj->Info.Address, outCudaBufferObject->Object);
	return result;
}

nosResult ImportExternalMemoryAsCudaBuffer(nosCudaBufferImportInfo* importInfo,
                                           uint64_t blockSize,
                                           nosObjectReference* outCudaBufferObject)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(importInfo);
	CHECK_VALID_ARGUMENT(outCudaBufferObject);
	nosCudaBufferCreateInfo createInfo{};
	createInfo.Type = NOS_CUDA_BUFFER_CREATION_TYPE_IMPORTED;
	createInfo.Size = blockSize;
	createInfo.Imported = *importInfo;
	return CreateBuffer(&createInfo, outCudaBufferObject);
}

nosResult GetCudaBufferFromAddress(uint64_t address, nosObjectReference* outCudaBufferObject)
{
	CHECK_VALID_ARGUMENT(outCudaBufferObject);
	auto res = ResManager.Get(address);
	if (res == nullptr)
		return NOS_RESULT_FAILED;
	auto ref = nos::ObjectRef::FromObjectId(*res);
	*outCudaBufferObject = ref.Release();

	return NOS_RESULT_SUCCESS;
}

nosResult GetCudaBufferInfo(nosCudaBufferObject buffer, nosCudaBufferInfo* outBufferInfo)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(outBufferInfo);
	auto* bufferObj = nos::GetForeignHandle<BufferObject>(buffer);
	if (!bufferObj)
		return NOS_RESULT_FAILED;
	*outBufferInfo = bufferObj->Info;
	return NOS_RESULT_SUCCESS;
}

nosResult ImportExternalSemaphore(uint64_t handle,
                                  nosCudaExternalSemaphoreHandleType handleType,
                                  nosCudaExternalSemaphore* extSem)
{
	CHECK_CONTEXT_SWITCH();

	cudaExternalSemaphore_t extSemCuda = NULL;
	cudaExternalSemaphoreHandleDesc desc = {};
	memset(&desc, 0, sizeof(desc));
	desc.type = (cudaExternalSemaphoreHandleType)handleType;
	switch (handleType)
	{
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUEFD: desc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
		desc.handle.fd = handle;
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUEWIN32: desc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUEWIN32KMT: desc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32Kmt;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12FENCE: desc.type = cudaExternalSemaphoreHandleTypeD3D12Fence;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D11FENCE: desc.type = cudaExternalSemaphoreHandleTypeD3D11Fence;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_NVSCISYNC: desc.type = cudaExternalSemaphoreHandleTypeNvSciSync;
		desc.handle.nvSciSyncObj = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_KEYEDMUTEX: desc.type = cudaExternalSemaphoreHandleTypeKeyedMutex;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_KEYEDMUTEXKMT: desc.type = cudaExternalSemaphoreHandleTypeKeyedMutexKmt;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINESEMAPHOREFD
	: desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
		desc.handle.fd = handle;
		break;
	case NOS_CUDA_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINESEMAPHOREWIN32
	: desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreWin32;
		desc.handle.win32.handle = reinterpret_cast<void*>(handle);
		break;
	default: return NOS_RESULT_FAILED;
	}
	cudaError res = cudaImportExternalSemaphore(&extSemCuda, &desc);
	CHECK_CUDA_RT_ERROR(res);
	(*extSem) = extSemCuda;
	return NOS_RESULT_SUCCESS;
}

nosResult CreateCuRandState(nosCudaRandState* state, uint64_t count)
{
	uint64_t internalState = NULL;

	cudaError res = cudaMalloc((void**)&internalState, count * sizeof(curandState));
	CHECK_CUDA_RT_ERROR(res);

	(*state) = reinterpret_cast<void*>(internalState);
	return NOS_RESULT_SUCCESS;
}

nosResult DestroyCuRandState(nosCudaRandState* state)
{
	cudaError res = cudaFree(reinterpret_cast<curandState*>(*state));
	CHECK_CUDA_RT_ERROR(res);
	(*state) = NULL;
	return NOS_RESULT_SUCCESS;
}

nosResult DestroyBuffer(nosCudaBufferInfo* cudaBuffer)
{
	CHECK_CONTEXT_SWITCH();

	CHECK_VALID_ARGUMENT(cudaBuffer);

	CUresult driverRes = CUDA_SUCCESS;
	cudaError rtRes = cudaSuccess;
	if (cudaBuffer->Address != NULL)
	{
		if (!cudaBuffer->IsImported)
		{
			if (cudaBuffer->ShareInfo.CreateHandle != NULL)
			{
				driverRes = cuMemUnmap(cudaBuffer->Address, cudaBuffer->BlockSize);
				CHECK_CUDA_DRIVER_ERROR(driverRes);
				driverRes = cuMemRelease(cudaBuffer->ShareInfo.CreateHandle);
				CHECK_CUDA_DRIVER_ERROR(driverRes);
				driverRes = cuMemAddressFree(cudaBuffer->Address, cudaBuffer->BlockSize);
				CHECK_CUDA_DRIVER_ERROR(driverRes);
#if defined(_WIN32)
				if (cudaBuffer->ShareInfo.ShareableHandle != NULL)
				{
					auto closeRes = CloseHandle(reinterpret_cast<HANDLE>(cudaBuffer->ShareInfo.ShareableHandle));
					if (!closeRes)
					{
						nosEngine.LogW("Failed to close exported CUDA shareable handle (win32 error: %lu).",
						               GetLastError());
					}
				}
#endif
			}
			else if (cudaBuffer->MemoryType == NOS_CUDA_MEMORY_TYPE_MANAGED || cudaBuffer->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE)
			{
				rtRes = cudaFree(reinterpret_cast<void*>(cudaBuffer->Address));
				CHECK_CUDA_RT_ERROR(rtRes);
			}
			else if (cudaBuffer->MemoryType == NOS_CUDA_MEMORY_TYPE_HOST)
			{
				rtRes = cudaFreeHost(reinterpret_cast<void*>(cudaBuffer->Address));
				CHECK_CUDA_RT_ERROR(rtRes);
			}
		}
		else
		{
			rtRes = cudaFree(reinterpret_cast<void*>(cudaBuffer->Address));
			CHECK_CUDA_RT_ERROR(rtRes);

			rtRes = cudaDestroyExternalMemory(
				reinterpret_cast<cudaExternalMemory_t>(cudaBuffer->ImportedCudaExternalMemory));
			CHECK_CUDA_RT_ERROR(rtRes);
		}
	}

	memset(cudaBuffer, 0, sizeof(nosCudaBufferInfo)); //Let the users know

	return NOS_RESULT_SUCCESS;
}

FORCEINLINE nosResult ContextSwitch()
{
	nos::Name ID{};
	nosResult nosRes = GetCallingPluginId(&ID);
	if (nosRes != NOS_RESULT_SUCCESS)
		return nosRes;

	CUresult cuRes = CUDA_SUCCESS;
	if (IDContextMap.contains(ID))
	{
		ActiveContext = IDContextMap[ID];
		cuRes = cuCtxSetCurrent(reinterpret_cast<CUcontext>(ActiveContext));
		CHECK_CUDA_DRIVER_ERROR(cuRes);
	}
	else
	{
		CUcontext currentCtx = {};
		cuRes = cuCtxGetCurrent(&currentCtx);
		if (currentCtx == NULL)
		{
			cuRes = cuCtxSetCurrent(reinterpret_cast<CUcontext>(ActiveContext));
		}
		else
		{
			ActiveContext = currentCtx;
		}
	}
	return NOS_RESULT_SUCCESS;
}

FORCEINLINE nosResult GetCallingPluginId(nos::Name* ID)
{
	nosPluginInfo moduleContext = {};
	nosResult nosRes = nosEngine.GetCallingPlugin(&moduleContext);
	if (nosRes != NOS_RESULT_SUCCESS)
		return nosRes;
	(*ID) = moduleContext.Id.Name;
	return NOS_RESULT_SUCCESS;
}

StreamObject::StreamObject(cudaStream_t handle)
	: Handle(handle)
{
}

StreamObject::~StreamObject()
{
	CHECK_CONTEXT_SWITCH();
	cudaError res = cudaStreamDestroy(Handle);
	CHECK_CUDA_RT_ERROR_LOG_ONLY(res);
}

Result<std::unique_ptr<StreamObject>, nosResult> StreamObject::Create()
{
	CHECK_CONTEXT_SWITCH();
	cudaStream_t handle;
	cudaError res = cudaStreamCreate(&handle);
	CHECK_CUDA_RT_ERROR(res);
	auto ret = std::make_unique<StreamObject>(handle);
	return ret;
}

EngineBuffer StreamObject::Serialize() const
{
	return EngineBuffer::CopyFrom(nos::Buffer::From(nos::sys::cuda::TStream{}));
}

BufferObject::~BufferObject()
{
	CHECK_CONTEXT_SWITCH();
	if (Info.Address != 0)
	{
		ResManager.Remove(Info.Address);
		DestroyBuffer(&Info);
	}
}

Result<std::unique_ptr<BufferObject>, nosResult> BufferObject::Create(const nosCudaBufferCreateInfo& createInfo)
{
	CHECK_CONTEXT_SWITCH();
	auto obj = std::make_unique<BufferObject>();
	nosCudaBufferInfo info{};
	nosResult res = CreateBufferResource(createInfo, &info);

	if (res != NOS_RESULT_SUCCESS)
		return res;

	obj->Info = info;
	if (createInfo.Type == NOS_CUDA_BUFFER_CREATION_TYPE_IMPORTED)
	{
		obj->ExternalMemoryHandleType = createInfo.Imported.ExternalMemoryHandleType;
		obj->HasExternalMemoryHandleType = true;
	}
	else if (createInfo.Type == NOS_CUDA_BUFFER_CREATION_TYPE_DEVICE && createInfo.Device.Export)
	{
		obj->ExternalMemoryHandleType = NOS_CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUEWIN32;
		obj->HasExternalMemoryHandleType = true;
	}
	obj->ExternalMemoryPid = static_cast<uint64_t>(GetCurrentProcessId());
	return obj;
}

EngineBuffer BufferObject::Serialize() const
{
	nos::sys::cuda::Buffer buffer{};
	buffer.mutate_handle(Info.Address);
	buffer.mutate_offset(Info.Offset);
	buffer.mutate_size_in_bytes(Info.AllocationSize);
	buffer.mutate_element_type(ElementType);

	if (Info.IsImported || Info.ShareInfo.ShareableHandle != 0)
	{
		auto& extMem = buffer.mutable_external_memory();
		extMem.mutate_allocation_size(Info.AllocationSize);
		extMem.mutate_handle(Info.IsImported ? Info.ImportedExternalHandle
		                                                : Info.ShareInfo.ShareableHandle);
		extMem.mutate_pid(ExternalMemoryPid);
		if (HasExternalMemoryHandleType)
			extMem.mutate_handle_type(static_cast<uint32_t>(ExternalMemoryHandleType));
	}
	return EngineBuffer::CopyFrom(nos::Buffer::From(buffer));
}

namespace type
{
nosResult ConstructStreamObject(nosBuffer buffer,
                                nosForeignHandle* outForeignHandle,
                                nosBuffer* outSerializedData)
{
	auto res = StreamObject::Create();
	if (auto* err = res.Error())
		return *err;
	auto* streamObj = (*res).release();
	*outForeignHandle = streamObj;
	*outSerializedData = streamObj->Serialize().Release();
	return NOS_RESULT_SUCCESS;
}

void ReleaseStreamObject(nosForeignHandle foreignHandle)
{
	auto streamObj = static_cast<StreamObject*>(foreignHandle);
	delete streamObj;
}

nosResult InitializeStreamPinObject(nosFbShowAs showAs, nosUUID pinId, nosBuffer constructorBuffer)
{
	if (showAs == fb::ShowAs::OUTPUT_PIN)
	{
		nos::ObjectRef obj{};
		nosEngine.ObjectAPI->CreateForeignObject(NSN_StreamTypeName, constructorBuffer, &obj.GetStorage());
		return nosEngine.SetPinObject(pinId, obj);
	}
	return NOS_RESULT_SUCCESS;
}

void OnStreamInputPinDisconnected(const nosOnInputPinDisconnectedParams* params)
{
	nosEngine.SetPinObject(params->PinId, NOS_NULL_OBJECT);
}

nosResult ConstructBufferObject(nosBuffer buffer, nosForeignHandle* outForeignHandle, nosBuffer* outSerializedData)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(outForeignHandle);
	CHECK_VALID_ARGUMENT(outSerializedData);

	auto* fbBuf = ObjectData<nos::sys::cuda::Buffer>::Interpret(buffer);
	if (!fbBuf)
		return NOS_RESULT_INVALID_ARGUMENT;

	nosCudaBufferCreateInfo createInfo{};
	const auto& extMem = fbBuf->external_memory();
	const bool hasExternalMemory = extMem.handle() != 0;
	if (hasExternalMemory)
	{
		createInfo.Type = NOS_CUDA_BUFFER_CREATION_TYPE_IMPORTED;
		createInfo.Size = fbBuf->size_in_bytes();
		createInfo.Imported.ExternalMemoryHandle = extMem.handle();
		createInfo.Imported.ExternalMemoryHandleType =
			static_cast<nosCudaExternalMemoryHandleType>(extMem.handle_type());
		createInfo.Imported.AllocationSize = extMem.allocation_size();
		createInfo.Imported.Offset = fbBuf->offset();
	}
	else
	{
		createInfo.Type = NOS_CUDA_BUFFER_CREATION_TYPE_DEVICE;
		createInfo.Size = fbBuf->size_in_bytes();
		createInfo.Device.Export = NOS_FALSE;
	}
 
	auto res = BufferObject::Create(createInfo);
	if (auto* err = res.Error())
		return *err;

	auto* bufferObj = (*res).release();
	bufferObj->ElementType = fbBuf->element_type();
	bufferObj->ExternalMemoryPid = extMem.pid();
	if (hasExternalMemory)
		bufferObj->HasExternalMemoryHandleType = true;

	*outForeignHandle = bufferObj;
	*outSerializedData = bufferObj->Serialize().Release();
	return NOS_RESULT_SUCCESS;
}

void ReleaseBufferObject(nosForeignHandle foreignHandle)
{
	auto bufferObj = static_cast<BufferObject*>(foreignHandle);
	delete bufferObj;
}

nosResult InitializeBufferPinObject(nosFbShowAs showAs, nosUUID pinId, nosBuffer constructorBuffer)
{
	if (showAs == fb::ShowAs::OUTPUT_PIN)
	{
		nos::ObjectRef obj{};
		nosEngine.ObjectAPI->CreateForeignObject(NSN_BufferTypeName, constructorBuffer, &obj.GetStorage());
		if (auto* fbBuf = ObjectData<nos::sys::cuda::Buffer>::Interpret(constructorBuffer))
		{
			if (fbBuf->handle() != 0)
				ResManager.Add(fbBuf->handle(), obj.GetObjectId());
		}
		return nosEngine.SetPinObject(pinId, obj);
	}
	return NOS_RESULT_SUCCESS;
}

void OnBufferInputPinDisconnected(const nosOnInputPinDisconnectedParams* params)
{
	nosEngine.SetPinObject(params->PinId, NOS_NULL_OBJECT);
}
}
}
