/******************************************************************************
* Copyright (C) 2022 Xilinx, Inc.  All rights reserved.
* Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
*
* @file xdfeofdm_hw.h
* @addtogroup dfeofdm Overview
* @{
*
* Contains the register definitions for xdfeofdm.
*
* The general naming format is as follows:
* XDFEOFDM_<register group>_<register_name>_...
* In the case where a group has only one register,
* <register group> is omitted leaving:
* XDFEOFDM_<register_name>_...
* see below for how the ellipis is populated.
*
* Register offsets:
* XDFEOFDM_<register name>_OFFSET
* e.g. XDFEOFDM_STATE_OPERATIONAL_OFFSET
* This offset is relative to the base address of the IP.
*
* Bitfield offsets:
* XDFEOFDM_<register name>_<bitfield name>_OFFSET
* e.g. XDFEOFDM_STATE_OPERATIONAL_BF_OFFSET
* This offset is relative to the register address.
*
* Bitfield widths:
* XDFEOFDM_<register name>_<bitfield name>_WIDTH
* e.g. XDFEOFDM_STATE_OPERATIONAL_BF_WIDTH
* This is the width of the bitfield in bits.
*
* Bitfield values:
* XDFEOFDM_<register name>_<bitfield name>_<function>
* e.g. XDFEOFDM_STATE_OPERATIONAL_BF_YES
* This value is relative to the bitfield start address.
*
* Note:
* In the case where a bitfield fieled/width/value is common to a set of
* registers, the highest level naming is used.
* e.g. XDFEOFDM_TRIGGERS_SOURCE_OFFSET applies to all
*      XDFEOFDM_TRIGGERS_<register>_OFFSET registers, hence
*      the <register> specifier is removed.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---    -------- -----------------------------------------------
* 1.0   dc     11/21/22 Initial version
*
* </pre>
*
******************************************************************************/
#ifndef XDFEOFDM_HW_H_
#define XDFEOFDM_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

/**************************** Definitions *******************************/
/* CORE functionality */
/* Version */
#define XDFEOFDM_VERSION_OFFSET 0x00U /**< Register offset */
#define XDFEOFDM_VERSION_PATCH_OFFSET 0U
#define XDFEOFDM_VERSION_PATCH_WIDTH 8U
#define XDFEOFDM_VERSION_REVISION_OFFSET 8U
#define XDFEOFDM_VERSION_REVISION_WIDTH 8U
#define XDFEOFDM_VERSION_MINOR_OFFSET 16U
#define XDFEOFDM_VERSION_MINOR_WIDTH 8U
#define XDFEOFDM_VERSION_MAJOR_OFFSET 24U
#define XDFEOFDM_VERSION_MAJOR_WIDTH 8U

/* Reset */
#define XDFEOFDM_RESET_OFFSET 0x04U /**< Register offset */
#define XDFEOFDM_RESET_BF_OFFSET 0U
#define XDFEOFDM_RESET_BF_WIDTH 1U
#define XDFEOFDM_RESET_OFF 0U
#define XDFEOFDM_RESET_ON 1U

/* Model_Param */
#define XDFEOFDM_MODEL_PARAM_OFFSET 0x08U /**< Register offset */
#define XDFEOFDM_MODEL_PARAM_RESERVED_OFFSET 0U
#define XDFEOFDM_MODEL_PARAM_RESERVED_WIDTH 1U
#define XDFEOFDM_MODEL_PARAM_NUM_ANTENNA_OFFSET 4U
#define XDFEOFDM_MODEL_PARAM_NUM_ANTENNA_WIDTH 4U
#define XDFEOFDM_MODEL_PARAM_ANTENNA_INTERLEAVE_OFFSET 16U
#define XDFEOFDM_MODEL_PARAM_ANTENNA_INTERLEAVE_WIDTH 4U

