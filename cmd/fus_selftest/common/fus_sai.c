/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "fus_sai.h"

/*******************************************************************************
 * Definitations
 ******************************************************************************/
/*! @brief _sai_transfer_state sai transfer state.*/
enum
{
    kSAI_Busy = 0x0U, /*!< SAI is busy */
    kSAI_Idle,        /*!< Transfer is done. */
    kSAI_Error        /*!< Transfer error occurred. */
};

/*! @brief Typedef for sai tx interrupt handler. */
typedef void (*sai_tx_isr_t)(I2S_Type *base, sai_handle_t *saiHandle);

/*! @brief Typedef for sai rx interrupt handler. */
typedef void (*sai_rx_isr_t)(I2S_Type *base, sai_handle_t *saiHandle);

/*! @brief check flag avalibility */
#define IS_SAI_FLAG_SET(reg, flag) (((reg) & ((uint32_t)flag)) != 0UL)

static void SAI_WriteNonBlocking(I2S_Type *base,
                                 uint32_t channel,
                                 uint32_t channelMask,
                                 uint32_t endChannel,
                                 uint8_t bitWidth,
                                 uint8_t *buffer,
                                 uint32_t size)
{
    uint32_t i = 0, j = 0U;
    uint8_t m            = 0;
    uint8_t bytesPerWord = bitWidth / 8U;
    uint32_t data        = 0;
    uint32_t temp        = 0;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        for (j = channel; j <= endChannel; j++)
        {
            if (IS_SAI_FLAG_SET((1UL << j), channelMask))
            {
                for (m = 0; m < bytesPerWord; m++)
                {
                    temp = (uint32_t)(*buffer);
                    data |= (temp << (8U * m));
                    buffer++;
                }
                base->TDR[j] = data;
                data         = 0;
            }
        }
    }
}

static void SAI_ReadNonBlocking(I2S_Type *base,
                                uint32_t channel,
                                uint32_t channelMask,
                                uint32_t endChannel,
                                uint8_t bitWidth,
                                uint8_t *buffer,
                                uint32_t size)
{
    uint32_t i = 0, j = 0;
    uint8_t m            = 0;
    uint8_t bytesPerWord = bitWidth / 8U;
    uint32_t data        = 0;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        for (j = channel; j <= endChannel; j++)
        {
            if (IS_SAI_FLAG_SET((1UL << j), channelMask))
            {
                data = base->RDR[j];
                for (m = 0; m < bytesPerWord; m++)
                {
                    *buffer = (uint8_t)(data >> (8U * m)) & 0xFFU;
                    buffer++;
                }
            }
        }
    }
}

static void SAI_GetCommonConfig(sai_transceiver_t *config,
                                sai_word_width_t bitWidth,
                                sai_mono_stereo_t mode,
                                uint32_t saiChannelMask)
{
    assert(NULL != config);
    assert(saiChannelMask != 0U);

    (void)memset(config, 0, sizeof(sai_transceiver_t));

    config->channelMask = (uint8_t)saiChannelMask;
    /* sync mode default configurations */
    config->syncMode = kSAI_ModeAsync;

    /* master mode default */
    config->masterSlave = kSAI_Master;

    /* bit default configurations */
    config->bitClock.bclkSrcSwap    = false;
    config->bitClock.bclkInputDelay = false;
    config->bitClock.bclkPolarity   = kSAI_SampleOnRisingEdge;
    config->bitClock.bclkSource     = kSAI_BclkSourceMclkDiv;

    /* frame sync default configurations */
    config->frameSync.frameSyncWidth = (uint8_t)bitWidth;
    config->frameSync.frameSyncEarly = true;
#if defined(FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND) && FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND
    config->frameSync.frameSyncGenerateOnDemand = false;
#endif
    config->frameSync.frameSyncPolarity = kSAI_PolarityActiveLow;

    /* serial data default configurations */
#if defined(FSL_FEATURE_SAI_HAS_CHANNEL_MODE) && FSL_FEATURE_SAI_HAS_CHANNEL_MODE
    config->serialData.dataMode = kSAI_DataPinStateOutputZero;
#endif
    config->serialData.dataOrder           = kSAI_DataMSB;
    config->serialData.dataWord0Length     = (uint8_t)bitWidth;
    config->serialData.dataWordLength      = (uint8_t)bitWidth;
    config->serialData.dataWordNLength     = (uint8_t)bitWidth;
    config->serialData.dataFirstBitShifted = (uint8_t)bitWidth;
    config->serialData.dataWordNum         = 2U;
    config->serialData.dataMaskedWord      = (uint32_t)mode;

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    /* fifo configurations */
    config->fifo.fifoWatermark = (uint8_t)((uint32_t)FSL_FEATURE_SAI_FIFO_COUNT / 2U);
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR
    config->fifo.fifoContinueOneError = true;
#endif
}

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) || \
    (defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))

