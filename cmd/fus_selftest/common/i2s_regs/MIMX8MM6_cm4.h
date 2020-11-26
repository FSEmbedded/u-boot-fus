/*
** ###################################################################
**     Processors:          MIMX8MM6CVTKZ
**                          MIMX8MM6DVTLZ
**
**     Compilers:           GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          Keil ARM C/C++ Compiler
**
**     Reference manual:    MX8MMRM, Rev. 0, 02/2019
**     Version:             rev. 4.0, 2019-02-18
**     Build:               b190228
**
**     Abstract:
**         CMSIS Peripheral Access Layer for MIMX8MM6_cm4
**
**     Copyright 1997-2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2019 NXP
**     All rights reserved.
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2018-03-26)
**         Initial version.
**     - rev. 2.0 (2018-07-20)
**         Rev.A Header EAR
**     - rev. 3.0 (2018-10-24)
**         Rev.B Header PRC
**     - rev. 4.0 (2019-02-18)
**         Rev.0 Header RFP
**
** ###################################################################
*/

/*!
 * @file MIMX8MM6_cm4.h
 * @version 4.0
 * @date 2019-02-18
 * @brief CMSIS Peripheral Access Layer for MIMX8MM6_cm4
 *
 * CMSIS Peripheral Access Layer for MIMX8MM6_cm4
 */

#ifndef _MIMX8MM6_CM4_H_
#define _MIMX8MM6_CM4_H_                         /**< Symbol preventing repeated inclusion */

/* ----------------------------------------------------------------------------
   -- I2S Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2S_Peripheral_Access_Layer I2S Peripheral Access Layer
 * @{
 */

/** I2S - Register Layout Typedef */
typedef volatile struct {
	volatile uint32_t VERID;                             /**< Version ID Register, offset: 0x0 */
	volatile uint32_t PARAM;                             /**< Parameter Register, offset: 0x4 */
	volatile uint32_t TCSR;                              /**< SAI Transmit Control Register, offset: 0x8 */
	volatile uint32_t TCR1;                              /**< SAI Transmit Configuration 1 Register, offset: 0xC */
	volatile uint32_t TCR2;                              /**< SAI Transmit Configuration 2 Register, offset: 0x10 */
	volatile uint32_t TCR3;                              /**< SAI Transmit Configuration 3 Register, offset: 0x14 */
	volatile uint32_t TCR4;                              /**< SAI Transmit Configuration 4 Register, offset: 0x18 */
	volatile uint32_t TCR5;                              /**< SAI Transmit Configuration 5 Register, offset: 0x1C */
	volatile uint32_t TDR[8];                            /**< SAI Transmit Data Register, array offset: 0x20, array step: 0x4 */
	volatile uint32_t TFR[8];                            /**< SAI Transmit FIFO Register, array offset: 0x40, array step: 0x4 */
	volatile uint32_t TMR;                               /**< SAI Transmit Mask Register, offset: 0x60 */
	volatile  uint8_t RESERVED_0[12];
	volatile uint32_t TTCR;                              /**< SAI Transmit Timestamp Control Register, offset: 0x70 */
	volatile uint32_t TTSR;                              /**< SAI Transmit Timestamp Register, offset: 0x74 */
	volatile uint32_t TBCR;                              /**< SAI Transmit Bit Count Register, offset: 0x78 */
	volatile uint32_t TBCTR;                             /**< SAI Transmit Bit Count Timestamp Register, offset: 0x7C */
	volatile uint8_t RESERVED_1[8];
	volatile  uint32_t RCSR;                              /**< SAI Receive Control Register, offset: 0x88 */
	volatile uint32_t RCR1;                              /**< SAI Receive Configuration 1 Register, offset: 0x8C */
	volatile uint32_t RCR2;                              /**< SAI Receive Configuration 2 Register, offset: 0x90 */
	volatile uint32_t RCR3;                              /**< SAI Receive Configuration 3 Register, offset: 0x94 */
	volatile uint32_t RCR4;                              /**< SAI Receive Configuration 4 Register, offset: 0x98 */
	volatile uint32_t RCR5;                              /**< SAI Receive Configuration 5 Register, offset: 0x9C */
	volatile uint32_t RDR[8];                            /**< SAI Receive Data Register, array offset: 0xA0, array step: 0x4 */
	volatile uint32_t RFR[8];                            /**< SAI Receive FIFO Register, array offset: 0xC0, array step: 0x4 */
	volatile uint32_t RMR;                               /**< SAI Receive Mask Register, offset: 0xE0 */
	volatile uint8_t RESERVED_2[12];
	volatile uint32_t RTCR;                              /**< SAI Receive Timestamp Control Register, offset: 0xF0 */
	volatile uint32_t RTSR;                              /**< SAI Receive Timestamp Register, offset: 0xF4 */
	volatile uint32_t RBCR;                              /**< SAI Receive Bit Count Register, offset: 0xF8 */
	volatile uint32_t RBCTR;                             /**< SAI Receive Bit Count Timestamp Register, offset: 0xFC */
	volatile uint32_t MCR;                               /**< SAI MCLK Control Register, offset: 0x100 */
} I2S_Type;

/* ----------------------------------------------------------------------------
   -- I2S Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2S_Register_Masks I2S Register Masks
 * @{
 */

/*! @name VERID - Version ID Register */
/*! @{ */
#define I2S_VERID_FEATURE_MASK                   (0xFFFFU)
#define I2S_VERID_FEATURE_SHIFT                  (0U)
/*! FEATURE - Feature Specification Number
 *  0b0000000000000000..Standard feature set.
 *  0b0000000000000010..Standard feature set with Timestamp Registers.
 */
#define I2S_VERID_FEATURE(x)                     (((uint32_t)(((uint32_t)(x)) << I2S_VERID_FEATURE_SHIFT)) & I2S_VERID_FEATURE_MASK)
#define I2S_VERID_MINOR_MASK                     (0xFF0000U)
#define I2S_VERID_MINOR_SHIFT                    (16U)
#define I2S_VERID_MINOR(x)                       (((uint32_t)(((uint32_t)(x)) << I2S_VERID_MINOR_SHIFT)) & I2S_VERID_MINOR_MASK)
#define I2S_VERID_MAJOR_MASK                     (0xFF000000U)
#define I2S_VERID_MAJOR_SHIFT                    (24U)
#define I2S_VERID_MAJOR(x)                       (((uint32_t)(((uint32_t)(x)) << I2S_VERID_MAJOR_SHIFT)) & I2S_VERID_MAJOR_MASK)
/*! @} */

/*! @name PARAM - Parameter Register */
/*! @{ */
#define I2S_PARAM_DATALINE_MASK                  (0xFU)
#define I2S_PARAM_DATALINE_SHIFT                 (0U)
#define I2S_PARAM_DATALINE(x)                    (((uint32_t)(((uint32_t)(x)) << I2S_PARAM_DATALINE_SHIFT)) & I2S_PARAM_DATALINE_MASK)
#define I2S_PARAM_FIFO_MASK                      (0xF00U)
#define I2S_PARAM_FIFO_SHIFT                     (8U)
#define I2S_PARAM_FIFO(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_PARAM_FIFO_SHIFT)) & I2S_PARAM_FIFO_MASK)
#define I2S_PARAM_FRAME_MASK                     (0xF0000U)
#define I2S_PARAM_FRAME_SHIFT                    (16U)
#define I2S_PARAM_FRAME(x)                       (((uint32_t)(((uint32_t)(x)) << I2S_PARAM_FRAME_SHIFT)) & I2S_PARAM_FRAME_MASK)
/*! @} */

