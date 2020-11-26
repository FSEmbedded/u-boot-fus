/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_SAI_H_
#define _FSL_SAI_H_
#include <common.h>
#include "fus_common.h"
#include "fsl_device_registers.h"

/*!
 * @addtogroup sai_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_SAI_DRIVER_VERSION (MAKE_VERSION(2, 2, 2)) /*!< Version 2.2.2 */
/*@}*/

/*! @brief _sai_status_t, SAI return status.*/
enum
{
    kStatus_SAI_TxBusy    = MAKE_STATUS(kStatusGroup_SAI, 0), /*!< SAI Tx is busy. */
    kStatus_SAI_RxBusy    = MAKE_STATUS(kStatusGroup_SAI, 1), /*!< SAI Rx is busy. */
    kStatus_SAI_TxError   = MAKE_STATUS(kStatusGroup_SAI, 2), /*!< SAI Tx FIFO error. */
    kStatus_SAI_RxError   = MAKE_STATUS(kStatusGroup_SAI, 3), /*!< SAI Rx FIFO error. */
    kStatus_SAI_QueueFull = MAKE_STATUS(kStatusGroup_SAI, 4), /*!< SAI transfer queue is full. */
    kStatus_SAI_TxIdle    = MAKE_STATUS(kStatusGroup_SAI, 5), /*!< SAI Tx is idle */
    kStatus_SAI_RxIdle    = MAKE_STATUS(kStatusGroup_SAI, 6)  /*!< SAI Rx is idle */
};

/*! @brief _sai_channel_mask,.sai channel mask value, actual channel numbers is depend soc specific */
enum
{
    kSAI_Channel0Mask = 1 << 0U, /*!< channel 0 mask value */
    kSAI_Channel1Mask = 1 << 1U, /*!< channel 1 mask value */
    kSAI_Channel2Mask = 1 << 2U, /*!< channel 2 mask value */
    kSAI_Channel3Mask = 1 << 3U, /*!< channel 3 mask value */
    kSAI_Channel4Mask = 1 << 4U, /*!< channel 4 mask value */
    kSAI_Channel5Mask = 1 << 5U, /*!< channel 5 mask value */
    kSAI_Channel6Mask = 1 << 6U, /*!< channel 6 mask value */
    kSAI_Channel7Mask = 1 << 7U, /*!< channel 7 mask value */
};

/*! @brief Define the SAI bus type */
typedef enum _sai_protocol
{
    kSAI_BusLeftJustified = 0x0U, /*!< Uses left justified format.*/
    kSAI_BusRightJustified,       /*!< Uses right justified format. */
    kSAI_BusI2S,                  /*!< Uses I2S format. */
    kSAI_BusPCMA,                 /*!< Uses I2S PCM A format.*/
    kSAI_BusPCMB                  /*!< Uses I2S PCM B format. */
} sai_protocol_t;

/*! @brief Master or slave mode */
typedef enum _sai_master_slave
{
    kSAI_Master                      = 0x0U, /*!< Master mode include bclk and frame sync */
    kSAI_Slave                       = 0x1U, /*!< Slave mode  include bclk and frame sync */
    kSAI_Bclk_Master_FrameSync_Slave = 0x2U, /*!< bclk in master mode, frame sync in slave mode */
    kSAI_Bclk_Slave_FrameSync_Master = 0x3U, /*!< bclk in slave mode, frame sync in master mode */
} sai_master_slave_t;

/*! @brief Mono or stereo audio format */
typedef enum _sai_mono_stereo
{
    kSAI_Stereo = 0x0U, /*!< Stereo sound. */
    kSAI_MonoRight,     /*!< Only Right channel have sound. */
    kSAI_MonoLeft       /*!< Only left channel have sound. */
} sai_mono_stereo_t;

/*! @brief SAI data order, MSB or LSB */
typedef enum _sai_data_order
{
    kSAI_DataLSB = 0x0U, /*!< LSB bit transferred first */
    kSAI_DataMSB         /*!< MSB bit transferred first */
} sai_data_order_t;

/*! @brief SAI clock polarity, active high or low */
typedef enum _sai_clock_polarity
{
    kSAI_PolarityActiveHigh  = 0x0U, /*!< Drive outputs on rising edge */
    kSAI_PolarityActiveLow   = 0x1U, /*!< Drive outputs on falling edge */
    kSAI_SampleOnFallingEdge = 0x0U, /*!< Sample inputs on falling edge */
    kSAI_SampleOnRisingEdge  = 0x1U, /*!< Sample inputs on rising edge */
} sai_clock_polarity_t;

