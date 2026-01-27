// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include "Services.h"
#include <cstring>
#include "CUDASubsysCommon.h"
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

	subsys->CreateBufferOnCuda = CreateBufferOnCuda;
	subsys->CreateShareableBufferOnCuda = CreateShareableBufferOnCuda;
	subsys->CreateBufferOnManagedMemory = CreateBufferOnManagedMemory;
	subsys->CreateBufferPinned = CreateBufferPinned;
	subsys->CreateBuffer = CreateBuffer;
	subsys->CopyBuffers = CopyBuffers;
	subsys->CopyBuffersAsync = CopyBuffersAsync;
	subsys->GetCUDABufferFromAddress = GetCudaBufferFromAddress;

	subsys->DestroyBuffer = DestroyBuffer;

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

nosResult NOSAPI_CALL CopyBuffers(nosCudaBufferInfo* source, nosCudaBufferInfo* destination)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(source);
	CHECK_VALID_ARGUMENT(destination);

	if (source->MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED || destination->MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED)
	{
		nosEngine.LogE("Invalid memory type for CUDA memcopy operation.");
		return NOS_RESULT_FAILED;
	}
	if (source->CreateInfo.AllocationSize != destination->CreateInfo.AllocationSize)
	{
		nosEngine.LogW("nosCUDABuffers have size mismatch, trimming will be performed for copying.");
	}

	cudaError res = cudaSuccess;
	size_t safeCopySize = std::min(source->CreateInfo.AllocationSize, destination->CreateInfo.BlockSize);
	cudaMemcpyKind kind{};
	switch (source->MemoryType)
	{
	case NOS_CUDA_MEMORY_TYPE_HOST: {
		kind = destination->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyDeviceToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_DEVICE: {
		kind = destination->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyDeviceToDevice : cudaMemcpyDeviceToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_MANAGED: {
		kind = destination->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	default: return NOS_RESULT_INVALID_ARGUMENT;
	}
	res = cudaMemcpy(reinterpret_cast<void*>(destination->Address),
					 reinterpret_cast<void*>(source->Address),
					 safeCopySize,
					 kind);
	CHECK_CUDA_RT_ERROR(res);
	return NOS_RESULT_SUCCESS;
}

nosResult CopyBuffersAsync(nosCudaStreamObject stream, nosCudaBufferInfo* source, nosCudaBufferInfo* destination)
{
	CHECK_CONTEXT_SWITCH();
	CHECK_VALID_ARGUMENT(source);
	CHECK_VALID_ARGUMENT(destination);

	if (source->MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED || destination->MemoryType == NOS_CUDA_MEMORY_TYPE_UNREGISTERED)
	{
		nosEngine.LogE("Invalid memory type for CUDA memcopy operation.");
		return NOS_RESULT_FAILED;
	}
	if (source->CreateInfo.AllocationSize != destination->CreateInfo.AllocationSize)
	{
		nosEngine.LogW("nosCUDABuffers have size mismatch, trimming will be performed for copying.");
	}

	cudaError res = cudaSuccess;
	size_t safeCopySize = std::min(source->CreateInfo.AllocationSize, destination->CreateInfo.BlockSize);
	cudaMemcpyKind kind{};
	switch (source->MemoryType)
	{
	case NOS_CUDA_MEMORY_TYPE_HOST: {
		kind = destination->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_DEVICE: {
		kind = destination->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyDeviceToDevice : cudaMemcpyDeviceToHost;
		break;
	}
	case NOS_CUDA_MEMORY_TYPE_MANAGED: {
		kind = destination->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
		break;
	}
	default: return NOS_RESULT_INVALID_ARGUMENT;
	}
	auto streamObject = nos::GetForeignHandle<StreamObject>(stream);
	res = cudaMemcpyAsync(reinterpret_cast<void*>(destination->Address),
						  reinterpret_cast<void*>(source->Address),
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

nosResult NOSAPI_CALL CreateBufferOnCuda(nosCudaBufferInfo* cudaBuffer, uint64_t size)
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
	cudaBuffer->CreateInfo.BlockSize = size;
	cudaBuffer->CreateInfo.AllocationSize = size;
	cudaBuffer->CreateInfo.IsImported = false;
	ResManager.Add(addr, *cudaBuffer);
	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL CreateShareableBufferOnCuda(nosCudaBufferInfo* cudaBuffer, uint64_t size)
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
	CHECK_IS_SUPPORTED(supportsVMM, HANDLE_TYPE_WIN32_HANDLE);

	CUmemAllocationProp prop;

	memset(&prop, 0, sizeof(prop));

	prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
	prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
	prop.location.id = (int)dev;
	prop.win32HandleMetaData = Descriptor::SecurityDescriptor::GetDefaultSecurityDescriptor();
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

	cudaBuffer->CreateInfo.BlockSize = aligned_size;
	cudaBuffer->CreateInfo.AllocationSize = size;
	cudaBuffer->ShareInfo.CreateHandle = allocHandle;
	cudaBuffer->ShareInfo.ShareableHandle = shareableHandle;
	cudaBuffer->ShareInfo.ReferenceCount = 1;
	cudaBuffer->Address = address;
	cudaBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_DEVICE;
	ResManager.Add(address, *cudaBuffer);
	return NOS_RESULT_SUCCESS;

}

nosResult NOSAPI_CALL CreateBufferOnManagedMemory(nosCudaBufferInfo* cudaBuffer, uint64_t size)
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
	ResManager.Add(addr, *cudaBuffer);

	return NOS_RESULT_SUCCESS;
}

nosResult NOSAPI_CALL CreateBufferPinned(nosCudaBufferInfo* cudaBuffer, uint64_t size)
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
	ResManager.Add(addr, *cudaBuffer);

	return nosResult();
}

nosResult InitBuffer(void* source, uint64_t size, nosCudaMemoryType type, nosCudaBufferInfo* destination)
{
	destination->Address = reinterpret_cast<uint64_t>(source);
	destination->CreateInfo.AllocationSize = size;
	destination->ShareInfo.CreateHandle = NULL;
	destination->ShareInfo.ReferenceCount = 0;
	destination->ShareInfo.ShareableHandle = NULL;
	destination->MemoryType = type;
	ResManager.Add(destination->Address, *destination);

	return NOS_RESULT_SUCCESS;
}

nosResult CreateBuffer(nosCudaBufferInfo* cudaBuffer, uint64_t size)
{
	void* data = malloc(size);

	cudaBuffer->Address = reinterpret_cast<uint64_t>(data);
	cudaBuffer->ShareInfo.ShareableHandle = NULL;
	cudaBuffer->ShareInfo.CreateHandle = NULL;
	cudaBuffer->ShareInfo.ReferenceCount = 0;
	cudaBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_HOST;
	cudaBuffer->CreateInfo.AllocationSize = size;
	cudaBuffer->CreateInfo.BlockSize = size;
	cudaBuffer->CreateInfo.IsImported = false;
	ResManager.Add(cudaBuffer->Address, *cudaBuffer);

	return NOS_RESULT_SUCCESS;
}

nosResult GetCudaBufferFromAddress(uint64_t address, nosCudaBufferInfo* outBuffer)
{
	void* res = ResManager.Get(address);
	if (res == nullptr)
		return NOS_RESULT_FAILED;
	(*outBuffer) = *reinterpret_cast<nosCudaBufferInfo*>(res);

	return NOS_RESULT_SUCCESS;
}

nosResult ImportExternalMemoryAsCudaBuffer(uint64_t Handle,
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

	uint64_t outCudaPointerAddres = NULL;
	outBuffer->Address = reinterpret_cast<uint64_t>(pointer);
	outBuffer->CreateInfo.BlockSize = BlockSize;
	outBuffer->CreateInfo.AllocationSize = AllocationSize;
	outBuffer->CreateInfo.Offset = Offset;
	outBuffer->CreateInfo.IsImported = true;
	outBuffer->CreateInfo.ImportedInternalHandle = reinterpret_cast<uint64_t>(externalMemory);
	outBuffer->CreateInfo.ImportedExternalHandle = Handle;
	outBuffer->ShareInfo.CreateHandle = NULL;
	outBuffer->ShareInfo.ShareableHandle = NULL;
	outBuffer->ShareInfo.ReferenceCount = 0;
	outBuffer->MemoryType = NOS_CUDA_MEMORY_TYPE_DEVICE;
	ResManager.Add(outBuffer->Address, *outBuffer);
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

nosResult NOSAPI_CALL DestroyBuffer(nosCudaBufferInfo* cudaBuffer)
{
	CHECK_CONTEXT_SWITCH();

	CHECK_VALID_ARGUMENT(cudaBuffer);

	CUresult driverRes = CUDA_SUCCESS;
	cudaError rtRes = cudaSuccess;
	if (cudaBuffer->Address != NULL)
	{
		if (!cudaBuffer->CreateInfo.IsImported)
		{
			if (cudaBuffer->ShareInfo.CreateHandle != NULL)
			{
				driverRes = cuMemUnmap(cudaBuffer->Address, cudaBuffer->CreateInfo.BlockSize);
				CHECK_CUDA_DRIVER_ERROR(driverRes);
				driverRes = cuMemRelease(cudaBuffer->ShareInfo.CreateHandle);
				CHECK_CUDA_DRIVER_ERROR(driverRes);
				driverRes = cuMemAddressFree(cudaBuffer->Address, cudaBuffer->CreateInfo.BlockSize);
				CHECK_CUDA_DRIVER_ERROR(driverRes);
			}
			else if (cudaBuffer->MemoryType == NOS_CUDA_MEMORY_TYPE_MANAGED || cudaBuffer->MemoryType == NOS_CUDA_MEMORY_TYPE_DEVICE)
			{
				rtRes = cudaFree(reinterpret_cast<void*>(cudaBuffer->Address));
				CHECK_CUDA_RT_ERROR(rtRes);
			}
			else if (cudaBuffer->MemoryType == NOS_CUDA_MEMORY_TYPE_HOST)
			{
				free(reinterpret_cast<void*>(cudaBuffer->Address));
			}
		}
		else
		{
			rtRes = cudaFree(reinterpret_cast<void*>(cudaBuffer->Address));
			CHECK_CUDA_RT_ERROR(rtRes);

			rtRes = cudaDestroyExternalMemory(
				reinterpret_cast<cudaExternalMemory_t>(cudaBuffer->CreateInfo.ImportedInternalHandle));
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
}
}