static void SAI_SetMasterClockDivider(I2S_Type *base, uint32_t mclk_Hz, uint32_t mclkSrcClock_Hz)
{
    assert(mclk_Hz <= mclkSrcClock_Hz);

    uint32_t sourceFreq = mclkSrcClock_Hz / 100U; /*In order to prevent overflow */
    uint32_t targetFreq = mclk_Hz / 100U;         /*In order to prevent overflow */

#if FSL_FEATURE_SAI_HAS_MCR_MCLK_POST_DIV
    uint32_t postDivider = sourceFreq / targetFreq;

    /* if source equal to target, then disable divider */
    if (postDivider == 1U)
    {
        base->MCR &= ~I2S_MCR_DIVEN_MASK;
    }
    else
    {
        base->MCR = (base->MCR & (~I2S_MCR_DIV_MASK)) | I2S_MCR_DIV(postDivider / 2U - 1U) | I2S_MCR_DIVEN_MASK;
    }
#endif
#if FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER
    uint16_t fract, divide;
    uint32_t remaind           = 0;
    uint32_t current_remainder = 0xFFFFFFFFU;
    uint16_t current_fract     = 0;
    uint16_t current_divide    = 0;
    uint32_t mul_freq          = 0;
    uint32_t max_fract         = 256;

    /* Compute the max fract number */
    max_fract = targetFreq * 4096U / sourceFreq + 1U;
    if (max_fract > 256U)
    {
        max_fract = 256U;
    }

    /* Looking for the closet frequency */
    for (fract = 1; fract < max_fract; fract++)
    {
        mul_freq = sourceFreq * fract;
        remaind  = mul_freq % targetFreq;
        divide   = (uint16_t)(mul_freq / targetFreq);

        /* Find the exactly frequency */
        if (remaind == 0U)
        {
            current_fract  = fract;
            current_divide = (uint16_t)(mul_freq / targetFreq);
            break;
        }

        /* Closer to next one, set the closest to next data */
        if (remaind > mclk_Hz / 2U)
        {
            remaind = targetFreq - remaind;
            divide += 1U;
        }

        /* Update the closest div and fract */
        if (remaind < current_remainder)
        {
            current_fract     = fract;
            current_divide    = divide;
            current_remainder = remaind;
        }
    }

    /* Fill the computed fract and divider to registers */
    base->MDR = I2S_MDR_DIVIDE(current_divide - 1UL) | I2S_MDR_FRACT(current_fract - 1UL);

    /* Waiting for the divider updated */
    while ((base->MCR & I2S_MCR_DUF_MASK) != 0UL)
    {
    }
#endif
}
#endif
/*! @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Gets the SAI Tx status flag state.
 *
 * @param base SAI base pointer
 * @return SAI Tx status flag value. Use the Status Mask to get the status value needed.
 */
static inline uint32_t SAI_TxGetStatusFlag(I2S_Type *base)
{
    return base->TCSR;
}

/*!
 * @brief Clears the SAI Tx status flag state.
 *
 * @param base SAI base pointer
 * @param mask State mask. It can be a combination of the following source if defined:
 *        @arg kSAI_WordStartFlag
 *        @arg kSAI_SyncErrorFlag
 *        @arg kSAI_FIFOErrorFlag
 */
static inline void SAI_TxClearStatusFlags(I2S_Type *base, uint32_t mask)
{
    base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | mask);
}

/*!
 * @brief Gets the SAI Tx status flag state.
 *
 * @param base SAI base pointer
 * @return SAI Rx status flag value. Use the Status Mask to get the status value needed.
 */
static inline uint32_t SAI_RxGetStatusFlag(I2S_Type *base)
{
    return base->RCSR;
}

/*!
 * @brief Clears the SAI Rx status flag state.
 *
 * @param base SAI base pointer
 * @param mask State mask. It can be a combination of the following sources if defined.
 *        @arg kSAI_WordStartFlag
 *        @arg kSAI_SyncErrorFlag
 *        @arg kSAI_FIFOErrorFlag
 */
static inline void SAI_RxClearStatusFlags(I2S_Type *base, uint32_t mask)
{
    base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | mask);
}

void SAI_Init(I2S_Type *base)
{

    /* Enable the SAI clock */
    /* disable interrupt and DMA request*/
    base->TCSR &= ~(I2S_TCSR_FWIE_MASK | I2S_TCSR_FEIE_MASK | I2S_TCSR_FWDE_MASK);
    base->RCSR &= ~(I2S_RCSR_FWIE_MASK | I2S_RCSR_FEIE_MASK | I2S_RCSR_FWDE_MASK);
}

/*!
 * brief Get classic I2S mode configurations.
 *
 * param config transceiver configurations.
 * param bitWidth audio data bitWidth.
 * param mode audio data channel.
 * param saiChannelMask channel mask value to enable.
 */
void SAI_GetClassicI2SConfig(sai_transceiver_t *config,
                             sai_word_width_t bitWidth,
                             sai_mono_stereo_t mode,
                             uint32_t saiChannelMask)
{
    SAI_GetCommonConfig(config, bitWidth, mode, saiChannelMask);
}



/*!
 * brief SAI transmitter configurations.
 *
 * param base SAI base pointer.
 * param config transmitter configurations.
 */
void SAI_TxSetConfig(I2S_Type *base, sai_transceiver_t *config)
{
    assert(config != NULL);
    assert(FSL_FEATURE_SAI_CHANNEL_COUNTn(base) != -1);

    uint8_t i           = 0U;
    uint32_t val        = 0U;
    uint8_t channelNums = 0U;

    /* reset transmitter */
    SAI_TxReset(base);

    /* if channel mask is not set, then format->channel must be set,
     use it to get channel mask value */
    if (config->channelMask == 0U)
    {
        config->channelMask = 1U << config->startChannel;
    }

    for (i = 0U; i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base); i++)
    {
        if (IS_SAI_FLAG_SET(1UL << i, config->channelMask))
        {
            channelNums++;
            config->endChannel = i;
        }
    }

    for (i = 0U; i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base); i++)
    {
        if (IS_SAI_FLAG_SET((1UL << i), config->channelMask))
        {
            config->startChannel = i;
            break;
        }
    }

    config->channelNums = channelNums;