/*! @brief Synchronous or asynchronous mode */
typedef enum _sai_sync_mode
{
    kSAI_ModeAsync = 0x0U, /*!< Asynchronous mode */
    kSAI_ModeSync,         /*!< Synchronous mode (with receiver or transmit) */
#if defined(FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI) && (FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI)
    kSAI_ModeSyncWithOtherTx, /*!< Synchronous with another SAI transmit */
    kSAI_ModeSyncWithOtherRx  /*!< Synchronous with another SAI receiver */
#endif                        /* FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI */
} sai_sync_mode_t;

#if !(defined(FSL_FEATURE_SAI_HAS_NO_MCR_MICS) && (FSL_FEATURE_SAI_HAS_NO_MCR_MICS))
/*! @brief Mater clock source */
typedef enum _sai_mclk_source
{
    kSAI_MclkSourceSysclk = 0x0U, /*!< Master clock from the system clock */
    kSAI_MclkSourceSelect1,       /*!< Master clock from source 1 */
    kSAI_MclkSourceSelect2,       /*!< Master clock from source 2 */
    kSAI_MclkSourceSelect3        /*!< Master clock from source 3 */
} sai_mclk_source_t;
#endif

/*! @brief Bit clock source */
typedef enum _sai_bclk_source
{
    kSAI_BclkSourceBusclk = 0x0U, /*!< Bit clock using bus clock */
    /* General device bit source definition */
    kSAI_BclkSourceMclkOption1 = 0x1U, /*!< Bit clock MCLK option 1 */
    kSAI_BclkSourceMclkOption2 = 0x2U, /*!< Bit clock MCLK option2  */
    kSAI_BclkSourceMclkOption3 = 0x3U, /*!< Bit clock MCLK option3 */
    /* Kinetis device bit clock source definition */
    kSAI_BclkSourceMclkDiv   = 0x1U, /*!< Bit clock using master clock divider */
    kSAI_BclkSourceOtherSai0 = 0x2U, /*!< Bit clock from other SAI device  */
    kSAI_BclkSourceOtherSai1 = 0x3U  /*!< Bit clock from other SAI device */
} sai_bclk_source_t;

/*! @brief _sai_interrupt_enable_t, The SAI interrupt enable flag */
enum
{
    kSAI_WordStartInterruptEnable =
        I2S_TCSR_WSIE_MASK, /*!< Word start flag, means the first word in a frame detected */
    kSAI_SyncErrorInterruptEnable   = I2S_TCSR_SEIE_MASK, /*!< Sync error flag, means the sync error is detected */
    kSAI_FIFOWarningInterruptEnable = I2S_TCSR_FWIE_MASK, /*!< FIFO warning flag, means the FIFO is empty */
    kSAI_FIFOErrorInterruptEnable   = I2S_TCSR_FEIE_MASK, /*!< FIFO error flag */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    kSAI_FIFORequestInterruptEnable = I2S_TCSR_FRIE_MASK, /*!< FIFO request, means reached watermark */
#endif                                                    /* FSL_FEATURE_SAI_FIFO_COUNT */
};

/*! @brief _sai_dma_enable_t, The DMA request sources */
enum
{
    kSAI_FIFOWarningDMAEnable = I2S_TCSR_FWDE_MASK, /*!< FIFO warning caused by the DMA request */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    kSAI_FIFORequestDMAEnable = I2S_TCSR_FRDE_MASK, /*!< FIFO request caused by the DMA request */
#endif                                              /* FSL_FEATURE_SAI_FIFO_COUNT */
};

/*! @brief _sai_flags, The SAI status flag */
enum
{
    kSAI_WordStartFlag = I2S_TCSR_WSF_MASK, /*!< Word start flag, means the first word in a frame detected */
    kSAI_SyncErrorFlag = I2S_TCSR_SEF_MASK, /*!< Sync error flag, means the sync error is detected */
    kSAI_FIFOErrorFlag = I2S_TCSR_FEF_MASK, /*!< FIFO error flag */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    kSAI_FIFORequestFlag = I2S_TCSR_FRF_MASK, /*!< FIFO request flag. */
#endif                                        /* FSL_FEATURE_SAI_FIFO_COUNT */
    kSAI_FIFOWarningFlag = I2S_TCSR_FWF_MASK, /*!< FIFO warning flag */
};