/* Delays */
#define XDFEOFDM_DELAY_OFFSET 0x10U /**< Register offset */
#define XDFEOFDM_DELAY_BF_OFFSET 0U
#define XDFEOFDM_DELAY_BF_WIDTH 12U
#define XDFEOFDM_DATA_LATENCY_OFFSET 0x14U /**< Register offset */
#define XDFEOFDM_DATA_LATENCY_BF_OFFSET 0U
#define XDFEOFDM_DATA_LATENCY_BF_WIDTH 16U

/* State */
#define XDFEOFDM_STATE_OPERATIONAL_OFFSET 0x20U /**< Register offset */
#define XDFEOFDM_STATE_OPERATIONAL_BF_OFFSET 0U
#define XDFEOFDM_STATE_OPERATIONAL_BF_WIDTH 1U
#define XDFEOFDM_STATE_OPERATIONAL_NO 0U
#define XDFEOFDM_STATE_OPERATIONAL_YES 1U
#define XDFEOFDM_STATE_LOW_POWER_OFFSET 0x24U /**< Register offset */
#define XDFEOFDM_STATE_LOW_POWER_BF_OFFSET 0U
#define XDFEOFDM_STATE_LOW_POWER_BF_WIDTH 1U
#define XDFEOFDM_STATE_LOW_POWER_BF_NO 0U
#define XDFEOFDM_STATE_LOW_POWER_BF_YES 1U

/* Triggers */
#define XDFEOFDM_TRIGGERS_ACTIVATE_OFFSET 0x30U /**< Register offset */
#define XDFEOFDM_TRIGGERS_LOW_POWER_OFFSET 0x38U /**< Register offset */
#define XDFEOFDM_TRIGGERS_CC_UPDATE_OFFSET 0x3CU /**< Register offset */
/* Bit fields */
#define XDFEOFDM_TRIGGERS_TRIGGER_ENABLE_WIDTH 1U
#define XDFEOFDM_TRIGGERS_TRIGGER_ENABLE_OFFSET 0U
#define XDFEOFDM_TRIGGERS_TRIGGER_ENABLE_DISABLED 0U
#define XDFEOFDM_TRIGGERS_TRIGGER_ENABLE_ENABLED 1U
#define XDFEOFDM_TRIGGERS_MODE_WIDTH 2U
#define XDFEOFDM_TRIGGERS_MODE_OFFSET 4U
#define XDFEOFDM_TRIGGERS_MODE_IMMEDIATE 0U
#define XDFEOFDM_TRIGGERS_MODE_TUSER_SINGLE_SHOT 1U
#define XDFEOFDM_TRIGGERS_MODE_TUSER_CONTINUOUS 2U
#define XDFEOFDM_TRIGGERS_MODE_RESERVED 3U
#define XDFEOFDM_TRIGGERS_TUSER_EDGE_LEVEL_WIDTH 2U
#define XDFEOFDM_TRIGGERS_TUSER_EDGE_LEVEL_OFFSET 8U
#define XDFEOFDM_TRIGGERS_TUSER_EDGE_LEVEL_LOW 0U
#define XDFEOFDM_TRIGGERS_TUSER_EDGE_LEVEL_HIGH 1U
#define XDFEOFDM_TRIGGERS_TUSER_EDGE_LEVEL_FALING 2U
#define XDFEOFDM_TRIGGERS_TUSER_EDGE_LEVEL_RISING 3U
#define XDFEOFDM_TRIGGERS_STATE_OUTPUT_WIDTH 1U
#define XDFEOFDM_TRIGGERS_STATE_OUTPUT_OFFSET 12U
#define XDFEOFDM_TRIGGERS_STATE_OUTPUT_DISABLED 0U
#define XDFEOFDM_TRIGGERS_STATE_OUTPUT_ENABLED 1U
#define XDFEOFDM_TRIGGERS_TUSER_BIT_WIDTH 8U
#define XDFEOFDM_TRIGGERS_TUSER_BIT_OFFSET 16U