/*! @name TCSR - SAI Transmit Control Register */
/*! @{ */
#define I2S_TCSR_FRDE_MASK                       (0x1U)
#define I2S_TCSR_FRDE_SHIFT                      (0U)
/*! FRDE - FIFO Request DMA Enable
 *  0b0..Disables the DMA request.
 *  0b1..Enables the DMA request.
 */
#define I2S_TCSR_FRDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FRDE_SHIFT)) & I2S_TCSR_FRDE_MASK)
#define I2S_TCSR_FWDE_MASK                       (0x2U)
#define I2S_TCSR_FWDE_SHIFT                      (1U)
/*! FWDE - FIFO Warning DMA Enable
 *  0b0..Disables the DMA request.
 *  0b1..Enables the DMA request.
 */
#define I2S_TCSR_FWDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FWDE_SHIFT)) & I2S_TCSR_FWDE_MASK)
#define I2S_TCSR_FRIE_MASK                       (0x100U)
#define I2S_TCSR_FRIE_SHIFT                      (8U)
/*! FRIE - FIFO Request Interrupt Enable
 *  0b0..Disables the interrupt.
 *  0b1..Enables the interrupt.
 */
#define I2S_TCSR_FRIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FRIE_SHIFT)) & I2S_TCSR_FRIE_MASK)
#define I2S_TCSR_FWIE_MASK                       (0x200U)
#define I2S_TCSR_FWIE_SHIFT                      (9U)
/*! FWIE - FIFO Warning Interrupt Enable
 *  0b0..Disables the interrupt.
 *  0b1..Enables the interrupt.
 */
#define I2S_TCSR_FWIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FWIE_SHIFT)) & I2S_TCSR_FWIE_MASK)
#define I2S_TCSR_FEIE_MASK                       (0x400U)
#define I2S_TCSR_FEIE_SHIFT                      (10U)
/*! FEIE - FIFO Error Interrupt Enable
 *  0b0..Disables the interrupt.
 *  0b1..Enables the interrupt.
 */
#define I2S_TCSR_FEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FEIE_SHIFT)) & I2S_TCSR_FEIE_MASK)
#define I2S_TCSR_SEIE_MASK                       (0x800U)
#define I2S_TCSR_SEIE_SHIFT                      (11U)
/*! SEIE - Sync Error Interrupt Enable
 *  0b0..Disables interrupt.
 *  0b1..Enables interrupt.
 */
#define I2S_TCSR_SEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_SEIE_SHIFT)) & I2S_TCSR_SEIE_MASK)
#define I2S_TCSR_WSIE_MASK                       (0x1000U)
#define I2S_TCSR_WSIE_SHIFT                      (12U)
/*! WSIE - Word Start Interrupt Enable
 *  0b0..Disables interrupt.
 *  0b1..Enables interrupt.
 */
#define I2S_TCSR_WSIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_WSIE_SHIFT)) & I2S_TCSR_WSIE_MASK)
#define I2S_TCSR_FRF_MASK                        (0x10000U)
#define I2S_TCSR_FRF_SHIFT                       (16U)
/*! FRF - FIFO Request Flag
 *  0b0..Transmit FIFO watermark has not been reached.
 *  0b1..Transmit FIFO watermark has been reached.
 */
#define I2S_TCSR_FRF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FRF_SHIFT)) & I2S_TCSR_FRF_MASK)
#define I2S_TCSR_FWF_MASK                        (0x20000U)
#define I2S_TCSR_FWF_SHIFT                       (17U)
/*! FWF - FIFO Warning Flag
 *  0b0..No enabled transmit FIFO is empty.
 *  0b1..Enabled transmit FIFO is empty.
 */
#define I2S_TCSR_FWF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FWF_SHIFT)) & I2S_TCSR_FWF_MASK)
#define I2S_TCSR_FEF_MASK                        (0x40000U)
#define I2S_TCSR_FEF_SHIFT                       (18U)
/*! FEF - FIFO Error Flag
 *  0b0..Transmit underrun not detected.
 *  0b1..Transmit underrun detected.
 */
#define I2S_TCSR_FEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FEF_SHIFT)) & I2S_TCSR_FEF_MASK)
#define I2S_TCSR_SEF_MASK                        (0x80000U)
#define I2S_TCSR_SEF_SHIFT                       (19U)
/*! SEF - Sync Error Flag
 *  0b0..Sync error not detected.
 *  0b1..Frame sync error detected.
 */
#define I2S_TCSR_SEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_SEF_SHIFT)) & I2S_TCSR_SEF_MASK)
#define I2S_TCSR_WSF_MASK                        (0x100000U)
#define I2S_TCSR_WSF_SHIFT                       (20U)
/*! WSF - Word Start Flag
 *  0b0..Start of word not detected.
 *  0b1..Start of word detected.
 */
#define I2S_TCSR_WSF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_WSF_SHIFT)) & I2S_TCSR_WSF_MASK)
#define I2S_TCSR_SR_MASK                         (0x1000000U)
#define I2S_TCSR_SR_SHIFT                        (24U)
/*! SR - Software Reset
 *  0b0..No effect.
 *  0b1..Software reset.
 */
#define I2S_TCSR_SR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_SR_SHIFT)) & I2S_TCSR_SR_MASK)
#define I2S_TCSR_FR_MASK                         (0x2000000U)
#define I2S_TCSR_FR_SHIFT                        (25U)
/*! FR - FIFO Reset
 *  0b0..No effect.
 *  0b1..FIFO reset.
 */
#define I2S_TCSR_FR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FR_SHIFT)) & I2S_TCSR_FR_MASK)
#define I2S_TCSR_BCE_MASK                        (0x10000000U)
#define I2S_TCSR_BCE_SHIFT                       (28U)
/*! BCE - Bit Clock Enable
 *  0b0..Transmit bit clock is disabled.
 *  0b1..Transmit bit clock is enabled.
 */
#define I2S_TCSR_BCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_BCE_SHIFT)) & I2S_TCSR_BCE_MASK)
#define I2S_TCSR_DBGE_MASK                       (0x20000000U)
#define I2S_TCSR_DBGE_SHIFT                      (29U)
/*! DBGE - Debug Enable
 *  0b0..Transmitter is disabled in Debug mode, after completing the current frame.
 *  0b1..Transmitter is enabled in Debug mode.
 */
#define I2S_TCSR_DBGE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_DBGE_SHIFT)) & I2S_TCSR_DBGE_MASK)
#define I2S_TCSR_STOPE_MASK                      (0x40000000U)
#define I2S_TCSR_STOPE_SHIFT                     (30U)
/*! STOPE - Stop Enable
 *  0b0..Transmitter disabled in Stop mode.
 *  0b1..Transmitter enabled in Stop mode.
 */