#if defined(FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE) && (FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE)
    /* make sure combine mode disabled while multipe channel is used */
    if (config->channelNums > 1U)
    {
        base->TCR4 &= ~I2S_TCR4_FCOMB_MASK;
    }
#endif

    /* Set data channel */
    base->TCR3 &= ~I2S_TCR3_TCE_MASK;
    base->TCR3 |= I2S_TCR3_TCE(config->channelMask);

    if (config->syncMode == kSAI_ModeAsync)
    {
        val = base->TCR2;
        val &= ~I2S_TCR2_SYNC_MASK;
        base->TCR2 = (val | I2S_TCR2_SYNC(0U));
    }
    if (config->syncMode == kSAI_ModeSync)
    {
        val = base->TCR2;
        val &= ~I2S_TCR2_SYNC_MASK;
        base->TCR2 = (val | I2S_TCR2_SYNC(1U));
        /* If sync with Rx, should set Rx to async mode */
        val = base->RCR2;
        val &= ~I2S_RCR2_SYNC_MASK;
        base->RCR2 = (val | I2S_RCR2_SYNC(0U));
    }
#if defined(FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI) && (FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI)
    if (config->syncMode == kSAI_ModeSyncWithOtherTx)
    {
        val = base->TCR2;
        val &= ~I2S_TCR2_SYNC_MASK;
        base->TCR2 = (val | I2S_TCR2_SYNC(2U));
    }
    if (config->syncMode == kSAI_ModeSyncWithOtherRx)
    {
        val = base->TCR2;
        val &= ~I2S_TCR2_SYNC_MASK;
        base->TCR2 = (val | I2S_TCR2_SYNC(3U));
    }
#endif /* FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI */

    /* bit clock configurations */
    SAI_TxSetBitclockConfig(base, config->masterSlave, &config->bitClock);
    /* serial data configurations */
    SAI_TxSetSerialDataConfig(base, &config->serialData);
    /* frame sync configurations */
    SAI_TxSetFrameSyncConfig(base, config->masterSlave, &config->frameSync);
    /* fifo configurations */
    SAI_TxSetFifoConfig(base, &config->fifo);


}

/*!
 * brief SAI receiver configurations.
 *
 * param base SAI base pointer.
 * param config transmitter configurations.
 */
void SAI_RxSetConfig(I2S_Type *base, sai_transceiver_t *config)
{
    assert(config != NULL);
    assert(FSL_FEATURE_SAI_CHANNEL_COUNTn(base) != -1);

    uint8_t i           = 0U;
    uint32_t val        = 0U;
    uint8_t channelNums = 0U;

    /* reset receiver */
    SAI_RxReset(base);

    /* if channel mask is not set, then format->channel must be set,
     use it to get channel mask value */
    if (config->channelMask == 0U)
    {
        config->channelMask = 1U << config->startChannel;
    }

    for (i = 0U; i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base); i++)
    {
        if (IS_SAI_FLAG_SET((1UL << i), config->channelMask))
        {
            channelNums++;
            config->endChannel = i;
        }
    }

    for (i = 0U; i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base); i++)
    {
        if (IS_SAI_FLAG_SET((1UL << i), config->channelMask))
        {
            config->startChannel = i;
            break;
        }
    }

    config->channelNums = channelNums;
#if defined(FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE) && (FSL_FEATURE_SAI_HAS_FIFO_COMBINE_MODE)
    /* make sure combine mode disabled while multipe channel is used */
    if (config->channelNums > 1U)
    {
        base->RCR4 &= ~I2S_RCR4_FCOMB_MASK;
    }
#endif

    /* Set data channel */
    base->RCR3 &= ~I2S_RCR3_RCE_MASK;
    base->RCR3 |= I2S_RCR3_RCE(config->channelMask);

    /* Set Sync mode */
    if (config->syncMode == kSAI_ModeAsync)
    {
        val = base->RCR2;
        val &= ~I2S_RCR2_SYNC_MASK;
        base->RCR2 = (val | I2S_RCR2_SYNC(0U));
    }
    if (config->syncMode == kSAI_ModeSync)
    {
        val = base->RCR2;
        val &= ~I2S_RCR2_SYNC_MASK;
        base->RCR2 = (val | I2S_RCR2_SYNC(1U));
        /* If sync with Tx, should set Tx to async mode */
        val = base->TCR2;
        val &= ~I2S_TCR2_SYNC_MASK;
        base->TCR2 = (val | I2S_TCR2_SYNC(0U));
    }
#if defined(FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI) && (FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI)
    if (config->syncMode == kSAI_ModeSyncWithOtherTx)
    {
        val = base->RCR2;
        val &= ~I2S_RCR2_SYNC_MASK;
        base->RCR2 = (val | I2S_RCR2_SYNC(2U));
    }
    if (config->syncMode == kSAI_ModeSyncWithOtherRx)
    {
        val = base->RCR2;
        val &= ~I2S_RCR2_SYNC_MASK;
        base->RCR2 = (val | I2S_RCR2_SYNC(3U));
    }
#endif /* FSL_FEATURE_SAI_HAS_SYNC_WITH_ANOTHER_SAI */

    /* bit clock configurations */
    SAI_RxSetBitclockConfig(base, config->masterSlave, &config->bitClock);
    /* serial data configurations */
    SAI_RxSetSerialDataConfig(base, &config->serialData);
    /* frame sync configurations */
    SAI_RxSetFrameSyncConfig(base, config->masterSlave, &config->frameSync);
    /* fifo configurations */
    SAI_RxSetFifoConfig(base, &config->fifo);
}