/*! @brief The reset type */
typedef enum _sai_reset_type
{
    kSAI_ResetTypeSoftware = I2S_TCSR_SR_MASK, /*!< Software reset, reset the logic state */
    kSAI_ResetTypeFIFO     = I2S_TCSR_FR_MASK, /*!< FIFO reset, reset the FIFO read and write pointer */
    kSAI_ResetAll          = I2S_TCSR_SR_MASK | I2S_TCSR_FR_MASK /*!< All reset. */
} sai_reset_type_t;

#if defined(FSL_FEATURE_SAI_HAS_FIFO_PACKING) && FSL_FEATURE_SAI_HAS_FIFO_PACKING
/*!
 * @brief The SAI packing mode
 * The mode includes 8 bit and 16 bit packing.
 */
typedef enum _sai_fifo_packing
{
    kSAI_FifoPackingDisabled = 0x0U, /*!< Packing disabled */
    kSAI_FifoPacking8bit     = 0x2U, /*!< 8 bit packing enabled */
    kSAI_FifoPacking16bit    = 0x3U  /*!< 16bit packing enabled */
} sai_fifo_packing_t;
#endif /* FSL_FEATURE_SAI_HAS_FIFO_PACKING */

/*! @brief SAI user configuration structure */
typedef struct _sai_config
{
    sai_protocol_t protocol;  /*!< Audio bus protocol in SAI */
    sai_sync_mode_t syncMode; /*!< SAI sync mode, control Tx/Rx clock sync */
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    bool mclkOutputEnable; /*!< Master clock output enable, true means master clock divider enabled */
#if !(defined(FSL_FEATURE_SAI_HAS_NO_MCR_MICS) && (FSL_FEATURE_SAI_HAS_NO_MCR_MICS))
    sai_mclk_source_t mclkSource; /*!< Master Clock source */
#endif                            /* FSL_FEATURE_SAI_HAS_MCR */
#endif
    sai_bclk_source_t bclkSource;   /*!< Bit Clock source */
    sai_master_slave_t masterSlave; /*!< Master or slave */
} sai_config_t;

#ifndef SAI_XFER_QUEUE_SIZE
/*!@brief SAI transfer queue size, user can refine it according to use case. */
#define SAI_XFER_QUEUE_SIZE (4U)
#endif

/*! @brief Audio sample rate */
typedef enum _sai_sample_rate
{
    kSAI_SampleRate8KHz    = 8000U,   /*!< Sample rate 8000 Hz */
    kSAI_SampleRate11025Hz = 11025U,  /*!< Sample rate 11025 Hz */
    kSAI_SampleRate12KHz   = 12000U,  /*!< Sample rate 12000 Hz */
    kSAI_SampleRate16KHz   = 16000U,  /*!< Sample rate 16000 Hz */
    kSAI_SampleRate22050Hz = 22050U,  /*!< Sample rate 22050 Hz */
    kSAI_SampleRate24KHz   = 24000U,  /*!< Sample rate 24000 Hz */
    kSAI_SampleRate32KHz   = 32000U,  /*!< Sample rate 32000 Hz */
    kSAI_SampleRate44100Hz = 44100U,  /*!< Sample rate 44100 Hz */
    kSAI_SampleRate48KHz   = 48000U,  /*!< Sample rate 48000 Hz */
    kSAI_SampleRate96KHz   = 96000U,  /*!< Sample rate 96000 Hz */
    kSAI_SampleRate192KHz  = 192000U, /*!< Sample rate 192000 Hz */
    kSAI_SampleRate384KHz  = 384000U, /*!< Sample rate 384000 Hz */
} sai_sample_rate_t;

/*! @brief Audio word width */
typedef enum _sai_word_width
{
    kSAI_WordWidth8bits  = 8U,  /*!< Audio data width 8 bits */
    kSAI_WordWidth16bits = 16U, /*!< Audio data width 16 bits */
    kSAI_WordWidth24bits = 24U, /*!< Audio data width 24 bits */
    kSAI_WordWidth32bits = 32U  /*!< Audio data width 32 bits */
} sai_word_width_t;