/* IRQ status */
#define XDFEOFDM_STATUS_ISR_OFFSET 0x40U /**< Register offset */
#define XDFEOFDM_ISR_HAS_NOT_OCCURRED 0U
#define XDFEOFDM_ISR_OCCURRED 1U
#define XDFEOFDM_ISR_CLEAR 1U
#define XDFEOFDM_STATUS_IER_OFFSET 0x44U /**< Register offset */
#define XDFEOFDM_IER_NO_CHANGE 0U
#define XDFEOFDM_IER_SET_BIT 1U
#define XDFEOFDM_STATUS_IDR_OFFSET 0x48U /**< Register offset */
#define XDFEOFDM_IDR_NO_CHANGE 0U
#define XDFEOFDM_IDR_RESET_BIT 1U
#define XDFEOFDM_STATUS_IMR_OFFSET 0x4CU /**< Register offset */
#define XDFEOFDM_IMR_INTERRUPT 0U
#define XDFEOFDM_IMR_NO_INTERRUPT 1U

#define XDFEOFDM_STATUS_CC_UPDATE_TRIGGERED_OFFSET 0U
#define XDFEOFDM_STATUS_CC_UPDATE_TRIGGERED_WIDTH 1U
#define XDFEOFDM_CC_UPDATE_TRIGGERED_LOW 0U
#define XDFEOFDM_CC_UPDATE_TRIGGERED_HIGH 1U
#define XDFEOFDM_STATUS_FT_CC_SEQUENCE_ERROR_OFFSET 1U
#define XDFEOFDM_STATUS_FT_CC_SEQUENCE_ERROR_WIDTH 1U
#define XDFEOFDM_STATUS_BF_SATURATION_OFFSET 2U
#define XDFEOFDM_STATUS_BF_SATURATION_WIDTH 1U
#define XDFEOFDM_STATUS_NO_EVENT 0U
#define XDFEOFDM_STATUS_EVENT 1U
#define XDFEOFDM_IRQ_FLAGS_MASK 0x7U

/* Saturation Events */
#define XDFEOFDM_STATUS_SATURATION_OFFSET 0x50U /**< Register offset */
#define XDFEOFDM_STATUS_SATURATION_CCID_OFFSET 0U
#define XDFEOFDM_STATUS_SATURATION_CCID_WIDTH 4U
#define XDFEOFDM_STATUS_SATURATION_SATURATION_COUNT_OFFSET 4U
#define XDFEOFDM_STATUS_SATURATION_SATURATION_COUNT_WIDTH 14U

/* TUSER Framing bit Location register */
#define XDFEOFDM_TUSER_FRAME_LOC_OFFSET 0x60U /**< Register offset */
#define XDFEOFDM_FRAME_BIT_OFFSET 0U
#define XDFEOFDM_FRAME_BIT_WIDTH 8U

/* FT Sequence */
#define XDFEOFDM_FT_SEQUENCE_LENGTH_CURRENT_OFFSET                             \
	0x1000U /**< Register offset */
#define XDFEOFDM_FT_SEQUENCE_LENGTH_CURRENT_BF_OFFSET 0U
#define XDFEOFDM_FT_SEQUENCE_LENGTH_NEXT_OFFSET 0x1004U /**< Register offset */
#define XDFEOFDM_FT_SEQUENCE_LENGTH_NEXT_BF_OFFSET 0U
#define XDFEOFDM_FT_SEQUENCE_LENGTH_WIDTH 4U