#define I2S_TCSR_STOPE(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_STOPE_SHIFT)) & I2S_TCSR_STOPE_MASK)
#define I2S_TCSR_TE_MASK                         (0x80000000U)
#define I2S_TCSR_TE_SHIFT                        (31U)
/*! TE - Transmitter Enable
 *  0b0..Transmitter is disabled.
 *  0b1..Transmitter is enabled, or transmitter has been disabled and has not yet reached end of frame.
 */
#define I2S_TCSR_TE(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_TE_SHIFT)) & I2S_TCSR_TE_MASK)
/*! @} */

/*! @name TCR1 - SAI Transmit Configuration 1 Register */
/*! @{ */
#define I2S_TCR1_TFW_MASK                        (0x7FU)
#define I2S_TCR1_TFW_SHIFT                       (0U)
#define I2S_TCR1_TFW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR1_TFW_SHIFT)) & I2S_TCR1_TFW_MASK)
/*! @} */

/*! @name TCR2 - SAI Transmit Configuration 2 Register */
/*! @{ */
#define I2S_TCR2_DIV_MASK                        (0xFFU)
#define I2S_TCR2_DIV_SHIFT                       (0U)
#define I2S_TCR2_DIV(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_DIV_SHIFT)) & I2S_TCR2_DIV_MASK)
#define I2S_TCR2_BYP_MASK                        (0x800000U)
#define I2S_TCR2_BYP_SHIFT                       (23U)
/*! BYP - Bit Clock Bypass
 *  0b0..Internal bit clock is generated from bit clock divider.
 *  0b1..Internal bit clock is divide by one of the audio master clock.
 */
#define I2S_TCR2_BYP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BYP_SHIFT)) & I2S_TCR2_BYP_MASK)
#define I2S_TCR2_BCD_MASK                        (0x1000000U)
#define I2S_TCR2_BCD_SHIFT                       (24U)
/*! BCD - Bit Clock Direction
 *  0b0..Bit clock is generated externally in Slave mode.
 *  0b1..Bit clock is generated internally in Master mode.
 */
#define I2S_TCR2_BCD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCD_SHIFT)) & I2S_TCR2_BCD_MASK)
#define I2S_TCR2_BCP_MASK                        (0x2000000U)
#define I2S_TCR2_BCP_SHIFT                       (25U)
/*! BCP - Bit Clock Polarity
 *  0b0..Bit clock is active high with drive outputs on rising edge and sample inputs on falling edge.
 *  0b1..Bit clock is active low with drive outputs on falling edge and sample inputs on rising edge.
 */
#define I2S_TCR2_BCP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCP_SHIFT)) & I2S_TCR2_BCP_MASK)
#define I2S_TCR2_MSEL_MASK                       (0xC000000U)
#define I2S_TCR2_MSEL_SHIFT                      (26U)
/*! MSEL - MCLK Select
 *  0b00..Bus Clock selected.
 *  0b01..Master Clock (MCLK) 1 option selected.
 *  0b10..Master Clock (MCLK) 2 option selected.
 *  0b11..Master Clock (MCLK) 3 option selected.
 */
#define I2S_TCR2_MSEL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_MSEL_SHIFT)) & I2S_TCR2_MSEL_MASK)
#define I2S_TCR2_BCI_MASK                        (0x10000000U)
#define I2S_TCR2_BCI_SHIFT                       (28U)
/*! BCI - Bit Clock Input
 *  0b0..No effect.
 *  0b1..Internal logic is clocked as if bit clock was externally generated.
 */
#define I2S_TCR2_BCI(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCI_SHIFT)) & I2S_TCR2_BCI_MASK)
#define I2S_TCR2_BCS_MASK                        (0x20000000U)
#define I2S_TCR2_BCS_SHIFT                       (29U)
/*! BCS - Bit Clock Swap
 *  0b0..Use the normal bit clock source.
 *  0b1..Swap the bit clock source.
 */
#define I2S_TCR2_BCS(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCS_SHIFT)) & I2S_TCR2_BCS_MASK)
#define I2S_TCR2_SYNC_MASK                       (0xC0000000U)
#define I2S_TCR2_SYNC_SHIFT                      (30U)
/*! SYNC - Synchronous Mode
 *  0b00..Asynchronous mode.
 *  0b01..Synchronous with receiver.
 *  0b10..Reserved.
 *  0b11..Reserved.
 */
#define I2S_TCR2_SYNC(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_SYNC_SHIFT)) & I2S_TCR2_SYNC_MASK)
/*! @} */

/*! @name TCR3 - SAI Transmit Configuration 3 Register */
/*! @{ */
#define I2S_TCR3_WDFL_MASK                       (0x1FU)
#define I2S_TCR3_WDFL_SHIFT                      (0U)
#define I2S_TCR3_WDFL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR3_WDFL_SHIFT)) & I2S_TCR3_WDFL_MASK)
#define I2S_TCR3_TCE_MASK                        (0xFF0000U)
#define I2S_TCR3_TCE_SHIFT                       (16U)
#define I2S_TCR3_TCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR3_TCE_SHIFT)) & I2S_TCR3_TCE_MASK)
#define I2S_TCR3_CFR_MASK                        (0xFF000000U)
#define I2S_TCR3_CFR_SHIFT                       (24U)
#define I2S_TCR3_CFR(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR3_CFR_SHIFT)) & I2S_TCR3_CFR_MASK)
/*! @} */

/*! @name TCR4 - SAI Transmit Configuration 4 Register */
/*! @{ */
#define I2S_TCR4_FSD_MASK                        (0x1U)
#define I2S_TCR4_FSD_SHIFT                       (0U)
/*! FSD - Frame Sync Direction
 *  0b0..Frame sync is generated externally in Slave mode.
 *  0b1..Frame sync is generated internally in Master mode.
 */
#define I2S_TCR4_FSD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FSD_SHIFT)) & I2S_TCR4_FSD_MASK)
#define I2S_TCR4_FSP_MASK                        (0x2U)
#define I2S_TCR4_FSP_SHIFT                       (1U)
/*! FSP - Frame Sync Polarity
 *  0b0..Frame sync is active high.
 *  0b1..Frame sync is active low.
 */
#define I2S_TCR4_FSP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FSP_SHIFT)) & I2S_TCR4_FSP_MASK)
#define I2S_TCR4_ONDEM_MASK                      (0x4U)
#define I2S_TCR4_ONDEM_SHIFT                     (2U)
/*! ONDEM - On Demand Mode
 *  0b0..Internal frame sync is generated continuously.
 *  0b1..Internal frame sync is generated when the FIFO warning flag is clear.
 */
#define I2S_TCR4_ONDEM(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_ONDEM_SHIFT)) & I2S_TCR4_ONDEM_MASK)
#define I2S_TCR4_FSE_MASK                        (0x8U)
#define I2S_TCR4_FSE_SHIFT                       (3U)
/*! FSE - Frame Sync Early
 *  0b0..Frame sync asserts with the first bit of the frame.
 *  0b1..Frame sync asserts one bit before the first bit of the frame.
 */