#if defined(FSL_FEATURE_SAI_HAS_CHANNEL_MODE) && FSL_FEATURE_SAI_HAS_CHANNEL_MODE
/*! @brief sai data pin state definition */
typedef enum _sai_data_pin_state
{
    kSAI_DataPinStateTriState =
        0U, /*!< transmit data pins are tri-stated when slots are masked or channels are disabled */
    kSAI_DataPinStateOutputZero = 1U, /*!< transmit data pins are never tri-stated and will output zero when slots
                                             are masked or channel disabled */
} sai_data_pin_state_t;
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE
/*! @brief sai fifo combine mode definition */
typedef enum _sai_fifo_combine
{
    kSAI_FifoCombineDisabled = 0U,          /*!< sai fifo combine mode disabled */
    kSAI_FifoCombineModeEnabledOnRead,      /*!< sai fifo combine mode enabled on FIFO reads */
    kSAI_FifoCombineModeEnabledOnWrite,     /*!< sai fifo combine mode enabled on FIFO write */
    kSAI_FifoCombineModeEnabledOnReadWrite, /*!< sai fifo combined mode enabled on FIFO read/writes */
} sai_fifo_combine_t;
#endif

/*! @brief sai transceiver type */
typedef enum _sai_transceiver_type
{
    kSAI_Transmitter = 0U, /*!< sai transmitter */
    kSAI_Receiver    = 1U, /*!< sai receiver */
} sai_transceiver_type_t;

/*! @brief sai frame sync len */
typedef enum _sai_frame_sync_len
{
    kSAI_FrameSyncLenOneBitClk    = 0U, /*!< 1 bit clock frame sync len for DSP mode */
    kSAI_FrameSyncLenPerWordWidth = 1U, /*!< Frame sync length decided by word width */
} sai_frame_sync_len_t;

/*! @brief sai transfer format */
typedef struct _sai_transfer_format
{
    uint32_t sampleRate_Hz;   /*!< Sample rate of audio data */
    uint32_t bitWidth;        /*!< Data length of audio data, usually 8/16/24/32 bits */
    sai_mono_stereo_t stereo; /*!< Mono or stereo */
#if defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)
    uint32_t masterClockHz; /*!< Master clock frequency in Hz */
#endif                      /* FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    uint8_t watermark; /*!< Watermark value */
#endif                 /* FSL_FEATURE_SAI_FIFO_COUNT */

    /* for the multi channel usage, user can provide channelMask Oonly, then sai driver will handle
     * other parameter carefully, such as
     * channelMask = kSAI_Channel0Mask | kSAI_Channel1Mask | kSAI_Channel4Mask
     * then in SAI_RxSetFormat/SAI_TxSetFormat function, channel/endChannel/channelNums will be calculated.
     * for the single channel usage, user can provide channel or channel mask only, such as,
     * channel = 0 or channelMask = kSAI_Channel0Mask.
     */
    uint8_t channel;     /*!< Transfer start channel */
    uint8_t channelMask; /*!< enabled channel mask value, reference _sai_channel_mask */
    uint8_t endChannel;  /*!< end channel number */
    uint8_t channelNums; /*!< Total enabled channel numbers */

    sai_protocol_t protocol; /*!< Which audio protocol used */
    bool isFrameSyncCompact; /*!< True means Frame sync length is configurable according to bitWidth, false means frame
                                sync length is 64 times of bit clock. */
} sai_transfer_format_t;

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) || \
    (defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
/*! @brief master clock configurations */
typedef struct _sai_master_clock
{
#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    bool mclkOutputEnable; /*!< master clock output enable */
#if !(defined(FSL_FEATURE_SAI_HAS_NO_MCR_MICS) && (FSL_FEATURE_SAI_HAS_NO_MCR_MICS))
    sai_mclk_source_t mclkSource; /*!< Master Clock source */
#endif
#endif

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) || \
    (defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
    uint32_t mclkHz;          /*!< target mclk frequency */
    uint32_t mclkSourceClkHz; /*!< mclk source frequency*/
#endif
} sai_master_clock_t;
#endif

/*! @brief sai fifo configurations */
typedef struct _sai_fifo
{
#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR
    bool fifoContinueOneError; /*!< fifo continues when error occur */
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE
    sai_fifo_combine_t fifoCombine; /*!< fifo combine mode */
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_PACKING) && FSL_FEATURE_SAI_HAS_FIFO_PACKING
    sai_fifo_packing_t fifoPacking; /*!< fifo packing mode */
#endif
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    uint8_t fifoWatermark; /*!< fifo watermark */
#endif
} sai_fifo_t;