/*!
 * brief Resets the SAI Tx.
 *
 * This function enables the software reset and FIFO reset of SAI Tx. After reset, clear the reset bit.
 *
 * param base SAI base pointer
 */
void SAI_TxReset(I2S_Type *base)
{
    /* Set the software reset and FIFO reset to clear internal state */
    base->TCSR = I2S_TCSR_SR_MASK | I2S_TCSR_FR_MASK;

    /* Clear software reset bit, this should be done by software */
    base->TCSR &= ~I2S_TCSR_SR_MASK;

    /* Reset all Tx register values */
    base->TCR2 = 0;
    base->TCR3 = 0;
    base->TCR4 = 0;
    base->TCR5 = 0;
    base->TMR  = 0;
}

/*!
 * brief Resets the SAI Rx.
 *
 * This function enables the software reset and FIFO reset of SAI Rx. After reset, clear the reset bit.
 *
 * param base SAI base pointer
 */
void SAI_RxReset(I2S_Type *base)
{
    /* Set the software reset and FIFO reset to clear internal state */
    base->RCSR = I2S_RCSR_SR_MASK | I2S_RCSR_FR_MASK;

    /* Clear software reset bit, this should be done by software */
    base->RCSR &= ~I2S_RCSR_SR_MASK;

    /* Reset all Rx register values */
    base->RCR2 = 0;
    base->RCR3 = 0;
    base->RCR4 = 0;
    base->RCR5 = 0;
    base->RMR  = 0;
}

/*!
 * brief Transmitter Bit clock configurations.
 *
 * param base SAI base pointer.
 * param masterSlave master or slave.
 * param config bit clock other configurations, can be NULL in slave mode.
 */
void SAI_TxSetBitclockConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_bit_clock_t *config)
{
    uint32_t tcr2 = base->TCR2;

    if ((masterSlave == kSAI_Master) || (masterSlave == kSAI_Bclk_Master_FrameSync_Slave))
    {
        assert(config != NULL);

        tcr2 &= ~(I2S_TCR2_BCD_MASK | I2S_TCR2_BCP_MASK | I2S_TCR2_BCI_MASK | I2S_TCR2_BCS_MASK | I2S_TCR2_MSEL_MASK);
        tcr2 |= I2S_TCR2_BCD(1U) | I2S_TCR2_BCP(config->bclkPolarity) | I2S_TCR2_BCI(config->bclkInputDelay) |
                I2S_TCR2_BCS(config->bclkSrcSwap) | I2S_TCR2_MSEL(config->bclkSource);
    }
    else
    {
        tcr2 &= ~(I2S_TCR2_BCD_MASK);
    }

    base->TCR2 = tcr2;
}

/*!
 * brief Receiver Bit clock configurations.
 *
 * param base SAI base pointer.
 * param masterSlave master or slave.
 * param config bit clock other configurations, can be NULL in slave mode.
 */
void SAI_RxSetBitclockConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_bit_clock_t *config)
{
    uint32_t rcr2 = base->RCR2;

    if ((masterSlave == kSAI_Master) || (masterSlave == kSAI_Bclk_Master_FrameSync_Slave))
    {
        assert(config != NULL);

        rcr2 &= ~(I2S_RCR2_BCD_MASK | I2S_RCR2_BCP_MASK | I2S_RCR2_BCI_MASK | I2S_RCR2_BCS_MASK | I2S_RCR2_MSEL_MASK);
        rcr2 |= I2S_RCR2_BCD(1U) | I2S_RCR2_BCP(config->bclkPolarity) | I2S_RCR2_BCI(config->bclkInputDelay) |
                I2S_RCR2_BCS(config->bclkSrcSwap) | I2S_RCR2_MSEL(config->bclkSource);
    }
    else
    {
        rcr2 &= ~(I2S_RCR2_BCD_MASK);
    }

    base->RCR2 = rcr2;
}

/*!
 * brief SAI transmitter fifo configurations.
 *
 * param base SAI base pointer.
 * param config fifo configurations.
 */
void SAI_TxSetFifoConfig(I2S_Type *base, sai_fifo_t *config)
{
    assert(config != NULL);
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    assert(config->fifoWatermark <= (I2S_TCR1_TFW_MASK >> I2S_TCR1_TFW_SHIFT));
#endif

    uint32_t tcr4 = base->TCR4;

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE
    tcr4 &= ~I2S_TCR4_FCOMB_MASK;
    tcr4 |= I2S_TCR4_FCOMB(config->fifoCombine);
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR
    tcr4 &= ~I2S_TCR4_FCONT_MASK;
    /* ERR05144: not set FCONT = 1 when TMR > 0, the transmit shift register may not load correctly that will cause TX
     * not work */
    if (base->TMR == 0U)
    {
        tcr4 |= I2S_TCR4_FCONT(config->fifoContinueOneError);
    }
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_PACKING) && FSL_FEATURE_SAI_HAS_FIFO_PACKING
    tcr4 &= ~I2S_TCR4_FPACK_MASK;
    tcr4 |= I2S_TCR4_FPACK(config->fifoPacking);
#endif

    base->TCR4 = tcr4;

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    base->TCR1 = (base->TCR1 & (~I2S_TCR1_TFW_MASK)) | I2S_TCR1_TFW(config->fifoWatermark);
#endif
}