#define I2S_TCR4_FSE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FSE_SHIFT)) & I2S_TCR4_FSE_MASK)
#define I2S_TCR4_MF_MASK                         (0x10U)
#define I2S_TCR4_MF_SHIFT                        (4U)
/*! MF - MSB First
 *  0b0..LSB is transmitted first.
 *  0b1..MSB is transmitted first.
 */
#define I2S_TCR4_MF(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_MF_SHIFT)) & I2S_TCR4_MF_MASK)
#define I2S_TCR4_CHMOD_MASK                      (0x20U)
#define I2S_TCR4_CHMOD_SHIFT                     (5U)
/*! CHMOD - Channel Mode
 *  0b0..TDM mode, transmit data pins are tri-stated when slots are masked or channels are disabled.
 *  0b1..Output mode, transmit data pins are never tri-stated and will output zero when slots are masked or channels are disabled.
 */
#define I2S_TCR4_CHMOD(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_CHMOD_SHIFT)) & I2S_TCR4_CHMOD_MASK)
#define I2S_TCR4_SYWD_MASK                       (0x1F00U)
#define I2S_TCR4_SYWD_SHIFT                      (8U)
#define I2S_TCR4_SYWD(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_SYWD_SHIFT)) & I2S_TCR4_SYWD_MASK)
#define I2S_TCR4_FRSZ_MASK                       (0x1F0000U)
#define I2S_TCR4_FRSZ_SHIFT                      (16U)
#define I2S_TCR4_FRSZ(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FRSZ_SHIFT)) & I2S_TCR4_FRSZ_MASK)
#define I2S_TCR4_FPACK_MASK                      (0x3000000U)
#define I2S_TCR4_FPACK_SHIFT                     (24U)
/*! FPACK - FIFO Packing Mode
 *  0b00..FIFO packing is disabled
 *  0b01..Reserved
 *  0b10..8-bit FIFO packing is enabled
 *  0b11..16-bit FIFO packing is enabled
 */
#define I2S_TCR4_FPACK(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FPACK_SHIFT)) & I2S_TCR4_FPACK_MASK)
#define I2S_TCR4_FCOMB_MASK                      (0xC000000U)
#define I2S_TCR4_FCOMB_SHIFT                     (26U)
/*! FCOMB - FIFO Combine Mode
 *  0b00..FIFO combine mode disabled.
 *  0b01..FIFO combine mode enabled on FIFO reads (from transmit shift registers).
 *  0b10..FIFO combine mode enabled on FIFO writes (by software).
 *  0b11..FIFO combine mode enabled on FIFO reads (from transmit shift registers) and writes (by software).
 */
#define I2S_TCR4_FCOMB(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FCOMB_SHIFT)) & I2S_TCR4_FCOMB_MASK)
#define I2S_TCR4_FCONT_MASK                      (0x10000000U)
#define I2S_TCR4_FCONT_SHIFT                     (28U)
/*! FCONT - FIFO Continue on Error
 *  0b0..On FIFO error, the SAI will continue from the start of the next frame after the FIFO error flag has been cleared.
 *  0b1..On FIFO error, the SAI will continue from the same word that caused the FIFO error to set after the FIFO warning flag has been cleared.
 */
#define I2S_TCR4_FCONT(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FCONT_SHIFT)) & I2S_TCR4_FCONT_MASK)
/*! @} */

/*! @name TCR5 - SAI Transmit Configuration 5 Register */
/*! @{ */
#define I2S_TCR5_FBT_MASK                        (0x1F00U)
#define I2S_TCR5_FBT_SHIFT                       (8U)
#define I2S_TCR5_FBT(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR5_FBT_SHIFT)) & I2S_TCR5_FBT_MASK)
#define I2S_TCR5_W0W_MASK                        (0x1F0000U)
#define I2S_TCR5_W0W_SHIFT                       (16U)
#define I2S_TCR5_W0W(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR5_W0W_SHIFT)) & I2S_TCR5_W0W_MASK)
#define I2S_TCR5_WNW_MASK                        (0x1F000000U)
#define I2S_TCR5_WNW_SHIFT                       (24U)
#define I2S_TCR5_WNW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR5_WNW_SHIFT)) & I2S_TCR5_WNW_MASK)
/*! @} */

/*! @name TDR - SAI Transmit Data Register */
/*! @{ */
#define I2S_TDR_TDR_MASK                         (0xFFFFFFFFU)
#define I2S_TDR_TDR_SHIFT                        (0U)
#define I2S_TDR_TDR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TDR_TDR_SHIFT)) & I2S_TDR_TDR_MASK)
/*! @} */

/* The count of I2S_TDR */
#define I2S_TDR_COUNT                            (8U)

/*! @name TFR - SAI Transmit FIFO Register */
/*! @{ */
#define I2S_TFR_RFP_MASK                         (0xFFU)
#define I2S_TFR_RFP_SHIFT                        (0U)
#define I2S_TFR_RFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TFR_RFP_SHIFT)) & I2S_TFR_RFP_MASK)
#define I2S_TFR_WFP_MASK                         (0xFF0000U)
#define I2S_TFR_WFP_SHIFT                        (16U)
#define I2S_TFR_WFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TFR_WFP_SHIFT)) & I2S_TFR_WFP_MASK)
#define I2S_TFR_WCP_MASK                         (0x80000000U)
#define I2S_TFR_WCP_SHIFT                        (31U)
/*! WCP - Write Channel Pointer
 *  0b0..No effect.
 *  0b1..FIFO combine is enabled for FIFO writes and this FIFO will be written on the next FIFO write.
 */
#define I2S_TFR_WCP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TFR_WCP_SHIFT)) & I2S_TFR_WCP_MASK)
/*! @} */

/* The count of I2S_TFR */
#define I2S_TFR_COUNT                            (8U)

/*! @name TMR - SAI Transmit Mask Register */
/*! @{ */
#define I2S_TMR_TWM_MASK                         (0xFFFFFFFFU)
#define I2S_TMR_TWM_SHIFT                        (0U)
/*! TWM - Transmit Word Mask
 *  0b00000000000000000000000000000000..Word N is enabled.
 *  0b00000000000000000000000000000001..Word N is masked. The transmit data pins are tri-stated or drive zero when masked.
 */
#define I2S_TMR_TWM(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TMR_TWM_SHIFT)) & I2S_TMR_TWM_MASK)
/*! @} */

/*! @name TTCR - SAI Transmit Timestamp Control Register */
/*! @{ */
#define I2S_TTCR_TSEN_MASK                       (0x1U)
#define I2S_TTCR_TSEN_SHIFT                      (0U)
/*! TSEN - Timestamp Enable
 *  0b0..Timestamp counter is disabled.
 *  0b1..Timestamp counter is enabled.
 */