/*! @brief sai bit clock configurations */
typedef struct _sai_bit_clock
{
    bool bclkSrcSwap;    /*!< bit clock source swap */
    bool bclkInputDelay; /*!< bit clock actually used by the transmitter is delayed by the pad output delay,
                           this has effect of decreasing the data input setup time, but increasing the data output valid
                           time .*/
    sai_clock_polarity_t bclkPolarity; /*!< bit clock polarity */
    sai_bclk_source_t bclkSource;      /*!< bit Clock source */
} sai_bit_clock_t;

/*! @brief sai frame sync configurations */
typedef struct _sai_frame_sync
{
    uint8_t frameSyncWidth; /*!< frame sync width in number of bit clocks */
    bool frameSyncEarly;    /*!< TRUE is frame sync assert one bit before the first bit of frame
                                FALSE is frame sync assert with the first bit of the frame */

#if defined(FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND) && FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND
    bool frameSyncGenerateOnDemand; /*!< internal frame sync is generated when FIFO waring flag is clear */
#endif

    sai_clock_polarity_t frameSyncPolarity; /*!< frame sync polarity */

} sai_frame_sync_t;

/*! @brief sai serial data configurations */
typedef struct _sai_serial_data
{
#if defined(FSL_FEATURE_SAI_HAS_CHANNEL_MODE) && FSL_FEATURE_SAI_HAS_CHANNEL_MODE
    sai_data_pin_state_t dataMode; /*!< sai data pin state when slots masked or channel disabled */
#endif

    sai_data_order_t dataOrder; /*!< configure whether the LSB or MSB is transmitted first */
    uint8_t dataWord0Length;    /*!< configure the number of bits in the first word in each frame */
    uint8_t dataWordNLength; /*!< configure the number of bits in the each word in each frame, except the first word */
    uint8_t dataWordLength;  /*!< used to record the data length for dma transfer */
    uint8_t
        dataFirstBitShifted; /*!< Configure the bit index for the first bit transmitted for each word in the frame */
    uint8_t dataWordNum;     /*!< configure the number of words in each frame */
    uint32_t dataMaskedWord; /*!< configure whether the transmit word is masked */
} sai_serial_data_t;

/*! @brief sai transceiver configurations */
typedef struct _sai_transceiver
{
    sai_serial_data_t serialData; /*!< serial data configurations */
    sai_frame_sync_t frameSync;   /*!< ws configurations */
    sai_bit_clock_t bitClock;     /*!< bit clock configurations */
    sai_fifo_t fifo;              /*!< fifo configurations */

    sai_master_slave_t masterSlave; /*!< transceiver is master or slave */

    sai_sync_mode_t syncMode; /*!< transceiver sync mode */

    uint8_t startChannel; /*!< Transfer start channel */
    uint8_t channelMask;  /*!< enabled channel mask value, reference _sai_channel_mask */
    uint8_t endChannel;   /*!< end channel number */
    uint8_t channelNums;  /*!< Total enabled channel numbers */

} sai_transceiver_t;

/*! @brief SAI transfer structure */
typedef struct _sai_transfer
{
    uint8_t *data;   /*!< Data start address to transfer. */
    size_t dataSize; /*!< Transfer size. */
} sai_transfer_t;

typedef struct _sai_handle sai_handle_t;

/*! @brief SAI transfer callback prototype */
typedef void (*sai_transfer_callback_t)(I2S_Type *base, sai_handle_t *handle, status_t status, void *userData);

/*! @brief SAI handle structure */
struct _sai_handle
{
    I2S_Type *base; /*!< base address */

    uint32_t state;                   /*!< Transfer status */
    sai_transfer_callback_t callback; /*!< Callback function called at transfer event*/
    void *userData;                   /*!< Callback parameter passed to callback function*/
    uint8_t bitWidth;                 /*!< Bit width for transfer, 8/16/24/32 bits */

