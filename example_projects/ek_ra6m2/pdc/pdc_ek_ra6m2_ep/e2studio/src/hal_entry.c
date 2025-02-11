/***********************************************************************************************************************
 * File Name    : hal_entry.c
 * Description  : Contains data structures and functions used in hal_entry.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

#include "ov7670.h"
#include "pdc_ep.h"
#include "common_utils.h"

/*******************************************************************************************************************//**
 * @addtogroup pdc_ep
 * @{
 **********************************************************************************************************************/

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

/* Initialize capture ready flag to false. This gets set to true in PDC callback upon successful frame capture. */
static volatile bool g_capture_ready = false;

static void handle_error(fsp_err_t err, char *err_str,module_name_t module);
static void swap_32bit(uint8_t * const p_buffer, uint32_t width, uint32_t height);
static void swap_16bit(uint8_t * const p_buffer, uint32_t width, uint32_t height);

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void) {

    fsp_pack_version_t version                 = {RESET_VALUE};
    fsp_err_t          err                     = FSP_SUCCESS;
    uint8_t            rByte[BUFFER_SIZE_DOWN] = {RESET_VALUE};
    uint32_t           timeout_ms              = UINT32_MAX;

    /* Version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example Project information printed on the RTT */
    APP_PRINT (BANNER_INFO, EP_VERSION, version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    APP_PRINT (EP_INFO);

    /* Initialize PDC */
    err = R_PDC_Open( &g_pdc_ctrl, &g_pdc_cfg);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** R_PDC_Open API FAILED ** \r\n");
        APP_ERR_TRAP(err);
    }

    /* Now the PDC is outputting a clock to the camera, configuring the ov7670 camera */
    /* Setup the camera */
    err = ov7670_setup();
    if(FSP_SUCCESS != err)
    {
        /* cleanup and close PDC operation*/
        handle_error(err, "\r\n** Camera Setup Failed **\r\n",PDC_MODULE);
    }

    APP_PRINT("Camera Setup is Successful\r\n");
    APP_PRINT("\r\nPress any key to capture Image\r\n");

    while(true)
    {
        /*If user input from rtt is valid perform capture operation */
        if (APP_CHECK_DATA)
        {
            APP_READ(rByte);
            /* Reset user buffer */
            memset(g_user_buffer, RESET_VALUE, sizeof(g_user_buffer));
            /* Notify user the start of Capture operation*/
            APP_PRINT("\r\nImage Capturing Operation started\r\n");

            /* Start image capture */
            /* Store the result into the buffer */
            err = R_PDC_CaptureStart(&g_pdc_ctrl, g_user_buffer[RESET_VALUE]);
            /* Handle error */
            if(FSP_SUCCESS != err)
            {
                /*cleanup and close pdc operation*/
                handle_error(err,"\r\n** R_PDC_CaptureStart API FAILED **\r\n",ALL);
            }

            /*  Wait until Callback triggers*/
            while (true != g_capture_ready)
            {
                timeout_ms--;;
                if (RESET_VALUE == timeout_ms)
                {
                    err = FSP_ERR_TIMEOUT;
                    /* we have reached a scenario where transfer complete event is not received */
                    handle_error(err,"\r\n** Callback event not received **\r\n",ALL);

                }
            }

            APP_PRINT("PDC Capture Successful.\r\n");

            /* Example project is using OV7670 camera which support big-endian format only while RA MCU are little-endian.
             * In additional, camera transfer 16bits while RA MCU receive 32 bits data.
             * So, it is necessary to swap data to be able to display correct camera image image. */
            swap_32bit(g_user_buffer[RESET_VALUE], g_pdc_cfg.x_capture_pixels, g_pdc_cfg.y_capture_pixels);
            swap_16bit(g_user_buffer[RESET_VALUE], g_pdc_cfg.x_capture_pixels, g_pdc_cfg.y_capture_pixels);

            /*Reset the flag*/
            g_capture_ready = false;
            APP_PRINT("\r\nPress any key to capture Image\r\n");
        }
    }
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event) {
    if (BSP_WARM_START_RESET == event) {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event) {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
    }
}

/*******************************************************************************************************************//**
 *  @brief      pdc callback function
 *  @param[in]  p_args
 *  @retval     None
 **********************************************************************************************************************/
void g_pdc_user_callback(pdc_callback_args_t *p_args)
{
    if (PDC_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        g_capture_ready = true;
    }
}

/*******************************************************************************************************************//**
 *  @brief      close pdc and iic instances and handle the error.
 *  @param[in]  err     Error received while operation.
 *  @param[in]  err_str Error message
 *  @param[in]  module  Module name to close
 *  @retval     None
 **********************************************************************************************************************/
static void handle_error(fsp_err_t err, char *err_str, module_name_t module)
{
    switch (module)
    {
        case PDC_MODULE:
        {
            /* close pdc module */
            if (FSP_SUCCESS != R_PDC_Close(&g_pdc_ctrl))
            {
                APP_ERR_PRINT ("\r\n**R_PDC_Close API Failed ** \r\n ");
            }
        }
        break;
        case IIC_MODULE:
        {
            /* close IIC instance */
            ov7670_close();
        }
        break;
        case ALL:
        default:
        {
            if (FSP_SUCCESS != R_PDC_Close(&g_pdc_ctrl))
            {
                APP_ERR_PRINT ("\r\n**R_PDC_Close API Failed ** \r\n ");
            }
            /* close IIC instance */
            ov7670_close();
        }
        break;
    }

    APP_ERR_PRINT(err_str);
    APP_ERR_TRAP(err);
}

/*******************************************************************************************************************//**
 *  @brief      swap byte positions in 32 bit data, equivalent to 2 pixels in image buffer.
 *  @param[in]  p_buffer    pointer to image buffer
 *  @param[in]  width       width of image
 *  @param[in]  height      height of image
 *  @retval     None
 **********************************************************************************************************************/
static void swap_32bit(uint8_t * const p_buffer, uint32_t width, uint32_t height)
{
        uint32_t * p_double_pixel = (uint32_t *)p_buffer;
        uint32_t double_pixel = RESET_VALUE;

        for(uint32_t y = 0; y < height; y ++)
        {
            for(uint32_t x = 0; x < (width / 2); x ++)
            {
                /* swap 32 bit */
                double_pixel = *p_double_pixel;
                *p_double_pixel = __builtin_bswap32(double_pixel);

                /* next double pixel */
                p_double_pixel ++;
            }
        }
}

/*******************************************************************************************************************//**
 *  @brief      swap byte positions in 16 bit data, equivalent to 1 pixel in image buffer.
 *  @param[in]  p_buffer    pointer to image buffer
 *  @param[in]  width       width of image
 *  @param[in]  height      height of image
 *  @retval     None
 **********************************************************************************************************************/
static void swap_16bit(uint8_t * const p_buffer, uint32_t width, uint32_t height)
{
        uint16_t * p_pixel = (uint16_t *)p_buffer;
        uint16_t pixel = RESET_VALUE;

        for(uint32_t y = 0; y < height; y ++)
        {
            for(uint32_t x = 0; x < width ; x ++)
            {
                /* swap 16 bit */
                pixel = *p_pixel;
                *p_pixel = __builtin_bswap16(pixel);

                /* next pixel */
                p_pixel ++;
            }
        }
}

/*******************************************************************************************************************//**
 * @} (end addtogroup pdc_ep)
 **********************************************************************************************************************/