#define I2S_TTCR_TSEN(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TTCR_TSEN_SHIFT)) & I2S_TTCR_TSEN_MASK)
#define I2S_TTCR_TSINC_MASK                      (0x2U)
#define I2S_TTCR_TSINC_SHIFT                     (1U)
/*! TSINC - Timestamp Increment
 *  0b0..Timestamp counter starts to increment when enabled and the bit counter has incremented.
 *  0b1..Timestamp counter starts to increment when enabled.
 */
#define I2S_TTCR_TSINC(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TTCR_TSINC_SHIFT)) & I2S_TTCR_TSINC_MASK)
#define I2S_TTCR_RTSC_MASK                       (0x100U)
#define I2S_TTCR_RTSC_SHIFT                      (8U)
/*! RTSC - Reset Timestamp Counter
 *  0b0..Timestamp counter is not reset.
 *  0b1..Timestamp counter is reset.
 */
#define I2S_TTCR_RTSC(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TTCR_RTSC_SHIFT)) & I2S_TTCR_RTSC_MASK)
#define I2S_TTCR_RBC_MASK                        (0x200U)
#define I2S_TTCR_RBC_SHIFT                       (9U)
/*! RBC - Reset Bit Counter
 *  0b0..Bit counter is not reset.
 *  0b1..Bit counter is reset.
 */
#define I2S_TTCR_RBC(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TTCR_RBC_SHIFT)) & I2S_TTCR_RBC_MASK)
/*! @} */

/*! @name TTSR - SAI Transmit Timestamp Register */
/*! @{ */
#define I2S_TTSR_TSC_MASK                        (0xFFFFFFFFU)
#define I2S_TTSR_TSC_SHIFT                       (0U)
#define I2S_TTSR_TSC(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TTSR_TSC_SHIFT)) & I2S_TTSR_TSC_MASK)
/*! @} */

/*! @name TBCR - SAI Transmit Bit Count Register */
/*! @{ */
#define I2S_TBCR_BCNT_MASK                       (0xFFFFFFFFU)
#define I2S_TBCR_BCNT_SHIFT                      (0U)
#define I2S_TBCR_BCNT(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TBCR_BCNT_SHIFT)) & I2S_TBCR_BCNT_MASK)
/*! @} */

/*! @name TBCTR - SAI Transmit Bit Count Timestamp Register */
/*! @{ */
#define I2S_TBCTR_BCTS_MASK                      (0xFFFFFFFFU)
#define I2S_TBCTR_BCTS_SHIFT                     (0U)
#define I2S_TBCTR_BCTS(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TBCTR_BCTS_SHIFT)) & I2S_TBCTR_BCTS_MASK)
/*! @} */

/*! @name RCSR - SAI Receive Control Register */
/*! @{ */
#define I2S_RCSR_FRDE_MASK                       (0x1U)
#define I2S_RCSR_FRDE_SHIFT                      (0U)
/*! FRDE - FIFO Request DMA Enable
 *  0b0..Disables the DMA request.
 *  0b1..Enables the DMA request.
 */
#define I2S_RCSR_FRDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FRDE_SHIFT)) & I2S_RCSR_FRDE_MASK)
#define I2S_RCSR_FWDE_MASK                       (0x2U)
#define I2S_RCSR_FWDE_SHIFT                      (1U)
/*! FWDE - FIFO Warning DMA Enable
 *  0b0..Disables the DMA request.
 *  0b1..Enables the DMA request.
 */
#define I2S_RCSR_FWDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FWDE_SHIFT)) & I2S_RCSR_FWDE_MASK)
#define I2S_RCSR_FRIE_MASK                       (0x100U)
#define I2S_RCSR_FRIE_SHIFT                      (8U)
/*! FRIE - FIFO Request Interrupt Enable
 *  0b0..Disables the interrupt.
 *  0b1..Enables the interrupt.
 */
#define I2S_RCSR_FRIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FRIE_SHIFT)) & I2S_RCSR_FRIE_MASK)
#define I2S_RCSR_FWIE_MASK                       (0x200U)
#define I2S_RCSR_FWIE_SHIFT                      (9U)
/*! FWIE - FIFO Warning Interrupt Enable
 *  0b0..Disables the interrupt.
 *  0b1..Enables the interrupt.
 */
#define I2S_RCSR_FWIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FWIE_SHIFT)) & I2S_RCSR_FWIE_MASK)
#define I2S_RCSR_FEIE_MASK                       (0x400U)
#define I2S_RCSR_FEIE_SHIFT                      (10U)
/*! FEIE - FIFO Error Interrupt Enable
 *  0b0..Disables the interrupt.
 *  0b1..Enables the interrupt.
 */
#define I2S_RCSR_FEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FEIE_SHIFT)) & I2S_RCSR_FEIE_MASK)
#define I2S_RCSR_SEIE_MASK                       (0x800U)
#define I2S_RCSR_SEIE_SHIFT                      (11U)
/*! SEIE - Sync Error Interrupt Enable
 *  0b0..Disables interrupt.
 *  0b1..Enables interrupt.
 */
#define I2S_RCSR_SEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_SEIE_SHIFT)) & I2S_RCSR_SEIE_MASK)
#define I2S_RCSR_WSIE_MASK                       (0x1000U)
#define I2S_RCSR_WSIE_SHIFT                      (12U)
/*! WSIE - Word Start Interrupt Enable
 *  0b0..Disables interrupt.
 *  0b1..Enables interrupt.
 */
#define I2S_RCSR_WSIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_WSIE_SHIFT)) & I2S_RCSR_WSIE_MASK)
#define I2S_RCSR_FRF_MASK                        (0x10000U)
#define I2S_RCSR_FRF_SHIFT                       (16U)
/*! FRF - FIFO Request Flag
 *  0b0..Receive FIFO watermark not reached.
 *  0b1..Receive FIFO watermark has been reached.
 */
#define I2S_RCSR_FRF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FRF_SHIFT)) & I2S_RCSR_FRF_MASK)
#define I2S_RCSR_FWF_MASK                        (0x20000U)
#define I2S_RCSR_FWF_SHIFT                       (17U)
/*! FWF - FIFO Warning Flag
 *  0b0..No enabled receive FIFO is full.
 *  0b1..Enabled receive FIFO is full.
 */
#define I2S_RCSR_FWF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FWF_SHIFT)) & I2S_RCSR_FWF_MASK)
#define I2S_RCSR_FEF_MASK                        (0x40000U)
#define I2S_RCSR_FEF_SHIFT                       (18U)
/*! FEF - FIFO Error Flag
 *  0b0..Receive overflow not detected.
 *  0b1..Receive overflow detected.
 */
#define I2S_RCSR_FEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FEF_SHIFT)) & I2S_RCSR_FEF_MASK)
#define I2S_RCSR_SEF_MASK                        (0x80000U)
#define I2S_RCSR_SEF_SHIFT                       (19U)
/*! SEF - Sync Error Flag
 *  0b0..Sync error not detected.
 *  0b1..Frame sync error detected.
 */
#define I2S_RCSR_SEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_SEF_SHIFT)) & I2S_RCSR_SEF_MASK)
#define I2S_RCSR_WSF_MASK                        (0x100000U)
#define I2S_RCSR_WSF_SHIFT                       (20U)
/*! WSF - Word Start Flag
 *  0b0..Start of word not detected.
 *  0b1..Start of word detected.
 */