    /* for the multi channel usage, user can provide channelMask Oonly, then sai driver will handle
     * other parameter carefully, such as
     * channelMask = kSAI_Channel0Mask | kSAI_Channel1Mask | kSAI_Channel4Mask
     * then in SAI_RxSetFormat/SAI_TxSetFormat function, channel/endChannel/channelNums will be calculated.
     * for the single channel usage, user can provide channel or channel mask only, such as,
     * channel = 0 or channelMask = kSAI_Channel0Mask.
     */
    uint8_t channel;     /*!< Transfer start channel */
    uint8_t channelMask; /*!< enabled channel mask value, refernece _sai_channel_mask */
    uint8_t endChannel;  /*!< end channel number */
    uint8_t channelNums; /*!< Total enabled channel numbers */

    sai_transfer_t saiQueue[SAI_XFER_QUEUE_SIZE]; /*!< Transfer queue storing queued transfer */
    size_t transferSize[SAI_XFER_QUEUE_SIZE];     /*!< Data bytes need to transfer */
    volatile uint8_t queueUser;                   /*!< Index for user to queue transfer */
    volatile uint8_t queueDriver;                 /*!< Index for driver to get the transfer data and size */
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    uint8_t watermark; /*!< Watermark value */
#endif
};

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initializes the SAI peripheral.
 *
 * This API gates the SAI clock. The SAI module can't operate unless SAI_Init is called to enable the clock.
 *
 * @param base SAI base pointer.
 */
void SAI_Init(I2S_Type *base);

/*!
 * @brief Get classic I2S mode configurations.
 *
 * @param config transceiver configurations.
 * @param bitWidth audio data bitWidth.
 * @param mode audio data channel.
 * @param saiChannelMask mask value of the channel to be enable.
 */
void SAI_GetClassicI2SConfig(sai_transceiver_t *config,
                             sai_word_width_t bitWidth,
                             sai_mono_stereo_t mode,
                             uint32_t saiChannelMask);

/*!
 * @brief SAI transmitter configurations.
 *
 * @param base SAI base pointer.
 * @param config transmitter configurations.
 */
void SAI_TxSetConfig(I2S_Type *base, sai_transceiver_t *config);

/*!
 * @brief SAI receiver configurations.
 *
 * @param base SAI base pointer.
 * @param config receiver configurations.
 */
void SAI_RxSetConfig(I2S_Type *base, sai_transceiver_t *config);

/*!
 * @brief Resets the SAI Tx.
 *
 * This function enables the software reset and FIFO reset of SAI Tx. After reset, clear the reset bit.
 *
 * @param base SAI base pointer
 */
void SAI_TxReset(I2S_Type *base);

/*!
 * @brief Resets the SAI Rx.
 *
 * This function enables the software reset and FIFO reset of SAI Rx. After reset, clear the reset bit.
 *
 * @param base SAI base pointer
 */
void SAI_RxReset(I2S_Type *base);

/*!
 * @brief Transmitter Bit clock configurations.
 *
 * @param base SAI base pointer.
 * @param masterSlave master or slave.
 * @param config bit clock other configurations, can be NULL in slave mode.
 */
void SAI_TxSetBitclockConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_bit_clock_t *config);

/*!
 * @brief Receiver Bit clock configurations.
 *
 * @param base SAI base pointer.
 * @param masterSlave master or slave.
 * @param config bit clock other configurations, can be NULL in slave mode.
 */
void SAI_RxSetBitclockConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_bit_clock_t *config);

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) || \
    (defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
/*!
 * @brief Master clock configurations.
 *
 * @param base SAI base pointer.
 * @param config master clock configurations.
 */
void SAI_SetMasterClockConfig(I2S_Type *base, sai_master_clock_t *config);
#endif

/*!
 * @brief SAI transmitter fifo configurations.
 *
 * @param base SAI base pointer.
 * @param config fifo configurations.
 */
void SAI_TxSetFifoConfig(I2S_Type *base, sai_fifo_t *config);

/*!
 * @brief SAI receiver fifo configurations.
 *
 * @param base SAI base pointer.
 * @param config fifo configurations.
 */
void SAI_RxSetFifoConfig(I2S_Type *base, sai_fifo_t *config);

/*!
 * @brief SAI transmitter Frame sync configurations.
 *
 * @param base SAI base pointer.
 * @param masterSlave master or slave.
 * @param config frame sync configurations, can be NULL in slave mode.
 */
void SAI_TxSetFrameSyncConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_frame_sync_t *config);

/*!
 * @brief SAI receiver Frame sync configurations.
 *
 * @param base SAI base pointer.
 * @param masterSlave master or slave.
 * @param config frame sync configurations, can be NULL in slave mode.
 */