/*!
 * brief SAI receiver fifo configurations.
 *
 * param base SAI base pointer.
 * param config fifo configurations.
 */
void SAI_RxSetFifoConfig(I2S_Type *base, sai_fifo_t *config)
{
    assert(config != NULL);
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    assert(config->fifoWatermark <= (I2S_TCR1_TFW_MASK >> I2S_TCR1_TFW_SHIFT));
#endif
    uint32_t rcr4 = base->RCR4;

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_COMBINE
    rcr4 &= ~I2S_RCR4_FCOMB_MASK;
    rcr4 |= I2S_RCR4_FCOMB(config->fifoCombine);
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR
    rcr4 &= ~I2S_RCR4_FCONT_MASK;
    rcr4 |= I2S_RCR4_FCONT(config->fifoContinueOneError);
#endif

#if defined(FSL_FEATURE_SAI_HAS_FIFO_PACKING) && FSL_FEATURE_SAI_HAS_FIFO_PACKING
    rcr4 &= ~I2S_RCR4_FPACK_MASK;
    rcr4 |= I2S_RCR4_FPACK(config->fifoPacking);
#endif

    base->RCR4 = rcr4;

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    base->RCR1 = (base->RCR1 & (~I2S_RCR1_RFW_MASK)) | I2S_RCR1_RFW(config->fifoWatermark);
#endif
}

/*!
 * brief SAI transmitter Frame sync configurations.
 *
 * param base SAI base pointer.
 * param masterSlave master or slave.
 * param config frame sync configurations, can be NULL in slave mode.
 */
void SAI_TxSetFrameSyncConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_frame_sync_t *config)
{
    uint32_t tcr4 = base->TCR4;

    if ((masterSlave == kSAI_Master) || (masterSlave == kSAI_Bclk_Slave_FrameSync_Master))
    {
        assert(config != NULL);
        assert((config->frameSyncWidth - 1UL) <= (I2S_TCR4_SYWD_MASK >> I2S_TCR4_SYWD_SHIFT));

        tcr4 &= ~(I2S_TCR4_FSE_MASK | I2S_TCR4_FSP_MASK | I2S_TCR4_FSD_MASK | I2S_TCR4_SYWD_MASK);

#if defined(FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND) && FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND
        tcr4 &= ~I2S_TCR4_ONDEM_MASK;
        tcr4 |= I2S_TCR4_ONDEM(config->frameSyncGenerateOnDemand);
#endif

        tcr4 |= I2S_TCR4_FSE(config->frameSyncEarly) | I2S_TCR4_FSP(config->frameSyncPolarity) | I2S_TCR4_FSD(1UL) |
                I2S_TCR4_SYWD(config->frameSyncWidth - 1UL);
    }
    else
    {
        tcr4 &= ~I2S_TCR4_FSD_MASK;
    }

    base->TCR4 = tcr4;
}

/*!
 * brief SAI receiver Frame sync configurations.
 *
 * param base SAI base pointer.
 * param masterSlave master or slave.
 * param config frame sync configurations, can be NULL in slave mode.
 */
void SAI_RxSetFrameSyncConfig(I2S_Type *base, sai_master_slave_t masterSlave, sai_frame_sync_t *config)
{
    uint32_t rcr4 = base->RCR4;

    if ((masterSlave == kSAI_Master) || (masterSlave == kSAI_Bclk_Slave_FrameSync_Master))
    {
        assert(config != NULL);
        assert((config->frameSyncWidth - 1UL) <= (I2S_RCR4_SYWD_MASK >> I2S_RCR4_SYWD_SHIFT));

        rcr4 &= ~(I2S_RCR4_FSE_MASK | I2S_RCR4_FSP_MASK | I2S_RCR4_FSD_MASK | I2S_RCR4_SYWD_MASK);

#if defined(FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND) && FSL_FEATURE_SAI_HAS_FRAME_SYNC_ON_DEMAND
        rcr4 &= ~I2S_RCR4_ONDEM_MASK;
        rcr4 |= I2S_RCR4_ONDEM(config->frameSyncGenerateOnDemand);
#endif

        rcr4 |= I2S_RCR4_FSE(config->frameSyncEarly) | I2S_RCR4_FSP(config->frameSyncPolarity) | I2S_RCR4_FSD(1UL) |
                I2S_RCR4_SYWD(config->frameSyncWidth - 1UL);
    }
    else
    {
        rcr4 &= ~I2S_RCR4_FSD_MASK;
    }

    base->RCR4 = rcr4;
}

/*!
 * brief SAI transmitter Serial data configurations.
 *
 * param base SAI base pointer.
 * param config serial data configurations.
 */
void SAI_TxSetSerialDataConfig(I2S_Type *base, sai_serial_data_t *config)
{
    assert(config != NULL);

    uint32_t tcr4 = base->TCR4;

    base->TCR5 = I2S_TCR5_WNW(config->dataWordNLength - 1UL) | I2S_TCR5_W0W(config->dataWord0Length - 1UL) |
                 I2S_TCR5_FBT(config->dataFirstBitShifted - 1UL);
    base->TMR = config->dataMaskedWord;
#if defined(FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR) && FSL_FEATURE_SAI_HAS_FIFO_FUNCTION_AFTER_ERROR
    /* ERR05144: not set FCONT = 1 when TMR > 0, the transmit shift register may not load correctly that will cause TX
     * not work */
    if (config->dataMaskedWord > 0U)
    {
        tcr4 &= ~I2S_TCR4_FCONT_MASK;
    }
#endif
    tcr4 &= ~(I2S_TCR4_FRSZ_MASK | I2S_TCR4_MF_MASK);
    tcr4 |= I2S_TCR4_FRSZ(config->dataWordNum - 1UL) | I2S_TCR4_MF(config->dataOrder);

#if defined(FSL_FEATURE_SAI_HAS_CHANNEL_MODE) && FSL_FEATURE_SAI_HAS_CHANNEL_MODE
    tcr4 &= ~I2S_TCR4_CHMOD_MASK;
    tcr4 |= I2S_TCR4_CHMOD(config->dataMode);
#endif

    base->TCR4 = tcr4;
}

