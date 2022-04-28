/*******************************************************************
* Copyright (C) 2010-2022 Xilinx, Inc. All rights reserved.*
* SPDX-License-Identifier: MIT
*******************************************************************************/


#include "xparameters.h"
#include "xcsiss.h"

/*
* List of Sub-cores included in the subsystem
* Sub-core device id will be set by its driver in xparameters.h
*/

#define XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_CSI2_RX_CTRL_0_PRESENT	 1
#define XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_DPHY_0_PRESENT	 1


/*
* List of Sub-cores excluded from the subsystem
*   - Excluded sub-core device id is set to 255
*   - Excluded sub-core baseaddr is set to 0
*/

#define XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_AXI_IIC_0_PRESENT 0
#define XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_AXI_IIC_0_DEVICE_ID 255
#define XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_AXI_IIC_0_BASEADDR 0

XCsiSs_Config XCsiSs_ConfigTable[] =
{
	{
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_DEVICE_ID,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_BASEADDR,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_HIGHADDR,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CMN_INC_IIC,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CMN_NUM_LANES,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CMN_NUM_PIXELS,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CMN_PXL_FORMAT,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CMN_VC,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CSI_BUF_DEPTH,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CSI_EMB_NON_IMG,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_DPY_EN_REG_IF,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_DPY_LINE_RATE,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CSI_EN_CRC,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_CSI_EN_ACTIVELANES,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_EN_CSI_V2_0,
		XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_EN_VCX,
		{
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_AXI_IIC_0_PRESENT,
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_AXI_IIC_0_DEVICE_ID,
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_AXI_IIC_0_BASEADDR
		},
		{
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_CSI2_RX_CTRL_0_PRESENT,
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_CSI2_RX_CTRL_0_DEVICE_ID,
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_CSI2_RX_CTRL_0_BASEADDR
		},
		{
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_DPHY_0_PRESENT,
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_DPHY_0_DEVICE_ID,
			XPAR_MIPI_CSI2_RX_SUBSYSTEM_0_MIPI_DPHY_0_BASEADDR
		},
	}
};