void SAI_RxSetFrameSyncConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_frame_sync_t *config);

/*!
 * @brief SAI transmitter Serial data configurations.
 *
 * @param base SAI base pointer.
 * @param config serial data configurations.
 */
void SAI_TxSetSerialDataConfig(I2S_Type *base, sai_serial_data_t *config);

/*!
 * @brief SAI receiver Serial data configurations.
 *
 * @param base SAI base pointer.
 * @param config serial data configurations.
 */
void SAI_RxSetSerialDataConfig(I2S_Type *base, sai_serial_data_t *config);

/*!
 * @brief Transmitter bit clock rate configurations.
 *
 * @param base SAI base pointer.
 * @param sourceClockHz, bit clock source frequency.
 * @param sampleRate audio data sample rate.
 * @param bitWidth, audio data bitWidth.
 * @param channelNumbers, audio channel numbers.
 */
void SAI_TxSetBitClockRate(
    I2S_Type *base, uint32_t sourceClockHz, uint32_t sampleRate, uint32_t bitWidth, uint32_t channelNumbers);

/*!
 * @brief Receiver bit clock rate configurations.
 *
 * @param base SAI base pointer.
 * @param sourceClockHz, bit clock source frequency.
 * @param sampleRate audio data sample rate.
 * @param bitWidth, audio data bitWidth.
 * @param channelNumbers, audio channel numbers.
 */
void SAI_RxSetBitClockRate(
    I2S_Type *base, uint32_t sourceClockHz, uint32_t sampleRate, uint32_t bitWidth, uint32_t channelNumbers);

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) || \
    (defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
/*!
 * @brief Master clock configurations.
 *
 * @param base SAI base pointer.
 * @param config master clock configurations.
 */
void SAI_SetMasterClockConfig(I2S_Type *base, sai_master_clock_t *config);
#endif

/*!
 * @brief Sends data using a blocking method.
 *
 * @note This function blocks by polling until data is ready to be sent.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be written.
 * @param size Bytes to be written.
 */
void SAI_WriteBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Sends data to multi channel using a blocking method.
 *
 * @note This function blocks by polling until data is ready to be sent.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param channelMask channel mask.
 * @param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be written.
 * @param size Bytes to be written.
 */
void SAI_WriteMultiChannelBlocking(
    I2S_Type *base, uint32_t channel, uint32_t channelMask, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Writes data into SAI FIFO.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param data Data needs to be written.
 */
static inline void SAI_WriteData(I2S_Type *base, uint32_t channel, uint32_t data)
{
    base->TDR[channel] = data;
}

/*!
 * @brief Receives data using a blocking method.
 *
 * @note This function blocks by polling until data is ready to be sent.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be read.
 * @param size Bytes to be read.
 */
void SAI_ReadBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Receives multi channel data using a blocking method.
 *
 * @note This function blocks by polling until data is ready to be sent.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @param channelMask channel mask.
 * @param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be read.
 * @param size Bytes to be read.
 */
void SAI_ReadMultiChannelBlocking(
    I2S_Type *base, uint32_t channel, uint32_t channelMask, uint32_t bitWidth, uint8_t *buffer, uint32_t size);

/*!
 * @brief Reads data from the SAI FIFO.
 *
 * @param base SAI base pointer.
 * @param channel Data channel used.
 * @return Data in SAI FIFO.
 */
static inline uint32_t SAI_ReadData(I2S_Type *base, uint32_t channel)
{
    return base->RDR[channel];
}

/*!
 * @brief Enables/disables the SAI Tx.
 *
 * @param base SAI base pointer.
 * @param enable True means enable SAI Tx, false means disable.
 */
void SAI_TxEnable(I2S_Type *base, bool enable);

/*!
 * @brief Enables/disables the SAI Rx.
 *
 * @param base SAI base pointer.
 * @param enable True means enable SAI Rx, false means disable.
 */
void SAI_RxEnable(I2S_Type *base, bool enable);

/*! @} */

void SAI_WriteReadBlocking(I2S_Type *base, uint32_t channel, uint32_t channelMask, uint32_t bitWidth, uint8_t *buffer_send,uint8_t *buffer_recv, uint32_t size);

#if defined(__cplusplus)
}
#endif /*_cplusplus*/

/*! @} */

#endif /* _FSL_SAI_H_ */