#define XDFEOFDM_FT_SEQUENCE_CURRENT_OFFSET(X)                                 \
	(0x1100U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_FT_SEQUENCE_NEXT_OFFSET(X)                                    \
	(0x1140U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_FT_SEQUENCE_OFFSET 0U
#define XDFEOFDM_FT_SEQUENCE_WIDTH 4U

/* CC Sequence Length */
#define XDFEOFDM_CC_SEQUENCE_LENGTH_CURRENT_OFFSET                             \
	0x1200U /**< Register offset */
#define XDFEOFDM_CC_SEQUENCE_LENGTH_NEXT_OFFSET 0x1204U /**< Register offset */
#define XDFEOFDM_CC_SEQUENCE_LENGTH_WIDTH 4U

/* CC Sequence */
#define XDFEOFDM_CC_SEQUENCE_CURRENT_OFFSET(X)                                 \
	(0x1300U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_CC_SEQUENCE_NEXT_OFFSET(X)                                    \
	(0x1340U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_CC_SEQUENCE_OFFSET 0U
#define XDFEOFDM_CC_SEQUENCE_WIDTH 4U

/* FT Carrier Config */
#define XDFEOFDM_CARRIER_CONFIGURATION1_CURRENT_OFFSET(X)                      \
	(0x1400U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_CARRIER_CONFIGURATION1_NEXT_OFFSET(X)                         \
	(0x1440U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_CARRIER_CONFIGURATION1_ENABLE_OFFSET 0U
#define XDFEOFDM_CARRIER_CONFIGURATION1_ENABLE_WIDTH 1U
#define XDFEOFDM_CARRIER_CONFIGURATION1_ENABLE_DISABLED 0U
#define XDFEOFDM_CARRIER_CONFIGURATION1_ENABLE_ENABLED 1U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_OFFSET 1U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_WIDTH 3U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_15kHz 0U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_30kHz 1U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_60kHz 2U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_120kHz 3U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_240kHz 4U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_480kHz 5U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUMEROLOGY_960kHz 6U
#define XDFEOFDM_CARRIER_CONFIGURATION1_FFT_SIZE_OFFSET 4U
#define XDFEOFDM_CARRIER_CONFIGURATION1_FFT_SIZE_WIDTH 4U
#define XDFEOFDM_CARRIER_CONFIGURATION1_FFT_SIZE_1024 0xAU
#define XDFEOFDM_CARRIER_CONFIGURATION1_FFT_SIZE_2048 0xBU
#define XDFEOFDM_CARRIER_CONFIGURATION1_FFT_SIZE_4096 0xCU
#define XDFEOFDM_FFT_SIZE_1024 1024U
#define XDFEOFDM_FFT_SIZE_2048 2048U
#define XDFEOFDM_FFT_SIZE_4096 4096U

#define XDFEOFDM_CARRIER_CONFIGURATION1_NUM_SUBCARRIERS_OFFSET 8U
#define XDFEOFDM_CARRIER_CONFIGURATION1_NUM_SUBCARRIERS_WIDTH 12U
#define XDFEOFDM_CARRIER_CONFIGURATION1_SCALE_FACTOR_OFFSET 20U
#define XDFEOFDM_CARRIER_CONFIGURATION1_SCALE_FACTOR_WIDTH 10U
#define XDFEOFDM_CARRIER_CONFIGURATION1_COMMS_STANDARD_OFFSET 31U
#define XDFEOFDM_CARRIER_CONFIGURATION1_COMMS_STANDARD_WIDTH 1U
#define XDFEOFDM_CARRIER_CONFIGURATION1_COMMS_STANDARD_5G_NR 0U
#define XDFEOFDM_CARRIER_CONFIGURATION1_COMMS_STANDARD_LTE 1U

/* CC Carrier Config */
#define XDFEOFDM_CARRIER_CONFIGURATION2_CURRENT_OFFSET(X)                      \
	(0x1500U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_CARRIER_CONFIGURATION2_NEXT_OFFSET(X)                         \
	(0x1540U + (X << 2U)) /**< Register offset */
#define XDFEOFDM_CARRIER_CONFIGURATION2_OUTPUT_DELAY_OFFSET 0U
#define XDFEOFDM_CARRIER_CONFIGURATION2_OUTPUT_DELAY_WIDTH 16U

#ifdef __cplusplus
}
#endif

#endif
/**
* @endcond
*/
/** @} */