/*!
 * @brief SAI receiver Serial data configurations.
 *
 * @param base SAI base pointer.
 * @param config serial data configurations.
 */
void SAI_RxSetSerialDataConfig(I2S_Type *base, sai_serial_data_t *config)
{
    assert(config != NULL);

    uint32_t rcr4 = base->RCR4;

    base->RCR5 = I2S_RCR5_WNW(config->dataWordNLength - 1UL) | I2S_RCR5_W0W(config->dataWord0Length - 1UL) |
                 I2S_RCR5_FBT(config->dataFirstBitShifted - 1UL);
    base->RMR = config->dataMaskedWord;

    rcr4 &= ~(I2S_RCR4_FRSZ_MASK | I2S_RCR4_MF_MASK);
    rcr4 |= I2S_RCR4_FRSZ(config->dataWordNum - 1uL) | I2S_RCR4_MF(config->dataOrder);

    base->RCR4 = rcr4;
}

void SAI_TxSetBitClockRate(
    I2S_Type *base, uint32_t sourceClockHz, uint32_t sampleRate, uint32_t bitWidth, uint32_t channelNumbers)
{
    uint32_t tcr2         = base->TCR2;
    uint32_t bitClockDiv  = 0;
    uint32_t bitClockFreq = sampleRate * bitWidth * channelNumbers;

    assert(sourceClockHz >= bitClockFreq);

    tcr2 &= ~I2S_TCR2_DIV_MASK;
    /* need to check the divided bclk, if bigger than target, then divider need to re-calculate. */
    bitClockDiv = sourceClockHz / bitClockFreq;
    /* for the condition where the source clock is smaller than target bclk */
    if (bitClockDiv == 0U)
    {
        bitClockDiv++;
    }
    /* recheck the divider if properly or not, to make sure output blck not bigger than target*/
    if ((sourceClockHz / bitClockDiv) > bitClockFreq)
    {
        bitClockDiv++;
    }

#if defined(FSL_FEATURE_SAI_HAS_BCLK_BYPASS) && (FSL_FEATURE_SAI_HAS_BCLK_BYPASS)
    /* if bclk same with MCLK, bypass the divider */
    if (bitClockDiv == 1U)
    {
        tcr2 |= I2S_TCR2_BYP_MASK;
    }
    else
#endif
    {
        tcr2 |= I2S_TCR2_DIV(bitClockDiv / 2U - 1UL);
    }

    base->TCR2 = tcr2;
}

/*!
 * brief Receiver bit clock rate configurations.
 *
 * param base SAI base pointer.
 * param sourceClockHz, bit clock source frequency.
 * param sampleRate audio data sample rate.
 * param bitWidth, audio data bitWidth.
 * param channelNumbers, audio channel numbers.
 */
void SAI_RxSetBitClockRate(
    I2S_Type *base, uint32_t sourceClockHz, uint32_t sampleRate, uint32_t bitWidth, uint32_t channelNumbers)
{
    uint32_t rcr2         = base->RCR2;
    uint32_t bitClockDiv  = 0;
    uint32_t bitClockFreq = sampleRate * bitWidth * channelNumbers;

    assert(sourceClockHz >= bitClockFreq);

    rcr2 &= ~I2S_RCR2_DIV_MASK;
    /* need to check the divided bclk, if bigger than target, then divider need to re-calculate. */
    bitClockDiv = sourceClockHz / bitClockFreq;
    /* for the condition where the source clock is smaller than target bclk */
    if (bitClockDiv == 0U)
    {
        bitClockDiv++;
    }
    /* recheck the divider if properly or not, to make sure output blck not bigger than target*/
    if ((sourceClockHz / bitClockDiv) > bitClockFreq)
    {
        bitClockDiv++;
    }

#if defined(FSL_FEATURE_SAI_HAS_BCLK_BYPASS) && (FSL_FEATURE_SAI_HAS_BCLK_BYPASS)
    /* if bclk same with MCLK, bypass the divider */
    if (bitClockDiv == 1U)
    {
        rcr2 |= I2S_RCR2_BYP_MASK;
    }
    else
#endif
    {
        rcr2 |= I2S_RCR2_DIV(bitClockDiv / 2U - 1UL);
    }

    base->RCR2 = rcr2;
}

#if (defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)) || \
    (defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER))
/*!
 * brief Master clock configurations.
 *
 * param base SAI base pointer.
 * param config master clock configurations.
 */
