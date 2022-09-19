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
#include "agt_ep.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

static fsp_err_t timer_status_check (void);

volatile uint8_t g_one_shot_timer_flag = RESET_FLAG;    //flag to check timer is enable or not
volatile uint8_t g_periodic_timer_flag = RESET_FLAG;    //flag to check timer1 is enable or not
volatile uint32_t g_error_flag = RESET_FLAG;            //flag to capture error in ISR's
volatile uint32_t g_status_check_flag = RESET_FLAG;     //flag to capture the status of timers

timer_callback_args_t agt0_callback_args;
timer_callback_args_t agt1_callback_args;

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void) {
    fsp_err_t err = FSP_SUCCESS;
    fsp_pack_version_t version = {RESET_VALUE};
    unsigned char rByte[BUFFER_SIZE_DOWN] = {RESET_VALUE};
    uint32_t time_period_ms_oneshot = RESET_VALUE;
    uint32_t time_period_ms_periodic = RESET_VALUE;
    uint32_t raw_counts_oneshot = RESET_VALUE;
    uint32_t raw_counts_periodic = RESET_VALUE;
    timer_info_t  one_shot_info =
    {
     .clock_frequency = RESET_VALUE,
     .count_direction = (timer_direction_t) RESET_VALUE,
     .period_counts = RESET_VALUE,
    };
    timer_info_t periodic_info =
    {
     .clock_frequency = RESET_VALUE,
     .count_direction = (timer_direction_t) RESET_VALUE,
     .period_counts = RESET_VALUE,
    };
    uint32_t clock_freq = RESET_VALUE;

    /* version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example Project information printed on the Console */
    APP_PRINT(BANNER_INFO,EP_VERSION,version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch );
    APP_PRINT("\r\nThis Example Project demonstrates the functionality of AGT in periodic mode and one-shot mode. "\
            "\r\nOn providing any input on the RTTviewer, AGT channel 0 starts in one-shot mode. AGT channel 1 "\
            "\r\nstarts in periodic mode when AGT channel 0 expires. Timer in periodic mode expires periodically"\
            "\r\nat a time period specified by user and toggles the on-board LED.\r\n");

    /* Initialize AGT driver */
    err = agt_init();
    /* Handle error */
    if (FSP_SUCCESS != err)
    {   /* AGT module init failed */
        APP_ERR_PRINT("\r\n ** AGT INIT FAILED ** \r\n");
        APP_ERR_TRAP(err);
    }

    err = g_timer_one_shot_callback_set_guard(NULL, one_shot_timer_callback, NULL, &agt0_callback_args);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        agt_deinit();
        APP_ERR_PRINT("\r\ng_timer_one_shot_callback_set_guard API failed.Closing all drivers. Restart the Application\r\n");
        APP_ERR_TRAP(err);
    }
    err = g_timer_periodic_callback_set_guard(NULL, periodic_timer_callback, NULL, &agt1_callback_args);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        agt_deinit();
        APP_ERR_PRINT("\r\ng_timer_periodic_callback_set_guard API failed.Closing all drivers. Restart the Application\r\n");
        APP_ERR_TRAP(err);
    }

    APP_PRINT("\r\nPlease enter time period values for one-shot and periodic mode timers in milliseconds\n"
            " Valid range: 1 to 2000\r\n");
    APP_PRINT("\r\nOne-shot mode:");

    while(true)
    {
        if (APP_CHECK_DATA)
        {
            /* Cleaning buffer */
            memset(&rByte[0], NULL_CHAR, BUFFER_SIZE_DOWN);
            APP_READ(rByte);
            time_period_ms_oneshot = (uint32_t)atoi((char *)rByte);
            if ((TIME_PERIOD_MAX >= time_period_ms_oneshot) && (TIME_PERIOD_MIN < time_period_ms_oneshot))
            {
                APP_PRINT("\r\nTime period for one-shot mode timer: %d\r\n",time_period_ms_oneshot);
                break;
            }
            else
            {
                APP_PRINT("\r\nInvalid input. Please enter valid input\r\n");
            }
        }
    }

    APP_PRINT("\r\nPeriodic mode:");

    while(true)
    {
        if (APP_CHECK_DATA)
        {
            /* Cleaning buffer */
            memset(&rByte[0], NULL_CHAR, BUFFER_SIZE_DOWN);
            APP_READ(rByte);
            time_period_ms_periodic = (uint32_t)atoi((char *)rByte);
            if ((TIME_PERIOD_MAX >= time_period_ms_periodic) && (TIME_PERIOD_MIN < time_period_ms_periodic))
            {
                APP_PRINT("\r\nTime period for periodic mode timer: %d\r\n",time_period_ms_periodic);
                break;
            }
            else
            {
                APP_PRINT("\r\nInvalid input. Please enter valid input\r\n");
            }
        }
    }

    /* Calculation of raw counts value for given milliseconds value */
    err = g_timer_one_shot_info_get_guard(NULL, &one_shot_info);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        agt_deinit();
        APP_ERR_PRINT("\r\nInfoGet API failed.Closing all drivers. Restart the Application\r\n");
        APP_ERR_TRAP(err);
    }

    /* Depending on the user selected clock source, raw counts value can be calculated
     * for the user given time-period values */
    clock_freq = one_shot_info.clock_frequency;
    raw_counts_oneshot = (uint32_t)((time_period_ms_oneshot * clock_freq ) / 1000);
    /* Set period value */
    err = g_timer_one_shot_period_set_guard(NULL, raw_counts_oneshot);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        agt_deinit();
        APP_ERR_PRINT("\r\nPeriodSet API failed.Closing all drivers. Restart the Application\r\n");
        APP_ERR_TRAP(err);
    }

    /* Calculation of raw counts value for given milliseconds value */
    err = g_timer_periodic_info_get_guard(NULL, &periodic_info);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        agt_deinit();
        APP_ERR_PRINT("\r\nInfoGet API failed.Closing all drivers. Restart the Application\r\n");
        APP_ERR_TRAP(err);
    }
    /* Depending on the user selected clock source, raw counts value can be calculated
     * for the user given time-period values */
    clock_freq = periodic_info.clock_frequency;
    raw_counts_periodic = (uint32_t)((time_period_ms_periodic * clock_freq ) / 1000);
    /* Set period value */
    err = g_timer_periodic_period_set_guard(NULL, raw_counts_periodic);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        agt_deinit();
        APP_ERR_PRINT("\r\nPeriodSet API failed. Closing all drivers.Restart the Application\r\n");
        APP_ERR_TRAP(err);
    }

    APP_PRINT("\r\nEnter any key to start or stop the timers");

    while(true)
    {
        if (APP_CHECK_DATA)
        {
            /* Cleaning buffer */
            memset (&rByte[0], NULL_CHAR, BUFFER_SIZE_DOWN);
            APP_READ(rByte);
            g_status_check_flag = SET_FLAG;
        }

        if (SET_FLAG == g_status_check_flag)
        {
            /* Check the status of timers and perform operation accordingly */
            g_status_check_flag = RESET_FLAG;
            err = timer_status_check();
            /* Handle error */
            if (FSP_SUCCESS != err)
            {
                agt_deinit();
                APP_ERR_PRINT("\r\nTimer start/stop failed");
                APP_ERR_TRAP(err);
            }
        }

        /* Check if AGT0 is enabled in OneShot mode */
        if (SET_FLAG == g_one_shot_timer_flag)
        {
            g_one_shot_timer_flag = RESET_FLAG;
            APP_PRINT("\r\nAGT0 is Enabled in OneShot mode");
        }

        /* Check if AGT1 is enabled in Periodic mode */
        if (SET_FLAG == g_periodic_timer_flag)
        {
            g_periodic_timer_flag = RESET_FLAG;
            APP_PRINT ("\r\n\r\nOne-shot mode AGT timer elapsed");
            APP_PRINT ("\r\n\r\nAGT1 is Enabled in Periodic mode");
            APP_PRINT ("\r\nLED will toggle for set time period");
            APP_PRINT ("\r\nEnter any key to stop timers\r\n");
        }

        /* Check if AGT1 is already running in Periodic mode */
        if (ALREADY_RUNNING == g_periodic_timer_flag)
        {
            g_periodic_timer_flag = RESET_FLAG;
            APP_PRINT ("\r\n\r\nOne-shot mode AGT timer elapsed\n");
            APP_PRINT ("\r\n\r\nAGT1 is already running in Periodic mode");
            APP_PRINT ("\r\nEnter any key to stop the timer\r\n");
        }
    }

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * @brief This function checks the status of both timers and starts/stops the timers.
 *
 * @param[in] None
 **********************************************************************************************************************/
