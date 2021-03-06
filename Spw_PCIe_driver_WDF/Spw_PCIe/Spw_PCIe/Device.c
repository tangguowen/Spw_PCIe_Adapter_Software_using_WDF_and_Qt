#include "driver.h"
#include "device.tmh"

#pragma warning(disable:4013)  // assuming extern returning int
#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, Spw_PCIeCreateDevice)
//#pragma alloc_text(PAGE, Spw_PCIeEvtDriverContextCleanup)

#pragma alloc_text(PAGE, Spw_PCIeEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, Spw_PCIeEvtDeviceReleaseHardware)
#pragma alloc_text(PAGE, Spw_PCIeEvtDeviceD0Entry)
#pragma alloc_text(PAGE, Spw_PCIeEvtDeviceD0Exit)
//#pragma alloc_text(PAGE, Spw_PCIeEvtIoWrite)

#endif

NTSTATUS
Spw_PCIeEvtDevicePrepareHardware(
IN WDFDEVICE Device,
IN WDFCMRESLIST ResourceList,
IN WDFCMRESLIST ResourceListTranslated
)
{
	ULONG		    i;
	NTSTATUS        status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDeviceContext;
	/*
	BOOLEAN                             foundPort      = FALSE;
    PHYSICAL_ADDRESS                    portBasePA     = {0};
    ULONG                               portCount      = 0;
    WDF_DMA_ENABLER_CONFIG              dmaConfig;
	*/
	PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;//record the Hareware resource that OS dispatched to PCIe


	/*
	在Windows驱动开发中，PCM_PARTIAL_RESOURCE_DESCRIPTOR记录了为PCI设备分配的硬件资源，
	可能有CmResourceTypePort, CmResourceTypeMemory等，
	后者表示一段memory地址空间，顾名思义，是通过memory space访问的，
	前者表示一段I/O地址空间，但其flag有CM_RESOURCE_PORT_MEMORY和CM_RESOURCE_PORT_IO两种，
	分别表示通过memory space访问以及通过I/O space访问，这就是PCI请求与实际分配的差异，
	在x86下，CmResourceTypePort的flag都是CM_RESOURCE_PORT_IO，即表明PCI设备请求的是I/O地址空间，分配的也是I/O地址空间，
	而在ARM或Alpha等下，flag是CM_RESOURCE_PORT_MEMORY，表明即使PCI请求的I/O地址空间，但分配在了memory space，
	我们需要通过memory space访问I/O设备(通过MmMapIoSpace映射物理地址空间到虚拟地址空间，当然，是内核的虚拟地址空间，这样驱动就可以正常访问设备了)。
	*/
	PAGED_CODE();

//	UNREFERENCED_PARAMETER(Resources);//告诉编译器不要发出Resources没有被引用的警告


	pDeviceContext = GetDeviceContext(Device);
	pDeviceContext->MemBaseAddress = NULL;
	pDeviceContext->Counter_i = 0;
	//get resource
	for (i = 0; i < WdfCmResourceListGetCount(ResourceListTranslated); i++) {

		descriptor = WdfCmResourceListGetDescriptor(ResourceListTranslated, i);
		//if failed：
		if (!descriptor) {
			return STATUS_DEVICE_CONFIGURATION_ERROR;
		}

		switch (descriptor->Type) {

		case CmResourceTypeMemory:
			//MmMapIoSpace将物理地址转换成系统内核模式地址
			if (i == 0){
				pDeviceContext->PhysicalAddressRegister = descriptor->u.Memory.Start.LowPart;
				pDeviceContext->BAR0_VirtualAddress = MmMapIoSpace(
					descriptor->u.Memory.Start,
					descriptor->u.Memory.Length,
					MmNonCached);
			}
			
			pDeviceContext->MemBaseAddress = MmMapIoSpace(
				descriptor->u.Memory.Start,
				descriptor->u.Memory.Length,
				MmNonCached);
			pDeviceContext->MemLength = descriptor->u.Memory.Length;

			break;

		default:
			break;
		}
		if (!pDeviceContext->MemBaseAddress){
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	pDeviceContext->Counter_i = i;
	DbgPrint("EvtDevicePrepareHardware - ends\n");

	return STATUS_SUCCESS;
}

NTSTATUS
Spw_PCIeEvtDeviceReleaseHardware(
IN WDFDEVICE Device,
IN WDFCMRESLIST ResourceListTranslated
)
{
	PDEVICE_CONTEXT	pDeviceContext = NULL;

	PAGED_CODE();

	DbgPrint("EvtDeviceReleaseHardware - begins\n");

	pDeviceContext = GetDeviceContext(Device);

	if (pDeviceContext->MemBaseAddress) {
		//MmUnmapIoSpace解除物理地址与系统内核模式地址的关联
		MmUnmapIoSpace(pDeviceContext->MemBaseAddress, pDeviceContext->MemLength);
		pDeviceContext->MemBaseAddress = NULL;
	}

	DbgPrint("EvtDeviceReleaseHardware - ends\n");

	return STATUS_SUCCESS;
}

NTSTATUS
Spw_PCIeEvtDeviceD0Entry(
IN  WDFDEVICE Device,
IN  WDF_POWER_DEVICE_STATE PreviousState
)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(PreviousState);

	return STATUS_SUCCESS;
}


NTSTATUS
Spw_PCIeEvtDeviceD0Exit(
IN  WDFDEVICE Device,
IN  WDF_POWER_DEVICE_STATE TargetState
)
{
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(TargetState);

	PAGED_CODE();

	return STATUS_SUCCESS;
}
