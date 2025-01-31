/***********************************************************************************************************************
 * File Name    : dhcpv4_server_ep.h
 * Description  : Contains data structures and functions used in dhcpv4_server_ep.
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

#ifndef DHCPV4_SERVER_EP_H_
#define DHCPV4_SERVER_EP_H_

/* Index corresponding to network interface */
#define INTERFACE_INDEX        (0U)
/* IP address range assigned by the DHCP server */
#define START_IP_ADDRESS_LIST (IP_ADDRESS(192, 168, 0, 10))
#define END_IP_ADDRESS_LIST   (IP_ADDRESS(192, 168, 0, 20))
/* Network Configuration */
#define NX_DHCP_SERVER_IP_ADDRESS                  (IP_ADDRESS(192,168,0,2))
#define NX_DHCP_ROUTER_IP_ADDRESS                  (IP_ADDRESS(192,168,0,2))
#define NX_DHCP_DNS_IP_ADDRESS                     (IP_ADDRESS(192,168,0,2))
#define LINK_ENABLE_WAIT_TIME (1000U)
#define NULL_CHAR             ('\0')

/* RTT input selection options to start, stop and print client info. */
#define DHCPV4_SERVER_START               (1U)
#define DHCPV4_SERVER_STOP                (2U)
#define PRINT_DHCPV4_CLIENT_INFO          (3U)

#define DHCPV4_SERVER_MENU               "\r\nDHCPV4 SERVER Menu options :"\
                                         "\r\n1. Enter 1 to start dhcpv4 server"\
                                         "\r\n2. Enter 2 to stop dhcpv4 server"\
                                         "\r\n3. Enter 3 to display client info"\
                                         "\r\nUser Input : \r\n"\

/* macros to print info, error and trap the error.*/
#define PRINT_INFO_STR(str)  (app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_INFO_STR, sizeof(str), (str)))
#define PRINT_ERR_STR(str)   (app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_STR, sizeof(str), (str)))
#define ERROR_TRAP(err)      (app_rtt_print_data(RTT_OUTPUT_MESSAGE_APP_ERR_TRAP, sizeof(UINT *),&(err)))

#define EP_INFO        "\r\nThis project demonstrates the basic functionality of Netx Duo DHCPv4 server\r\n"\
                       "with ethernet driver on Renesas RA MCUs based on Renesas FSP using AzureRTOS.\r\n"\
                       "DHCPV4 Server is created and based on user input, user can start, stop and print \r\n"\
                       "DHCPV4 clients info. On successful completion of each operation, the success status\r\n"\
                       "is displayed on RTT viewer. Error and info messages will be printed on JlinkRTTViewer.\r\n\n\n"


#endif /* DHCPV4_SERVER_EP_H_ */
