/***********************************************************************************************************************
* 
* Copyright [2020] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
*
* This software is supplied by Renesas Electronics America Inc. and may only be used with products of Renesas Electronics Corp.
* and its affiliates (“Renesas”).  No other uses are authorized.  This software is protected under all applicable laws, 
* including copyright laws.
* Renesas reserves the right to change or discontinue this software.
* THE SOFTWARE IS DELIVERED TO YOU “AS IS,” AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT 
* PERMISSIBLE UNDER APPLICABLE LAW,DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES OF 
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE.  TO THE MAXIMUM 
* EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE (OR ANY PERSON 
* OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING, WITHOUT LIMITATION, 
* ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES;
* ANY LOST PROFITS, OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF 
* THE POSSIBILITY OF SUCH LOSS,DAMAGES, CLAIMS OR COSTS.
* 
**********************************************************************************************************************/

1. Project Overview:

   	This project demonstrates the basic functionality of an RA4 MCU operating as a BLE Server board establish connection with 
	Renesas browser installed on the device client. An RA MCU using freeRTOS and the Renesas FSP was used to create this example 
	project. This EP "find me profile" is configured to establish the connection with the Renesas GATTBrowser installed on the 
	client device. Led 1 on the server board will provide visual feedback to indicate once the client has established connection 
	or when a disconnection occurs. LED1 is turned on when the connection is established. It will turn off if the client is 
	disconnected. The user can send alert levels, connect requests, or disconnect requests from the Renesas GATTBrowser on the 
	client device. LED0 blink speed is used to indicate which user alert level is selected. If user chooses alert level 0, the 
	LED0 does not blink. If user chooses alert level 1, LED0 blinks slower compared with if use chooses alert level 2. The 
	connection status and LEDs status messages will display on Jlink RTT viewer.

  	
2. Hardware Requirement:

	i. 1x micro usb cable.
        ii. 1x board of EK-RA4W1.
        iii. 1x mobile device with Renesas GATTBrowser installed.

3. Hardware Connections:
 	Supported Board EK_RA4W1:		
	i. Connect RA board to Host machine using micro usb cable.
       	
4. Software requirements:
 	i.  Device client (iphone or ipad) has Renesas GATT browser installed.
	ii. Compiler: Renesas e2 Studio Version: 2023-04 with FSP v4.4.0
	iii. J-Link RTTviewer V7.60f


Note: After running EP "find me profile". User must use Renesas GATT brower to search and connect to Local Name "RA4W1_BLE" displayed on your
      device RA4W1 board server). Send the messages "Alert level" from your device GATTBrowser and the messages will be responded on RTTviewer.
      



