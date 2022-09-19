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

#include "common_utils.h"
#include "acmplp_ep.h"
#include "dac_operation.h"

/*******************************************************************************************************************//**
 * @addtogroup acmplp_ep
 * @{
 **********************************************************************************************************************/

void R_BSP_WarmStart(bsp_warm_start_event_t event);

/*******************************************************************************************************************//**
 * The RA Configuration tool generates main() and uses it to generate threads if an RTOS is used.  This function is
 * called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    unsigned char rByte[BUF_SIZE]     = {INITIAL_VALUE};
    uint32_t      read_data           =  RESET_VALUE;
    fsp_err_t     error = FSP_SUCCESS;

    fsp_pack_version_t version = {RESET_VALUE};

    /* version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Project information */
    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2);
    APP_PRINT(BANNER_3,EP_VERSION);
    APP_PRINT(BANNER_4,version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch );
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6);
    APP_PRINT(EP_INFO);

    /* initialize acmplp driver */
    error = init_acmplp();
    if(FSP_SUCCESS != error)
    {
        APP_ERR_PRINT("\r\n ** ACMPLP INITIALIZATION FAILED ** \r\n");
        APP_ERR_TRAP(error);
    }

    /* Need to configure DAC pin manually as DAC output pin generated by ISDE configuration doesn't configure it properly
     * which may result in DAC pin giving zero output all the time */
#if defined (BOARD_RA2A1_EK)
    R_IOPORT_PinCfg(g_ioport.p_ctrl,BSP_IO_PORT_05_PIN_00, IOPORT_CFG_ANALOG_ENABLE | IOPORT_CFG_PERIPHERAL_PIN
            | IOPORT_PERIPHERAL_CAC_AD);
#endif

    /* initialize dac driver */
    error = init_dac();
    if(FSP_SUCCESS != error)
    {
        APP_ERR_PRINT("\r\n **  DAC INITIALIZATION FAILED ** \r\n");
        /* de-initialize opened ACMPLP module since module is in open state now */
        deinit_acmplp();
        APP_ERR_TRAP(error);
    }

    APP_PRINT ("\r\n Menu Options");
    APP_PRINT ("\r\n 1. Enter 1 for ACMPLP Normal Mode");
    APP_PRINT ("\r\n 2. Enter 2 for Exit\r\n");
    APP_PRINT (" User Input:  ");

    while (EXIT != read_data)
    {
        if (APP_CHECK_DATA)
        {
            APP_READ(rByte);

            /* Conversion from input string to integer value */
            read_data = (uint32_t) (atoi((char *)rByte));

            switch (read_data)
            {
                case NORMAL_MODE:
                {
                    error = acmplp_operation();
                    if(FSP_SUCCESS != error)
                    {
                        APP_ERR_PRINT("\r\n **  ACMPLP ENABLE NORMAL MODE FAILED ** \r\n");
                        /* de-initialize opened ACMPLP module since module is in open state now */
                        deinit_acmplp();
                        /* de-initialize opened DAC module since module is in open state now */
                        deinit_dac();
                        APP_ERR_TRAP(error);
                    }
                    break;
                }

                case EXIT:
                {
                    /* de-initialize opened ACMPLP module since module is in open state now */
                    deinit_acmplp();
                    /* de-initialize opened DAC module since module is in open state now */
                    deinit_dac();
                    APP_PRINT (" \r\nExited, Restart the Application\r\n");
                    break;
                }

                default:
                {
                    APP_PRINT (" \r\nInvalid Menu Option Selected\r\n");
                    break;
                }
            }
            if(EXIT != read_data)
            {
                APP_PRINT ("\r\n Menu Options");
                APP_PRINT ("\r\n 1. Enter 1 for ACMPLP Normal Mode");
                APP_PRINT ("\r\n 2. Enter 2 for Exit\r\n");
                APP_PRINT (" User Input:  ");
            }
            /* Reset buffer */
            memset (rByte, INITIAL_VALUE, BUF_SIZE);
        }
        else
        {
            /* do nothing */
        }
    }
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open (&g_ioport_ctrl, &g_bsp_pin_cfg);
    }
}

/*******************************************************************************************************************//**
 * @} (end addtogroup acmplp_ep)
 **********************************************************************************************************************/
