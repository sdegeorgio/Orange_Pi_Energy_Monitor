/*
 * EM100 - Energy Monitor for Volts / Amps / Power displayed on LCD, with USB and Ethernet interfaces.
 * Copyright (C) 2016-2017 Stephan de Georgio
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/* 
 * File:   telnet.h
 * Author: Stephan de Georgio
 *
 * Created on 14 July 2020, 21:09
 * 
 * This file contains a ver incomplete set of Telnet command and option codes
 */


#ifndef TELNET_H
#define TELNET_H

/* Telnet command codes */
/* EOF                 236 */
#define TELNET_EOF 236
/* SE                  240    End of subnegotiation parameters.*/
#define TELNET_SE 240
/* NOP                 241    No operation.
 * */
#define TELNET_NOP 241
/*Data Mark           242    The data stream portion of a Synch.
                           This should always be accompanied
                           by a TCP Urgent notification.
*/
#define TELNET_DM 242
/*Break               243    NVT character BRK.
 */
#define TELNET_BRK 243
/* Interrupt Process   244    The function IP.
 */
#define TELNET_IP 244
/*Abort output        245    The function AO.
*/
#define TELNET_AO 245
/*Are You There       246    The function AYT.
*/
#define TELNET_AYT 246
/*Erase character     247    The function EC.
*/
#define TELNET_EC 247
/*Erase Line          248    The function EL.
*/
#define TELNET_EL 248
/*Go ahead            249    The GA signal.
*/
#define TELNET_GA 249
/*SB                  250    Indicates that what follows is
                           subnegotiation of the indicated
                           option.
*/
#define TELNET_SB 250
/*WILL (option code)  251    Indicates the desire to begin
                           performing, or confirmation that
                           you are now performing, the
                           indicated option.
*/
#define TELNET_WILL 251
/*WON'T (option code) 252    Indicates the refusal to perform,
                           or continue performing, the
                           indicated option.
*/
#define TELNET_WONT 252
/*DO (option code)    253    Indicates the request that the
                           other party perform, or
                           confirmation that you are expecting
                           the other party to perform, the
                           indicated option.
*/
#define TELNET_DO 253
/*DON'T (option code) 254    Indicates the demand that the
                           other party stop performing,
                           or confirmation that you are no
                           longer expecting the other party
                           to perform, the indicated option.
*/
#define TELNET_DONT 254
/*IAC                 255    Data Byte 255.
 */
#define TELNET_IAC 255


/* Telnet option codes */
/*0  0         Binary Xmit      Allows transmission of binary data. */
#define TELNET_BINARY_XMIT 0
/*    1  1         Echo Data        Causes server to echo back
                                  all keystrokes.*/
#define TELNET_ECHO_DATA 1
/*    2  2         Reconnect        Reconnects to another TELNET host. */
#define TELNET_RECONNECT 2
/*    3  3         Suppress GA      Disables Go Ahead! command. */
#define TELNET_SUPPRESS_GA 3
/*    4  4         Message Sz       Conveys approximate message size. */
#define TELNET_MESSAGE_SIZE 4
/*    5  5         Opt Status       Lists status of options. */
#define TELNET_OPT_STATUS 5
/*    6  6         Timing Mark      Marks a data stream position for
                                    reference. */
#define TELNET_TIMING_MARK 6
/*    7  7         R/C XmtEcho      Allows remote control of terminal
                                    printers. */
#define TELNET_RC_XMTECHO 7
/*    8  8         Line Width       Sets output line width. */
#define TELNET_LINE_WIDTH 8
/*    9  9         Page Length      Sets page length in lines. */
#define TELNET_PAGE_LENGTH 9
/*   10  A        CR Use            Determines handling of carriage returns. */
#define TELNET_CR_USE 10
/*   11  B        Horiz Tabs        Sets horizontal tabs. */
#define TELNET_HORIZ_TABS 11
/*   12  C        Hor Tab Use       Determines handling of horizontal tabs. */
#define TELNET_HOR_TAB_USE 12
/*   13  D        FF Use            Determines handling of form feeds. */
#define TELNET_FF_USE 13
/*   14  E        Vert Tabs         Sets vertical tabs. */
#define TELNET_VERT_TABS 14
/*   15  F        Ver Tab Use       Determines handling of vertical tabs. */
#define TELNET_VER_TAB_USE 15
/*   16 10       Lf Use             Determines handling of line feeds. */
#define TELNET_LF_USE 16
/*   17 11       Ext ASCII          Defines extended ASCII characters. */
#define TELNET_EXT_ASCII 17
/*   18 12       Logout             Allows for forced log-off. */
#define TELNET_LOGOUT 18
/*   19 13       Byte Macro         Defines byte macros. */
#define TELNET_BYTE_MACRO 19
/*   20 14       Data Term          Allows subcommands for Data Entry to
                                  be sent. */
#define TELNET_DATA_TERM 20
/*   21 15       SUPDUP             Allows use of SUPDUP display protocol. */
#define TELNET_SUPDUP 21
/*   22 16       SUPDUP Outp        Allows sending of SUPDUP output. */
#define TELNET_SUPDUP_OUTPUT 22
/*   23 17       Send Locate        Allows terminal location to be sent. */
#define TELNET_SEND_LOCATE 23
/*   24 18       Term Type          Allows exchange of terminal type
                                  information. */
#define TELNET_TERM_TYPE 24
/*   25 19       End Record         Allows use of the End of record code
                                  (0xEF). */
#define TELNET_END_RECORD 25
/*   26 1A       TACACS ID          User ID exchange used to avoid more
                                  than 1 log-in. */
#define TELNET_TACACS_ID 26
/*   27 1B       Output Mark        Allows banner markings to be sent on
                                  output. */
#define TELNET_OUTPUT_MARK 27
/*   28 1C       Term Loc#          A numeric ID used to identify terminals. */
#define TELNET_TERM_LOC 28
/*   29 1D       3270 Regime        Allows emulation of 3270 family
                                  terminals. */
#define TELNET_REGIME 29
/*   30 1E       X.3 PAD            Allows use of X.3 protocol emulation. */
#define TELNET_X3_PAD 30
/*   31 1F       Window Size        Conveys window size for emulation
                                  screen. */
#define TELNET_WINDOW_SIZE 31
/*   32 20       Term Speed         Conveys baud rate information. */
#define TELNET_TERM_SPEED 32
/*   33 21       Remote Flow        Provides flow control (XON, XOFF). */
#define TELNET_REMOTE_FLOW 33
/*   34 22       Linemode           Provides linemode bulk character
                                  transactions. */
#define TELNET_LINEMODE 34
/*   255 FF      Extended           options list  Extended options list. */
#define TELNET_EXTENDED 255

#endif /* TELNET_H */