#define I2S_RCSR_WSF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_WSF_SHIFT)) & I2S_RCSR_WSF_MASK)
#define I2S_RCSR_SR_MASK                         (0x1000000U)
#define I2S_RCSR_SR_SHIFT                        (24U)
/*! SR - Software Reset
 *  0b0..No effect.
 *  0b1..Software reset.
 */
#define I2S_RCSR_SR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_SR_SHIFT)) & I2S_RCSR_SR_MASK)
#define I2S_RCSR_FR_MASK                         (0x2000000U)
#define I2S_RCSR_FR_SHIFT                        (25U)
/*! FR - FIFO Reset
 *  0b0..No effect.
 *  0b1..FIFO reset.
 */
#define I2S_RCSR_FR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FR_SHIFT)) & I2S_RCSR_FR_MASK)
#define I2S_RCSR_BCE_MASK                        (0x10000000U)
#define I2S_RCSR_BCE_SHIFT                       (28U)
/*! BCE - Bit Clock Enable
 *  0b0..Receive bit clock is disabled.
 *  0b1..Receive bit clock is enabled.
 */
#define I2S_RCSR_BCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_BCE_SHIFT)) & I2S_RCSR_BCE_MASK)
#define I2S_RCSR_DBGE_MASK                       (0x20000000U)
#define I2S_RCSR_DBGE_SHIFT                      (29U)
/*! DBGE - Debug Enable
 *  0b0..Receiver is disabled in Debug mode, after completing the current frame.
 *  0b1..Receiver is enabled in Debug mode.
 */
#define I2S_RCSR_DBGE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_DBGE_SHIFT)) & I2S_RCSR_DBGE_MASK)
#define I2S_RCSR_STOPE_MASK                      (0x40000000U)
#define I2S_RCSR_STOPE_SHIFT                     (30U)
/*! STOPE - Stop Enable
 *  0b0..Receiver disabled in Stop mode.
 *  0b1..Receiver enabled in Stop mode.
 */
#define I2S_RCSR_STOPE(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_STOPE_SHIFT)) & I2S_RCSR_STOPE_MASK)
#define I2S_RCSR_RE_MASK                         (0x80000000U)
#define I2S_RCSR_RE_SHIFT                        (31U)
/*! RE - Receiver Enable
 *  0b0..Receiver is disabled.
 *  0b1..Receiver is enabled, or receiver has been disabled and has not yet reached end of frame.
 */
#define I2S_RCSR_RE(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_RE_SHIFT)) & I2S_RCSR_RE_MASK)
/*! @} */

/*! @name RCR1 - SAI Receive Configuration 1 Register */
/*! @{ */
#define I2S_RCR1_RFW_MASK                        (0x7FU)
#define I2S_RCR1_RFW_SHIFT                       (0U)
#define I2S_RCR1_RFW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR1_RFW_SHIFT)) & I2S_RCR1_RFW_MASK)
/*! @} */

/*! @name RCR2 - SAI Receive Configuration 2 Register */
/*! @{ */
#define I2S_RCR2_DIV_MASK                        (0xFFU)
#define I2S_RCR2_DIV_SHIFT                       (0U)
#define I2S_RCR2_DIV(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_DIV_SHIFT)) & I2S_RCR2_DIV_MASK)
#define I2S_RCR2_BYP_MASK                        (0x800000U)
#define I2S_RCR2_BYP_SHIFT                       (23U)
/*! BYP - Bit Clock Bypass
 *  0b0..Internal bit clock is generated from bit clock divider.
 *  0b1..Internal bit clock is divide by one of the audio master clock.
 */
#define I2S_RCR2_BYP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BYP_SHIFT)) & I2S_RCR2_BYP_MASK)
#define I2S_RCR2_BCD_MASK                        (0x1000000U)
#define I2S_RCR2_BCD_SHIFT                       (24U)
/*! BCD - Bit Clock Direction
 *  0b0..Bit clock is generated externally in Slave mode.
 *  0b1..Bit clock is generated internally in Master mode.
 */
#define I2S_RCR2_BCD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCD_SHIFT)) & I2S_RCR2_BCD_MASK)
#define I2S_RCR2_BCP_MASK                        (0x2000000U)
#define I2S_RCR2_BCP_SHIFT                       (25U)
/*! BCP - Bit Clock Polarity
 *  0b0..Bit Clock is active high with drive outputs on rising edge and sample inputs on falling edge.
 *  0b1..Bit Clock is active low with drive outputs on falling edge and sample inputs on rising edge.
 */
#define I2S_RCR2_BCP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCP_SHIFT)) & I2S_RCR2_BCP_MASK)
#define I2S_RCR2_MSEL_MASK                       (0xC000000U)
#define I2S_RCR2_MSEL_SHIFT                      (26U)
/*! MSEL - MCLK Select
 *  0b00..Bus Clock selected.
 *  0b01..Master Clock (MCLK) 1 option selected.
 *  0b10..Master Clock (MCLK) 2 option selected.
 *  0b11..Master Clock (MCLK) 3 option selected.
 */
#define I2S_RCR2_MSEL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_MSEL_SHIFT)) & I2S_RCR2_MSEL_MASK)
#define I2S_RCR2_BCI_MASK                        (0x10000000U)
#define I2S_RCR2_BCI_SHIFT                       (28U)
/*! BCI - Bit Clock Input
 *  0b0..No effect.
 *  0b1..Internal logic is clocked as if bit clock was externally generated.
 */
#define I2S_RCR2_BCI(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCI_SHIFT)) & I2S_RCR2_BCI_MASK)
#define I2S_RCR2_BCS_MASK                        (0x20000000U)
#define I2S_RCR2_BCS_SHIFT                       (29U)
/*! BCS - Bit Clock Swap
 *  0b0..Use the normal bit clock source.
 *  0b1..Swap the bit clock source.
 */
#define I2S_RCR2_BCS(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCS_SHIFT)) & I2S_RCR2_BCS_MASK)
#define I2S_RCR2_SYNC_MASK                       (0xC0000000U)
#define I2S_RCR2_SYNC_SHIFT                      (30U)
/*! SYNC - Synchronous Mode
 *  0b00..Asynchronous mode.
 *  0b01..Synchronous with transmitter.
 *  0b10..Reserved.
 *  0b11..Reserved.
 */
#define I2S_RCR2_SYNC(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_SYNC_SHIFT)) & I2S_RCR2_SYNC_MASK)
/*! @} */

/*! @name RCR3 - SAI Receive Configuration 3 Register */
/*! @{ */
#define I2S_RCR3_WDFL_MASK                       (0x1FU)
#define I2S_RCR3_WDFL_SHIFT                      (0U)
#define I2S_RCR3_WDFL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR3_WDFL_SHIFT)) & I2S_RCR3_WDFL_MASK)
#define I2S_RCR3_RCE_MASK                        (0xFF0000U)
#define I2S_RCR3_RCE_SHIFT                       (16U)
#define I2S_RCR3_RCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR3_RCE_SHIFT)) & I2S_RCR3_RCE_MASK)
#define I2S_RCR3_CFR_MASK                        (0xFF000000U)
#define I2S_RCR3_CFR_SHIFT                       (24U)
#define I2S_RCR3_CFR(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR3_CFR_SHIFT)) & I2S_RCR3_CFR_MASK)
/*! @} */