void SAI_SetMasterClockConfig(I2S_Type *base, sai_master_clock_t *config)
{
    assert(config != NULL);



#if defined(FSL_FEATURE_SAI_HAS_MCR) && (FSL_FEATURE_SAI_HAS_MCR)
    uint32_t val = 0;
#if !(defined(FSL_FEATURE_SAI_HAS_NO_MCR_MICS) && (FSL_FEATURE_SAI_HAS_NO_MCR_MICS))
    /* Master clock source setting */
    val       = (base->MCR & ~I2S_MCR_MICS_MASK);
    base->MCR = (val | I2S_MCR_MICS(config->mclkSource));
#endif



    /* Configure Master clock output enable */
    val       = (base->MCR & ~I2S_MCR_MOE_MASK);
    base->MCR = (val | I2S_MCR_MOE(config->mclkOutputEnable));
#endif /* FSL_FEATURE_SAI_HAS_MCR */



#if ((defined(FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) && (FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER)) || \
     (defined(FSL_FEATURE_SAI_HAS_MCR_MCLK_POST_DIV) && (FSL_FEATURE_SAI_HAS_MCR_MCLK_POST_DIV)))
    /* Check if master clock divider enabled, then set master clock divider */
    if (config->mclkOutputEnable)
    {
        SAI_SetMasterClockDivider(base, config->mclkHz, config->mclkSourceClkHz);
    }
#endif /* FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER */
}
#endif

/*!
 * brief Sends data using a blocking method.
 *
 * note This function blocks by polling until data is ready to be sent.
 *
 * param base SAI base pointer.
 * param channel Data channel used.
 * param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * param buffer Pointer to the data to be written.
 * param size Bytes to be written.
 */
void SAI_WriteBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    uint32_t i            = 0;
    uint32_t bytesPerWord = bitWidth / 8U;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    bytesPerWord = (((uint32_t)FSL_FEATURE_SAI_FIFO_COUNT - base->TCR1) * bytesPerWord);
#endif

    while (i < size)
    {
        /* Wait until it can write data */
        while (!(IS_SAI_FLAG_SET(base->TCSR, I2S_TCSR_FWF_MASK)))
        {
        }
        SAI_WriteNonBlocking(base, channel, 1UL << channel, channel, (uint8_t)bitWidth, buffer, bytesPerWord);
        buffer += bytesPerWord;
        i += bytesPerWord;
    }

    /* Wait until the last data is sent */
    while (!(IS_SAI_FLAG_SET(base->TCSR, I2S_TCSR_FWF_MASK)))
    {
    }
}

/*!
 * brief Sends data to multi channel using a blocking method.
 *
 * note This function blocks by polling until data is ready to be sent.
 *
 * param base SAI base pointer.
 * param channel Data channel used.
 * param channelMask channel mask.
 * param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * param buffer Pointer to the data to be written.
 * param size Bytes to be written.
 */
void SAI_WriteMultiChannelBlocking(
    I2S_Type *base, uint32_t channel, uint32_t channelMask, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    assert(FSL_FEATURE_SAI_CHANNEL_COUNTn(base) != -1);

    uint32_t i = 0, j = 0;
    uint32_t bytesPerWord = bitWidth / 8U;
    uint32_t channelNums = 0U, endChannel = 0U;

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    bytesPerWord = (((uint32_t)FSL_FEATURE_SAI_FIFO_COUNT - base->TCR1) * bytesPerWord);
#endif

    for (i = 0U; (i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base)); i++)
    {
        if (IS_SAI_FLAG_SET((1UL << i), channelMask))
        {
            channelNums++;
            endChannel = i;
        }
    }

    bytesPerWord *= channelNums;

    while (j < size)
    {
        /* Wait until it can write data */
        while (!(IS_SAI_FLAG_SET(base->TCSR, I2S_TCSR_FWF_MASK)))
        {
        }

        SAI_WriteNonBlocking(base, channel, channelMask, endChannel, (uint8_t)bitWidth, buffer, bytesPerWord);
        buffer += bytesPerWord;
        j += bytesPerWord;
    }

    /* Wait until the last data is sent */
    while (!(IS_SAI_FLAG_SET(base->TCSR, I2S_TCSR_FWF_MASK)))
    {
    }
}

/*!
 * brief Receives multi channel data using a blocking method.
 *
 * note This function blocks by polling until data is ready to be sent.
 *
 * param base SAI base pointer.
 * param channel Data channel used.
 * param channelMask channel mask.
 * param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * param buffer Pointer to the data to be read.
 * param size Bytes to be read.
 */
void SAI_ReadMultiChannelBlocking(
    I2S_Type *base, uint32_t channel, uint32_t channelMask, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    assert(FSL_FEATURE_SAI_CHANNEL_COUNTn(base) != -1);

    uint32_t i = 0, j = 0;
    uint32_t bytesPerWord = bitWidth / 8U;
    uint32_t channelNums = 0U, endChannel = 0U;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    bytesPerWord = base->RCR1 * bytesPerWord;
#endif
    for (i = 0U; (i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base)); i++)
    {
        if (IS_SAI_FLAG_SET((1UL << i), channelMask))
        {
            channelNums++;
            endChannel = i;
        }
    }

    bytesPerWord *= channelNums;

    while (j < size)
    {
        /* Wait until data is received */
        while (!(IS_SAI_FLAG_SET(base->RCSR, I2S_RCSR_FWF_MASK)))
        {
        }

        SAI_ReadNonBlocking(base, channel, channelMask, endChannel, (uint8_t)bitWidth, buffer, bytesPerWord);
        buffer += bytesPerWord;
        j += bytesPerWord;
    }
}

/*!
 * brief Receives data using a blocking method.
 *
 * note This function blocks by polling until data is ready to be sent.
 *
 * param base SAI base pointer.
 * param channel Data channel used.
 * param bitWidth How many bits in an audio word; usually 8/16/24/32 bits.
 * param buffer Pointer to the data to be read.
 * param size Bytes to be read.
 */