static fsp_err_t timer_status_check (void)
{
    fsp_err_t err = FSP_SUCCESS;
    timer_status_t periodic_timer_status =
    {
     .counter = RESET_VALUE,
     .state = (timer_state_t) RESET_VALUE,
    };
    timer_status_t oneshot_timer_status =
    {
     .counter = RESET_VALUE,
     .state = (timer_state_t) RESET_VALUE,
    };

    /* Retrieve the status of timer running in periodic mode */
    err = g_timer_periodic_status_get_guard(NULL, &periodic_timer_status);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nStatusGet API failed");
        return err;
    }


    /* Retrieve the status of timer running in one-shot mode */
    err = g_timer_one_shot_status_get_guard(NULL, &oneshot_timer_status);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nStatusGet API failed");
        return err;
    }

    if (TIMER_STATE_STOPPED != oneshot_timer_status.state)
    {
        err = g_timer_one_shot_stop_guard(NULL);
        if (FSP_SUCCESS != err)
        {
            APP_PRINT("\r\nAGT Stop API failed");
            return err;
        }
        else
        {
            APP_PRINT("\r\nOne-shot timer stopped. "
                    "Enter any key to start timers.\r\n");
        }
    }
    else if (TIMER_STATE_STOPPED != periodic_timer_status.state)
    {
        err = g_timer_periodic_stop_guard(NULL);
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\nAGT Stop API failed");
            return err;
        }
        else
        {
            APP_PRINT("\r\nPeriodic timer stopped. "
                    "Enter any key to start timers.\r\n");
        }
    }
    else
    {
        err = agt_start_oneshot_timer();
        /* Handle error */
        if (FSP_SUCCESS != err)
        {
            APP_ERR_PRINT("\r\n AGT start failed");
            return err;
        }
        else
        {
            g_one_shot_timer_flag = SET_FLAG;        // Set Timer Flag as timer is started
        }
    }
    return err;
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