/*! @name RCR4 - SAI Receive Configuration 4 Register */
/*! @{ */
#define I2S_RCR4_FSD_MASK                        (0x1U)
#define I2S_RCR4_FSD_SHIFT                       (0U)
/*! FSD - Frame Sync Direction
 *  0b0..Frame Sync is generated externally in Slave mode.
 *  0b1..Frame Sync is generated internally in Master mode.
 */
#define I2S_RCR4_FSD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FSD_SHIFT)) & I2S_RCR4_FSD_MASK)
#define I2S_RCR4_FSP_MASK                        (0x2U)
#define I2S_RCR4_FSP_SHIFT                       (1U)
/*! FSP - Frame Sync Polarity
 *  0b0..Frame sync is active high.
 *  0b1..Frame sync is active low.
 */
#define I2S_RCR4_FSP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FSP_SHIFT)) & I2S_RCR4_FSP_MASK)
#define I2S_RCR4_ONDEM_MASK                      (0x4U)
#define I2S_RCR4_ONDEM_SHIFT                     (2U)
/*! ONDEM - On Demand Mode
 *  0b0..Internal frame sync is generated continuously.
 *  0b1..Internal frame sync is generated when the FIFO warning flag is clear.
 */
#define I2S_RCR4_ONDEM(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_ONDEM_SHIFT)) & I2S_RCR4_ONDEM_MASK)
#define I2S_RCR4_FSE_MASK                        (0x8U)
#define I2S_RCR4_FSE_SHIFT                       (3U)
/*! FSE - Frame Sync Early
 *  0b0..Frame sync asserts with the first bit of the frame.
 *  0b1..Frame sync asserts one bit before the first bit of the frame.
 */
#define I2S_RCR4_FSE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FSE_SHIFT)) & I2S_RCR4_FSE_MASK)
#define I2S_RCR4_MF_MASK                         (0x10U)
#define I2S_RCR4_MF_SHIFT                        (4U)
/*! MF - MSB First
 *  0b0..LSB is received first.
 *  0b1..MSB is received first.
 */
#define I2S_RCR4_MF(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_MF_SHIFT)) & I2S_RCR4_MF_MASK)
#define I2S_RCR4_SYWD_MASK                       (0x1F00U)
#define I2S_RCR4_SYWD_SHIFT                      (8U)
#define I2S_RCR4_SYWD(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_SYWD_SHIFT)) & I2S_RCR4_SYWD_MASK)
#define I2S_RCR4_FRSZ_MASK                       (0x1F0000U)
#define I2S_RCR4_FRSZ_SHIFT                      (16U)
#define I2S_RCR4_FRSZ(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FRSZ_SHIFT)) & I2S_RCR4_FRSZ_MASK)
#define I2S_RCR4_FPACK_MASK                      (0x3000000U)
#define I2S_RCR4_FPACK_SHIFT                     (24U)
/*! FPACK - FIFO Packing Mode
 *  0b00..FIFO packing is disabled
 *  0b01..Reserved.
 *  0b10..8-bit FIFO packing is enabled
 *  0b11..16-bit FIFO packing is enabled
 */
#define I2S_RCR4_FPACK(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FPACK_SHIFT)) & I2S_RCR4_FPACK_MASK)
#define I2S_RCR4_FCOMB_MASK                      (0xC000000U)
#define I2S_RCR4_FCOMB_SHIFT                     (26U)
/*! FCOMB - FIFO Combine Mode
 *  0b00..FIFO combine mode disabled.
 *  0b01..FIFO combine mode enabled on FIFO writes (from receive shift registers).
 *  0b10..FIFO combine mode enabled on FIFO reads (by software).
 *  0b11..FIFO combine mode enabled on FIFO writes (from receive shift registers) and reads (by software).
 */
#define I2S_RCR4_FCOMB(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FCOMB_SHIFT)) & I2S_RCR4_FCOMB_MASK)
#define I2S_RCR4_FCONT_MASK                      (0x10000000U)
#define I2S_RCR4_FCONT_SHIFT                     (28U)
/*! FCONT - FIFO Continue on Error
 *  0b0..On FIFO error, the SAI will continue from the start of the next frame after the FIFO error flag has been cleared.
 *  0b1..On FIFO error, the SAI will continue from the same word that caused the FIFO error to set after the FIFO warning flag has been cleared.
 */
#define I2S_RCR4_FCONT(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FCONT_SHIFT)) & I2S_RCR4_FCONT_MASK)
/*! @} */

/*! @name RCR5 - SAI Receive Configuration 5 Register */
/*! @{ */
#define I2S_RCR5_FBT_MASK                        (0x1F00U)
#define I2S_RCR5_FBT_SHIFT                       (8U)
#define I2S_RCR5_FBT(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR5_FBT_SHIFT)) & I2S_RCR5_FBT_MASK)
#define I2S_RCR5_W0W_MASK                        (0x1F0000U)
#define I2S_RCR5_W0W_SHIFT                       (16U)
#define I2S_RCR5_W0W(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR5_W0W_SHIFT)) & I2S_RCR5_W0W_MASK)
#define I2S_RCR5_WNW_MASK                        (0x1F000000U)
#define I2S_RCR5_WNW_SHIFT                       (24U)
#define I2S_RCR5_WNW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR5_WNW_SHIFT)) & I2S_RCR5_WNW_MASK)
/*! @} */

/*! @name RDR - SAI Receive Data Register */
/*! @{ */
#define I2S_RDR_RDR_MASK                         (0xFFFFFFFFU)
#define I2S_RDR_RDR_SHIFT                        (0U)
#define I2S_RDR_RDR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RDR_RDR_SHIFT)) & I2S_RDR_RDR_MASK)
/*! @} */

/* The count of I2S_RDR */
#define I2S_RDR_COUNT                            (8U)

/*! @name RFR - SAI Receive FIFO Register */
/*! @{ */
#define I2S_RFR_RFP_MASK                         (0xFFU)
#define I2S_RFR_RFP_SHIFT                        (0U)
#define I2S_RFR_RFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RFR_RFP_SHIFT)) & I2S_RFR_RFP_MASK)
#define I2S_RFR_RCP_MASK                         (0x8000U)
#define I2S_RFR_RCP_SHIFT                        (15U)
/*! RCP - Receive Channel Pointer
 *  0b0..No effect.
 *  0b1..FIFO combine is enabled for FIFO reads and this FIFO will be read on the next FIFO read.
 */
#define I2S_RFR_RCP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RFR_RCP_SHIFT)) & I2S_RFR_RCP_MASK)
#define I2S_RFR_WFP_MASK                         (0xFF0000U)
#define I2S_RFR_WFP_SHIFT                        (16U)
#define I2S_RFR_WFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RFR_WFP_SHIFT)) & I2S_RFR_WFP_MASK)
/*! @} */