void SAI_ReadBlocking(I2S_Type *base, uint32_t channel, uint32_t bitWidth, uint8_t *buffer, uint32_t size)
{
    uint32_t i            = 0;
    uint32_t bytesPerWord = bitWidth / 8U;
#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)
    bytesPerWord = base->RCR1 * bytesPerWord;
#endif

    while (i < size)
    {
        /* Wait until data is received */
        while (!(IS_SAI_FLAG_SET(base->RCSR, I2S_RCSR_FWF_MASK)))
        {
        }

        SAI_ReadNonBlocking(base, channel, 1UL << channel, channel, (uint8_t)bitWidth, buffer, bytesPerWord);
        buffer += bytesPerWord;
        i += bytesPerWord;
    }
}

/*!
 * brief Enables/disables the SAI Tx.
 *
 * param base SAI base pointer
 * param enable True means enable SAI Tx, false means disable.
 */
void SAI_TxEnable(I2S_Type *base, bool enable)
{
    if (enable)
    {
        /* If clock is sync with Rx, should enable RE bit. */
        if (((base->TCR2 & I2S_TCR2_SYNC_MASK) >> I2S_TCR2_SYNC_SHIFT) == 0x1U)
        {
            base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | I2S_RCSR_RE_MASK);
        }
        base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | I2S_TCSR_TE_MASK);
        /* Also need to clear the FIFO error flag before start */
        SAI_TxClearStatusFlags(base, kSAI_FIFOErrorFlag);
    }
    else
    {
        /* If Rx not in sync with Tx, then disable Tx, otherwise, shall not disable Tx */
        if (((base->RCR2 & I2S_RCR2_SYNC_MASK) >> I2S_RCR2_SYNC_SHIFT) != 0x1U)
        {
            /* Disable TE bit */
            base->TCSR = ((base->TCSR & 0xFFE3FFFFU) & (~I2S_TCSR_TE_MASK));
        }
    }
}

/*!
 * brief Enables/disables the SAI Rx.
 *
 * param base SAI base pointer
 * param enable True means enable SAI Rx, false means disable.
 */
void SAI_RxEnable(I2S_Type *base, bool enable)
{
    if (enable)
    {
        /* If clock is sync with Tx, should enable TE bit. */
        if (((base->RCR2 & I2S_RCR2_SYNC_MASK) >> I2S_RCR2_SYNC_SHIFT) == 0x1U)
        {
            base->TCSR = ((base->TCSR & 0xFFE3FFFFU) | I2S_TCSR_TE_MASK);
        }
        base->RCSR = ((base->RCSR & 0xFFE3FFFFU) | I2S_RCSR_RE_MASK);
        /* Also need to clear the FIFO error flag before start */
        SAI_RxClearStatusFlags(base, kSAI_FIFOErrorFlag);
    }
    else
    {
        /* If Tx not in sync with Rx, then disable Rx, otherwise, shall not disable Rx */
        if (((base->TCR2 & I2S_TCR2_SYNC_MASK) >> I2S_TCR2_SYNC_SHIFT) != 0x1U)
        {
            /* Disable RE bit */
            base->RCSR = ((base->RCSR & 0xFFE3FFFFU) & (~I2S_RCSR_RE_MASK));
        }
    }
}

void SAI_WriteReadBlocking(I2S_Type *base, uint32_t channel, uint32_t channelMask, uint32_t bitWidth, uint8_t *buffer_send,uint8_t *buffer_recv, uint32_t size)
{
    uint32_t i            = 0;
    uint32_t bytesPerWord = bitWidth / 8U;
    uint32_t channelNums = 0U, endChannel = 0U;

#if defined(FSL_FEATURE_SAI_FIFO_COUNT) && (FSL_FEATURE_SAI_FIFO_COUNT > 1)

    bytesPerWord = (((uint32_t)FSL_FEATURE_SAI_FIFO_COUNT - base->TCR1) * bytesPerWord);
#endif

    for (i = 0U; (i < (uint32_t)FSL_FEATURE_SAI_CHANNEL_COUNTn(base)); i++)
    {
        if (IS_SAI_FLAG_SET((1UL << i), channelMask))
        {
            channelNums++;
            endChannel = i;
        }
    }
    while (i < size)
    {
        /* Wait until it can write data */
        while (!(IS_SAI_FLAG_SET(base->TCSR, I2S_TCSR_FWF_MASK)))
        {
        }
        SAI_WriteNonBlocking(base, channel, channelMask, endChannel, (uint8_t)bitWidth, buffer_send, bytesPerWord);
        /* Wait until data is received */
        while (!(IS_SAI_FLAG_SET(base->RCSR, I2S_RCSR_FWF_MASK)))
        {
        }
        SAI_ReadNonBlocking(base, channel, channelMask, endChannel, (uint8_t)bitWidth, buffer_recv, bytesPerWord);
        buffer_send += bytesPerWord;
        buffer_recv += bytesPerWord;
        i += bytesPerWord;
    }
    /* Read double ammount of data */
    while (i < size*2){
		while (!(IS_SAI_FLAG_SET(base->RCSR, I2S_RCSR_FWF_MASK)))
		{
		}
		SAI_ReadNonBlocking(base, channel, channelMask, endChannel, (uint8_t)bitWidth, buffer_recv, bytesPerWord);
        buffer_recv += bytesPerWord;
        i += bytesPerWord;
    }
}
