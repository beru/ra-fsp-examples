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
 
    This example project demonstrates the typical use of the I3C Driver on Renesas RA2E2 MCUs based on Renesas FSP. 
    The I3C master on EK-RA2E2 demonstrates operations associated with an I3C slave running on another RA2E2 board. 
    Once initialization is successful, I3C Master device assigns slave address to I3C slave device through 
    DAA (dynamic address assignment) using I3C common command code (CCC). Once Dynamic Address Assignment is completed, 
    the Master EP performs write/read operations, and displays slave device information based on user input. 
    If the on-board switch is pressed on the slave side, it will initiate an IBI transfer request. 
    Error and info messages will be printed on Jlink RTTViewer.

2. Hardware Requirement:

	i. 2x Renesas EK-RA2E2 kit.
       ii. 2x USB A to USB Micro B Cable. 
      iii. 5x Jumper wires to link RA2E2 boards.
       iv. 1x Pull up resistor of value 3.9 or 4.7k ohms.
      
3. Hardware Connections:
 	Supported Board EK-RA2E2:		
	* Connect micro-USB cable between J10 on I3C master board EK-RA2E2 and PC.
        * Connect micro-USB cable between J10 on I3C slave board EK-RA2E2 and PC.
        * External 3V3 pull-up using 3.9 or 4.7k ohms resistor should be connected on SDA line.
        * Three jumper wires are required to establish loop back connection along I3C lines across the board with pins as mentioned below.
        
        Master Board              Slave Board
        -------------             ------------
	SDA0 Pin P401   ---->     SDA0 Pin 401
        SCL0 Pin P400   ---->     SCL0 Pin 400
        Common GND

Note: 
1. On slave side user can perform IBI transfer request using onboard Push button(S2).
2. readme is common for both i3c_master and i3c_slave EP.