/* The count of I2S_RFR */
#define I2S_RFR_COUNT                            (8U)

/*! @name RMR - SAI Receive Mask Register */
/*! @{ */
#define I2S_RMR_RWM_MASK                         (0xFFFFFFFFU)
#define I2S_RMR_RWM_SHIFT                        (0U)
/*! RWM - Receive Word Mask
 *  0b00000000000000000000000000000000..Word N is enabled.
 *  0b00000000000000000000000000000001..Word N is masked.
 */
#define I2S_RMR_RWM(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RMR_RWM_SHIFT)) & I2S_RMR_RWM_MASK)
/*! @} */

/*! @name RTCR - SAI Receive Timestamp Control Register */
/*! @{ */
#define I2S_RTCR_TSEN_MASK                       (0x1U)
#define I2S_RTCR_TSEN_SHIFT                      (0U)
/*! TSEN - Timestamp Enable
 *  0b0..Timestamp counter is disabled.
 *  0b1..Timestamp counter is enabled.
 */
#define I2S_RTCR_TSEN(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RTCR_TSEN_SHIFT)) & I2S_RTCR_TSEN_MASK)
#define I2S_RTCR_TSINC_MASK                      (0x2U)
#define I2S_RTCR_TSINC_SHIFT                     (1U)
/*! TSINC - Timestamp Increment
 *  0b0..Timestamp counter starts to increment when enabled and the bit counter has incremented.
 *  0b1..Timestamp counter starts to increment when enabled.
 */
#define I2S_RTCR_TSINC(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RTCR_TSINC_SHIFT)) & I2S_RTCR_TSINC_MASK)
#define I2S_RTCR_RTSC_MASK                       (0x100U)
#define I2S_RTCR_RTSC_SHIFT                      (8U)
/*! RTSC - Reset Timestamp Counter
 *  0b0..Timestamp counter is not reset.
 *  0b1..Timestamp counter is reset.
 */
#define I2S_RTCR_RTSC(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RTCR_RTSC_SHIFT)) & I2S_RTCR_RTSC_MASK)
#define I2S_RTCR_RBC_MASK                        (0x200U)
#define I2S_RTCR_RBC_SHIFT                       (9U)
/*! RBC - Reset Bit Counter
 *  0b0..Bit counter is not reset.
 *  0b1..Bit counter is reset.
 */
#define I2S_RTCR_RBC(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RTCR_RBC_SHIFT)) & I2S_RTCR_RBC_MASK)
/*! @} */

/*! @name RTSR - SAI Receive Timestamp Register */
/*! @{ */
#define I2S_RTSR_TSC_MASK                        (0xFFFFFFFFU)
#define I2S_RTSR_TSC_SHIFT                       (0U)
#define I2S_RTSR_TSC(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RTSR_TSC_SHIFT)) & I2S_RTSR_TSC_MASK)
/*! @} */

/*! @name RBCR - SAI Receive Bit Count Register */
/*! @{ */
#define I2S_RBCR_BCNT_MASK                       (0xFFFFFFFFU)
#define I2S_RBCR_BCNT_SHIFT                      (0U)
#define I2S_RBCR_BCNT(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RBCR_BCNT_SHIFT)) & I2S_RBCR_BCNT_MASK)
/*! @} */

/*! @name RBCTR - SAI Receive Bit Count Timestamp Register */
/*! @{ */
#define I2S_RBCTR_BCTS_MASK                      (0xFFFFFFFFU)
#define I2S_RBCTR_BCTS_SHIFT                     (0U)
#define I2S_RBCTR_BCTS(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RBCTR_BCTS_SHIFT)) & I2S_RBCTR_BCTS_MASK)
/*! @} */

/*! @name MCR - SAI MCLK Control Register */
/*! @{ */
#define I2S_MCR_DIV_MASK                         (0xFFU)
#define I2S_MCR_DIV_SHIFT                        (0U)
#define I2S_MCR_DIV(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_MCR_DIV_SHIFT)) & I2S_MCR_DIV_MASK)
#define I2S_MCR_DIVEN_MASK                       (0x800000U)
#define I2S_MCR_DIVEN_SHIFT                      (23U)
/*! DIVEN - MCLK Post Divide Enable
 *  0b0..Output on MCLK signal pin is the audio master clock.
 *  0b1..Output on MCLK signal pin is a post-divided version of audio master clock.
 */
#define I2S_MCR_DIVEN(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_MCR_DIVEN_SHIFT)) & I2S_MCR_DIVEN_MASK)
#define I2S_MCR_MOE_MASK                         (0x40000000U)
#define I2S_MCR_MOE_SHIFT                        (30U)
/*! MOE - MCLK Output Enable
 *  0b0..MCLK signal pin is an input.
 *  0b1..MCLK signal pin is an output.
 */
#define I2S_MCR_MOE(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_MCR_MOE_SHIFT)) & I2S_MCR_MOE_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group I2S_Register_Masks */


/* I2S - Peripheral instance base addresses */
/** Peripheral I2S1 base address */
#define I2S1_BASE                                (0x30010000u)
/** Peripheral I2S1 base pointer */
#define I2S1                                     ((I2S_Type *)I2S1_BASE)
/** Peripheral I2S2 base address */
#define I2S2_BASE                                (0x30020000u)
/** Peripheral I2S2 base pointer */
#define I2S2                                     ((I2S_Type *)I2S2_BASE)
/** Peripheral I2S3 base address */
#define I2S3_BASE                                (0x30030000u)
/** Peripheral I2S3 base pointer */
#define I2S3                                     ((I2S_Type *)I2S3_BASE)
/** Peripheral I2S5 base address */
#define I2S5_BASE                                (0x30050000u)
/** Peripheral I2S5 base pointer */
#define I2S5                                     ((I2S_Type *)I2S5_BASE)
/** Peripheral I2S6 base address */
#define I2S6_BASE                                (0x30060000u)
/** Peripheral I2S6 base pointer */
#define I2S6                                     ((I2S_Type *)I2S6_BASE)
/** Array initializer of I2S peripheral base addresses */
#define I2S_BASE_ADDRS                           { 0u, I2S1_BASE, I2S2_BASE, I2S3_BASE, 0u, I2S5_BASE, I2S6_BASE }
/** Array initializer of I2S peripheral base pointers */
#define I2S_BASE_PTRS                            { (I2S_Type *)0u, I2S1, I2S2, I2S3, (I2S_Type *)0u, I2S5, I2S6 }
/** Interrupt vectors for the I2S peripheral type */
#define I2S_RX_IRQS                              { NotAvail_IRQn, I2S1_IRQn, I2S2_IRQn, I2S3_IRQn, NotAvail_IRQn, I2S56_IRQn, I2S56_IRQn }
#define I2S_TX_IRQS                              { NotAvail_IRQn, I2S1_IRQn, I2S2_IRQn, I2S3_IRQn, NotAvail_IRQn, I2S56_IRQn, I2S56_IRQn }

/*!
 * @}
 */ /* end of group I2S_Peripheral_Access_Layer */



#endif  /* _MIMX8MM6_CM4_H_ */

