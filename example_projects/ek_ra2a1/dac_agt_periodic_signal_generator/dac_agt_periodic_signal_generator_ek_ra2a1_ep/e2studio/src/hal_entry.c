#include "common_utils.h"
#include <periodic_signal_generator_ep.h>

#define _USE_MATH_DEFINES
#include <math.h>

/* Array to store sine look up table */
static uint16_t look_up_table[SPP_MAX_VAL] = {0};

/* Variable to store user input for resolution, in samples per period
 * uint16_t to match g_transfer0_cfg.p_info->length */
static volatile uint16_t input_spp;

/* Variable to store user input for frequency, in Hz */
static volatile uint32_t input_freq;

/* Variable to store the DAC sample frequency. */
static volatile double sample_freq;

/* Variable to store maximum input frequency. */
static volatile uint32_t freq_max_val;

/* Variable to store the DAC's maximum sample rate in Hz, board specific */
static double dac_max_samp_rate;

/* Function to calculate LUT values */
static void generate_lut(void);

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{

    fsp_err_t err = FSP_SUCCESS;
    fsp_pack_version_t version = {RESET_VALUE};

    /*Version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example Project information printed on the Console */
    APP_PRINT(BANNER_INFO, EP_VERSION, version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    APP_PRINT(EP_INFO);

    /* Accounting for the 'Pin Configuration' limitation in the FSP Users' Manual for r_dac */
#ifdef BOARD_RA2A1_EK
    R_IOPORT_PinCfg(g_ioport.p_ctrl, BSP_IO_PORT_05_PIN_00, ((uint32_t)IOPORT_CFG_ANALOG_ENABLE | IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_CAC_AD));
#endif

    /* Initialize and start the DAC, AGT and DTC modules */
    err = R_DAC_Open (&g_dac0_ctrl, &g_dac0_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {   /* DAC module open failed */
        APP_ERR_PRINT("\r\n ** DAC Open API failed ** \r\n");
        APP_ERR_TRAP(err);
    }
    err = R_DAC_Start (&g_dac0_ctrl);
    /* Handle error */
    if(err != FSP_SUCCESS)
    {   /* DAC start failed */
        APP_ERR_PRINT("\r\n ** DAC Start API failed ** \r\n");
        APP_ERR_TRAP(err);
    }

    err = R_DTC_Open(&g_transfer0_ctrl, &g_transfer0_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {   /* DTC module open failed */
        APP_ERR_PRINT("\r\n ** DTC Open API failed ** \r\n");
        APP_ERR_TRAP(err);
    }
    err = R_DTC_Enable(&g_transfer0_ctrl);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {   /* DTC module enable failed */
       APP_ERR_PRINT("\r\n ** DTC Enable API failed ** \r\n");
       APP_ERR_TRAP(err);
    }

    err = R_AGT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {   /* AGT module open failed */
        APP_ERR_PRINT("\r\n ** AGT Open API failed ** \r\n");
        APP_ERR_TRAP(err);
    }
    err = R_AGT_Start(&g_timer0_ctrl);
    if (FSP_SUCCESS != err)
    {  /* AGT start failed */
       APP_ERR_PRINT("\r\n ** AGT Start API failed ** \r\n");
       APP_ERR_TRAP(err);
    }

    /* Calculate the DAC's maximum sample rate in Hz. This is calculated by the formula: 1/DAC_MAX_CONVERSION_PERIOD
     * DAC_MAX_CONVERSION_PERIOD for each MCU is specified in the Electrical Characteristics section of the RA User's Manual */
    dac_max_samp_rate = 1/(DAC_MAX_CONVERSION_PERIOD*pow(10,-6));

    while(1)
    {
        /* Prompt user to input desired signal's samples per period and check that it is within the valid range */
        APP_PRINT("\r\nEnter the desired samples per period for the output sine wave. Input Range: %d to %d\r\n", SPP_MIN_VAL, SPP_MAX_VAL);

        uint8_t is_not_valid = 1;

        while(is_not_valid)
        {
            if(APP_CHECK_DATA)
            {
                uint8_t readBuff[BUFFER_SIZE_DOWN] = { RESET_VALUE };
                APP_READ(readBuff);
                input_spp = (uint16_t) atoi ((char*) readBuff);

                if((input_spp >= SPP_MIN_VAL) && (input_spp <= SPP_MAX_VAL))
                {
                   is_not_valid = 0;
                }
                else
                {
                   APP_PRINT("\r\nValue out of bounds. Please enter desired samples per period within the range: %d to %d\r\n", SPP_MIN_VAL, SPP_MAX_VAL);
                }
            }

        }
        APP_PRINT("%d samples per period selected.\n", input_spp);

        /*Calculate the maximum possible frequency, determined by the relation: dac_max_samp_rate = freq_max_val/input_spp */
        freq_max_val = (uint32_t)(dac_max_samp_rate/ input_spp);

        /* Prompt user to input desired signal's frequency and check that it is within the valid range */
        APP_PRINT("\r\nEnter desired frequency for output sine wave, in Hz. Maximum input frequency: %d Hz\r\n", freq_max_val);
        is_not_valid = 1;

        while(is_not_valid)
        {
            if(APP_CHECK_DATA)
            {
                uint8_t readBuff[BUFFER_SIZE_DOWN] = { RESET_VALUE };
                APP_READ(readBuff);
                input_freq = (uint32_t) atoi ((char*) readBuff);

                if((input_freq > 0) && (input_freq <= freq_max_val))
                {
                    is_not_valid = 0;
                }
                else
                {
                    APP_PRINT("\r\nValue out of bounds. Please enter desired frequency below: %d Hz\r\n", freq_max_val);
                }
            }
        }
        APP_PRINT("%d Hz selected.\n", input_freq);

        /* Calculate the look up table */
        generate_lut();

        /* Reconfigure the DTC:
         *  destination: address of DADR0 register
         *  source: base address of the look up table
         *  length: length of the look up table
        */
        g_transfer0_cfg.p_info->p_dest = (void *) DADR0;
        g_transfer0_cfg.p_info->p_src = &look_up_table;
        g_transfer0_cfg.p_info->length = input_spp;

        err = R_DTC_Reconfigure(&g_transfer0_ctrl, g_transfer0_cfg.p_info);
        /* Handle error */
        if (FSP_SUCCESS != err)
        {   /* DTC module enable failed */
           APP_ERR_PRINT("\r\n ** DTC Reconfigure API failed ** \r\n");
           APP_ERR_TRAP(err);
        }

        /* Calculate the output sample frequency of the DAC for the specified wave. */
        sample_freq = input_spp*input_freq;

        /* Calculate and set the AGT counts per period, based on the relation between output sample frequency and the AGT clock source frequency */
        volatile uint32_t timer_freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB) >> g_timer0_cfg.source_div; //get PCLKB freq
        volatile uint32_t period_counts = (uint32_t) (timer_freq_hz/sample_freq) ;

        err = R_AGT_PeriodSet(&g_timer0_ctrl, period_counts);
        if (FSP_SUCCESS != err)
        {
           /* AGT period set failed */
           APP_ERR_PRINT("\r\n ** AGT PeriodSet API failed ** \r\n");
           APP_ERR_TRAP(err);
        }

        /* Provide feedback to JLinkRTT to indicate that the wave is being generated */
        APP_PRINT("\r\nOutputting %d Hz wave with %d samples per period.\r\n", input_freq, input_spp);

    }
#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif

}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open (&g_ioport_ctrl, g_ioport.p_cfg);
    }
}

#if BSP_TZ_SECURE_BUILD

BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
#endif

void generate_lut(void){
    /* Calculate look up table */
    for(int i=0; i< input_spp; ++i)
    {
        look_up_table[i] = (uint16_t) (DAC_MID_VAL*(cos(2*M_PI*i/input_spp) +1));
        /* Check the values of the LUT
        APP_PRINT("\r\n  %d", look_up_table[i]) */
    }
}
