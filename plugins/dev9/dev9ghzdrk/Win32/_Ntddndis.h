/*++ BUILD Version: 0001                // Increment this if a change has global effects
   Copyright (c) 1990-1993  Microsoft Corporation
   Module Name:
   ntddndis.h
   Abstract:
   This is the include file that defines all constants and types for
   accessing the Network driver interface device.
   Author:
   Steve Wood (stevewo) 27-May-1990
   Revision History:
   Adam Barr (adamba)           04-Nov-1992             added the correct values for NDIS 3.0.
   Jameel Hyder (jameelh)       01-Aug-95               added Pnp IoCTLs and structures
   Kyle Brandon (kyleb) 09/24/96                added general co ndis oids.
   -- */
#ifndef _NTDDNDIS_
#define _NTDDNDIS_
//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//                with the Ascii representation of the unit number.
//
#define DD_NDIS_DEVICE_NAME "\\Device\\UNKNOWN"
//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//                       buffers are passed to the driver!
//
#define _NDIS_CONTROL_CODE(request,method) \
			CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)
#define IOCTL_NDIS_QUERY_GLOBAL_STATS	_NDIS_CONTROL_CODE( 0, METHOD_OUT_DIRECT )
#define IOCTL_NDIS_QUERY_ALL_STATS		_NDIS_CONTROL_CODE( 1, METHOD_OUT_DIRECT )
#define IOCTL_NDIS_ADD_DEVICE			_NDIS_CONTROL_CODE( 2, METHOD_BUFFERED )
#define IOCTL_NDIS_DELETE_DEVICE		_NDIS_CONTROL_CODE( 3, METHOD_BUFFERED )
#define IOCTL_NDIS_TRANSLATE_NAME		_NDIS_CONTROL_CODE( 4, METHOD_BUFFERED )
#define IOCTL_NDIS_ADD_TDI_DEVICE		_NDIS_CONTROL_CODE( 5, METHOD_BUFFERED )
#define IOCTL_NDIS_NOTIFY_PROTOCOL		_NDIS_CONTROL_CODE( 6, METHOD_BUFFERED )
#define	IOCTL_NDIS_GET_LOG_DATA			_NDIS_CONTROL_CODE( 7, METHOD_OUT_DIRECT )
//
// NtDeviceIoControlFile InputBuffer/OutputBuffer record structures for
// this device.
//
//
// This is the type of an NDIS OID value.
//
typedef ULONG NDIS_OID, *PNDIS_OID;
//
// IOCTL_NDIS_QUERY_ALL_STATS returns a sequence of these, packed
// together (no padding is required since statistics all have
// four or eight bytes of data).
//
typedef struct _NDIS_STATISTICS_VALUE {
    NDIS_OID Oid;
    ULONG DataLength;
    UCHAR Data[1];		// variable length

} NDIS_STATISTICS_VALUE, *PNDIS_STATISTICS_VALUE;

//
// Structure used by TRANSLATE_NAME IOCTL
//
typedef struct _NET_PNP_ID {
    ULONG ClassId;
    ULONG Token;
} NET_PNP_ID, *PNET_PNP_ID;

typedef struct _NET_PNP_TRANSLATE_LIST {
    ULONG BytesNeeded;
    NET_PNP_ID IdArray[ANYSIZE_ARRAY];
} NET_PNP_TRANSLATE_LIST, *PNET_PNP_TRANSLATE_LIST;

//
// Structure used to define a self-contained variable data structure
//
typedef struct _NDIS_VAR_DATA_DESC {
    USHORT Length;		// # of octects of data

    USHORT MaximumLength;	// # of octects available

    LONG Offset;		// Offset of data relative to the descriptor

} NDIS_VAR_DATA_DESC, *PNDIS_VAR_DATA_DESC;

//
// Object Identifiers used by NdisRequest Query/Set Information
//
//
// General Objects
//
#define OID_GEN_SUPPORTED_LIST			  		0x00010101
#define OID_GEN_HARDWARE_STATUS			 		0x00010102
#define OID_GEN_MEDIA_SUPPORTED			 		0x00010103
#define OID_GEN_MEDIA_IN_USE					0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD		   		0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE		  		0x00010106
#define OID_GEN_LINK_SPEED				  		0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE	   		0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE			0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE		 		0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE		  		0x0001010B
#define OID_GEN_VENDOR_ID				   		0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION		  		0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER	   		0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD		   		0x0001010F
#define OID_GEN_DRIVER_VERSION			  		0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE		  		0x00010111
#define OID_GEN_PROTOCOL_OPTIONS				0x00010112
#define OID_GEN_MAC_OPTIONS				 		0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS			0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS			0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION			0x00010116
#define OID_GEN_XMIT_OK					 		0x00020101
#define OID_GEN_RCV_OK					  		0x00020102
#define OID_GEN_XMIT_ERROR				  		0x00020103
#define OID_GEN_RCV_ERROR				   		0x00020104
#define OID_GEN_RCV_NO_BUFFER			   		0x00020105
#define OID_GEN_DIRECTED_BYTES_XMIT		 		0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT			0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT			0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT	   		0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT			0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT	   		0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV		  		0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV		 		0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV		 		0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV			0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV		 		0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV			0x0002020C
#define OID_GEN_RCV_CRC_ERROR			   		0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH	   		0x0002020E
#define OID_GEN_GET_TIME_CAPS					0x0002020F
#define OID_GEN_GET_NETCARD_TIME				0x00020210
//
//      These are connection-oriented general OIDs.
//      These replace the above OIDs for connection-oriented media.
//
#define OID_GEN_CO_SUPPORTED_LIST			  	0x00010101
#define OID_GEN_CO_HARDWARE_STATUS			 	0x00010102
#define OID_GEN_CO_MEDIA_SUPPORTED			 	0x00010103
#define OID_GEN_CO_MEDIA_IN_USE					0x00010104
#define OID_GEN_CO_LINK_SPEED				  	0x00010105
#define OID_GEN_CO_VENDOR_ID				   	0x00010106
#define OID_GEN_CO_VENDOR_DESCRIPTION		  	0x00010107
#define OID_GEN_CO_DRIVER_VERSION			  	0x00010108
#define OID_GEN_CO_PROTOCOL_OPTIONS				0x00010109
#define OID_GEN_CO_MAC_OPTIONS				 	0x0001010A
#define OID_GEN_CO_MEDIA_CONNECT_STATUS			0x0001010B
#define OID_GEN_CO_VENDOR_DRIVER_VERSION		0x0001010C
#define OID_GEN_CO_MINIMUM_LINK_SPEED			0x0001010D
#define OID_GEN_CO_GET_TIME_CAPS				0x00010201
#define OID_GEN_CO_GET_NETCARD_TIME				0x00010202
//
//      These are connection-oriented statistics OIDs.
//
#define	OID_GEN_CO_XMIT_PDUS_OK					0x00020101
#define	OID_GEN_CO_RCV_PDUS_OK					0x00020102
#define	OID_GEN_CO_XMIT_PDUS_ERROR				0x00020103
#define	OID_GEN_CO_RCV_PDUS_ERROR				0x00020104
#define	OID_GEN_CO_RCV_PDUS_NO_BUFFER			0x00020105
#define	OID_GEN_CO_RCV_CRC_ERROR				0x00020201
#define OID_GEN_CO_TRANSMIT_QUEUE_LENGTH		0x00020202
#define	OID_GEN_CO_BYTES_XMIT					0x00020203
#define OID_GEN_CO_BYTES_RCV					0x00020204
#define	OID_GEN_CO_BYTES_XMIT_OUTSTANDING		0x00020205
#define	OID_GEN_CO_NETCARD_LOAD					0x00020206
//
// These are objects for Connection-oriented media call-managers and are not
// valid for ndis drivers. Under construction.
//
#define OID_CO_ADD_PVC							0xFF000001
#define OID_CO_DELETE_PVC						0xFF000002
#define OID_CO_GET_CALL_INFORMATION				0xFF000003
#define OID_CO_ADD_ADDRESS						0xFF000004
#define OID_CO_DELETE_ADDRESS					0xFF000005
#define OID_CO_GET_ADDRESSES					0xFF000006
#define OID_CO_ADDRESS_CHANGE					0xFF000007
#define OID_CO_SIGNALING_ENABLED				0xFF000008
#define OID_CO_SIGNALING_DISABLED				0xFF000009
//
// 802.3 Objects (Ethernet)
//
#define OID_802_3_PERMANENT_ADDRESS		 		0x01010101
#define OID_802_3_CURRENT_ADDRESS		   		0x01010102
#define OID_802_3_MULTICAST_LIST				0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE		 		0x01010104
#define OID_802_3_MAC_OPTIONS				 	0x01010105
//
//
#define	NDIS_802_3_MAC_OPTION_PRIORITY			0x00000001
#define OID_802_3_RCV_ERROR_ALIGNMENT	   		0x01020101
#define OID_802_3_XMIT_ONE_COLLISION			0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS	  		0x01020103
#define OID_802_3_XMIT_DEFERRED			 		0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS	   		0x01020202
#define OID_802_3_RCV_OVERRUN			   		0x01020203
#define OID_802_3_XMIT_UNDERRUN			 		0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE		0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST	   		0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS	  		0x01020207
//
// 802.5 Objects (Token-Ring)
//
#define OID_802_5_PERMANENT_ADDRESS		 		0x02010101
#define OID_802_5_CURRENT_ADDRESS		   		0x02010102
#define OID_802_5_CURRENT_FUNCTIONAL			0x02010103
#define OID_802_5_CURRENT_GROUP			 		0x02010104
#define OID_802_5_LAST_OPEN_STATUS		  		0x02010105
#define OID_802_5_CURRENT_RING_STATUS	   		0x02010106
#define OID_802_5_CURRENT_RING_STATE			0x02010107
#define OID_802_5_LINE_ERRORS			   		0x02020101
#define OID_802_5_LOST_FRAMES			   		0x02020102
#define OID_802_5_BURST_ERRORS			  		0x02020201
#define OID_802_5_AC_ERRORS				 		0x02020202
#define OID_802_5_ABORT_DELIMETERS		  		0x02020203
#define OID_802_5_FRAME_COPIED_ERRORS	   		0x02020204
#define OID_802_5_FREQUENCY_ERRORS		  		0x02020205
#define OID_802_5_TOKEN_ERRORS			  		0x02020206
#define OID_802_5_INTERNAL_ERRORS		   		0x02020207
//
// FDDI Objects
//
#define OID_FDDI_LONG_PERMANENT_ADDR			0x03010101
#define OID_FDDI_LONG_CURRENT_ADDR		  		0x03010102
#define OID_FDDI_LONG_MULTICAST_LIST			0x03010103
#define OID_FDDI_LONG_MAX_LIST_SIZE		 		0x03010104
#define OID_FDDI_SHORT_PERMANENT_ADDR	   		0x03010105
#define OID_FDDI_SHORT_CURRENT_ADDR		 		0x03010106
#define OID_FDDI_SHORT_MULTICAST_LIST	   		0x03010107
#define OID_FDDI_SHORT_MAX_LIST_SIZE			0x03010108
#define OID_FDDI_ATTACHMENT_TYPE				0x03020101
#define OID_FDDI_UPSTREAM_NODE_LONG		 		0x03020102
#define OID_FDDI_DOWNSTREAM_NODE_LONG	   		0x03020103
#define OID_FDDI_FRAME_ERRORS			   		0x03020104
#define OID_FDDI_FRAMES_LOST					0x03020105
#define OID_FDDI_RING_MGT_STATE			 		0x03020106
#define OID_FDDI_LCT_FAILURES			   		0x03020107
#define OID_FDDI_LEM_REJECTS					0x03020108
#define OID_FDDI_LCONNECTION_STATE		  		0x03020109
#define OID_FDDI_SMT_STATION_ID			 		0x03030201
#define OID_FDDI_SMT_OP_VERSION_ID		  		0x03030202
#define OID_FDDI_SMT_HI_VERSION_ID		  		0x03030203
#define OID_FDDI_SMT_LO_VERSION_ID		  		0x03030204
#define OID_FDDI_SMT_MANUFACTURER_DATA	  		0x03030205
#define OID_FDDI_SMT_USER_DATA			  		0x03030206
#define OID_FDDI_SMT_MIB_VERSION_ID		 		0x03030207
#define OID_FDDI_SMT_MAC_CT				 		0x03030208
#define OID_FDDI_SMT_NON_MASTER_CT		  		0x03030209
#define OID_FDDI_SMT_MASTER_CT			  		0x0303020A
#define OID_FDDI_SMT_AVAILABLE_PATHS			0x0303020B
#define OID_FDDI_SMT_CONFIG_CAPABILITIES		0x0303020C
#define OID_FDDI_SMT_CONFIG_POLICY		  		0x0303020D
#define OID_FDDI_SMT_CONNECTION_POLICY	  		0x0303020E
#define OID_FDDI_SMT_T_NOTIFY			   		0x0303020F
#define OID_FDDI_SMT_STAT_RPT_POLICY			0x03030210
#define OID_FDDI_SMT_TRACE_MAX_EXPIRATION   	0x03030211
#define OID_FDDI_SMT_PORT_INDEXES		   		0x03030212
#define OID_FDDI_SMT_MAC_INDEXES				0x03030213
#define OID_FDDI_SMT_BYPASS_PRESENT		 		0x03030214
#define OID_FDDI_SMT_ECM_STATE			  		0x03030215
#define OID_FDDI_SMT_CF_STATE			   		0x03030216
#define OID_FDDI_SMT_HOLD_STATE			 		0x03030217
#define OID_FDDI_SMT_REMOTE_DISCONNECT_FLAG 	0x03030218
#define OID_FDDI_SMT_STATION_STATUS		 		0x03030219
#define OID_FDDI_SMT_PEER_WRAP_FLAG		 		0x0303021A
#define OID_FDDI_SMT_MSG_TIME_STAMP		 		0x0303021B
#define OID_FDDI_SMT_TRANSITION_TIME_STAMP  	0x0303021C
#define OID_FDDI_SMT_SET_COUNT			  		0x0303021D
#define OID_FDDI_SMT_LAST_SET_STATION_ID		0x0303021E
#define OID_FDDI_MAC_FRAME_STATUS_FUNCTIONS 	0x0303021F
#define OID_FDDI_MAC_BRIDGE_FUNCTIONS	   		0x03030220
#define OID_FDDI_MAC_T_MAX_CAPABILITY	   		0x03030221
#define OID_FDDI_MAC_TVX_CAPABILITY		 		0x03030222
#define OID_FDDI_MAC_AVAILABLE_PATHS			0x03030223
#define OID_FDDI_MAC_CURRENT_PATH		   		0x03030224
#define OID_FDDI_MAC_UPSTREAM_NBR		   		0x03030225
#define OID_FDDI_MAC_DOWNSTREAM_NBR		 		0x03030226
#define OID_FDDI_MAC_OLD_UPSTREAM_NBR	   		0x03030227
#define OID_FDDI_MAC_OLD_DOWNSTREAM_NBR	 		0x03030228
#define OID_FDDI_MAC_DUP_ADDRESS_TEST	   		0x03030229
#define OID_FDDI_MAC_REQUESTED_PATHS			0x0303022A
#define OID_FDDI_MAC_DOWNSTREAM_PORT_TYPE   	0x0303022B
#define OID_FDDI_MAC_INDEX				  		0x0303022C
#define OID_FDDI_MAC_SMT_ADDRESS				0x0303022D
#define OID_FDDI_MAC_LONG_GRP_ADDRESS	   		0x0303022E
#define OID_FDDI_MAC_SHORT_GRP_ADDRESS	  		0x0303022F
#define OID_FDDI_MAC_T_REQ				  		0x03030230
#define OID_FDDI_MAC_T_NEG				  		0x03030231
#define OID_FDDI_MAC_T_MAX				  		0x03030232
#define OID_FDDI_MAC_TVX_VALUE			  		0x03030233
#define OID_FDDI_MAC_T_PRI0				 		0x03030234
#define OID_FDDI_MAC_T_PRI1				 		0x03030235
#define OID_FDDI_MAC_T_PRI2				 		0x03030236
#define OID_FDDI_MAC_T_PRI3				 		0x03030237
#define OID_FDDI_MAC_T_PRI4				 		0x03030238
#define OID_FDDI_MAC_T_PRI5				 		0x03030239
#define OID_FDDI_MAC_T_PRI6				 		0x0303023A
#define OID_FDDI_MAC_FRAME_CT			   		0x0303023B
#define OID_FDDI_MAC_COPIED_CT			  		0x0303023C
#define OID_FDDI_MAC_TRANSMIT_CT				0x0303023D
#define OID_FDDI_MAC_TOKEN_CT			   		0x0303023E
#define OID_FDDI_MAC_ERROR_CT			   		0x0303023F
#define OID_FDDI_MAC_LOST_CT					0x03030240
#define OID_FDDI_MAC_TVX_EXPIRED_CT		 		0x03030241
#define OID_FDDI_MAC_NOT_COPIED_CT		  		0x03030242
#define OID_FDDI_MAC_LATE_CT					0x03030243
#define OID_FDDI_MAC_RING_OP_CT			 		0x03030244
#define OID_FDDI_MAC_FRAME_ERROR_THRESHOLD  	0x03030245
#define OID_FDDI_MAC_FRAME_ERROR_RATIO	  		0x03030246
#define OID_FDDI_MAC_NOT_COPIED_THRESHOLD   	0x03030247
#define OID_FDDI_MAC_NOT_COPIED_RATIO	   		0x03030248
#define OID_FDDI_MAC_RMT_STATE			  		0x03030249
#define OID_FDDI_MAC_DA_FLAG					0x0303024A
#define OID_FDDI_MAC_UNDA_FLAG			  		0x0303024B
#define OID_FDDI_MAC_FRAME_ERROR_FLAG	   		0x0303024C
#define OID_FDDI_MAC_NOT_COPIED_FLAG			0x0303024D
#define OID_FDDI_MAC_MA_UNITDATA_AVAILABLE  	0x0303024E
#define OID_FDDI_MAC_HARDWARE_PRESENT	   		0x0303024F
#define OID_FDDI_MAC_MA_UNITDATA_ENABLE	 		0x03030250
#define OID_FDDI_PATH_INDEX				 		0x03030251
#define OID_FDDI_PATH_RING_LATENCY		  		0x03030252
#define OID_FDDI_PATH_TRACE_STATUS		  		0x03030253
#define OID_FDDI_PATH_SBA_PAYLOAD		   		0x03030254
#define OID_FDDI_PATH_SBA_OVERHEAD		  		0x03030255
#define OID_FDDI_PATH_CONFIGURATION		 		0x03030256
#define OID_FDDI_PATH_T_R_MODE			  		0x03030257
#define OID_FDDI_PATH_SBA_AVAILABLE		 		0x03030258
#define OID_FDDI_PATH_TVX_LOWER_BOUND	   		0x03030259
#define OID_FDDI_PATH_T_MAX_LOWER_BOUND	 		0x0303025A
#define OID_FDDI_PATH_MAX_T_REQ			 		0x0303025B
#define OID_FDDI_PORT_MY_TYPE			   		0x0303025C
#define OID_FDDI_PORT_NEIGHBOR_TYPE		 		0x0303025D
#define OID_FDDI_PORT_CONNECTION_POLICIES   	0x0303025E
#define OID_FDDI_PORT_MAC_INDICATED		 		0x0303025F
#define OID_FDDI_PORT_CURRENT_PATH		  		0x03030260
#define OID_FDDI_PORT_REQUESTED_PATHS	   		0x03030261
#define OID_FDDI_PORT_MAC_PLACEMENT		 		0x03030262
#define OID_FDDI_PORT_AVAILABLE_PATHS	   		0x03030263
#define OID_FDDI_PORT_MAC_LOOP_TIME		 		0x03030264
#define OID_FDDI_PORT_PMD_CLASS			 		0x03030265
#define OID_FDDI_PORT_CONNECTION_CAPABILITIES	0x03030266
#define OID_FDDI_PORT_INDEX				 		0x03030267
#define OID_FDDI_PORT_MAINT_LS			  		0x03030268
#define OID_FDDI_PORT_BS_FLAG			   		0x03030269
#define OID_FDDI_PORT_PC_LS				 		0x0303026A
#define OID_FDDI_PORT_EB_ERROR_CT		   		0x0303026B
#define OID_FDDI_PORT_LCT_FAIL_CT		   		0x0303026C
#define OID_FDDI_PORT_LER_ESTIMATE		  		0x0303026D
#define OID_FDDI_PORT_LEM_REJECT_CT		 		0x0303026E
#define OID_FDDI_PORT_LEM_CT					0x0303026F
#define OID_FDDI_PORT_LER_CUTOFF				0x03030270
#define OID_FDDI_PORT_LER_ALARM			 		0x03030271
#define OID_FDDI_PORT_CONNNECT_STATE			0x03030272
#define OID_FDDI_PORT_PCM_STATE			 		0x03030273
#define OID_FDDI_PORT_PC_WITHHOLD		   		0x03030274
#define OID_FDDI_PORT_LER_FLAG			  		0x03030275
#define OID_FDDI_PORT_HARDWARE_PRESENT	  		0x03030276
#define OID_FDDI_SMT_STATION_ACTION		 		0x03030277
#define OID_FDDI_PORT_ACTION					0x03030278
#define OID_FDDI_IF_DESCR				   		0x03030279
#define OID_FDDI_IF_TYPE						0x0303027A
#define OID_FDDI_IF_MTU					 		0x0303027B
#define OID_FDDI_IF_SPEED				   		0x0303027C
#define OID_FDDI_IF_PHYS_ADDRESS				0x0303027D
#define OID_FDDI_IF_ADMIN_STATUS				0x0303027E
#define OID_FDDI_IF_OPER_STATUS			 		0x0303027F
#define OID_FDDI_IF_LAST_CHANGE			 		0x03030280
#define OID_FDDI_IF_IN_OCTETS			   		0x03030281
#define OID_FDDI_IF_IN_UCAST_PKTS		   		0x03030282
#define OID_FDDI_IF_IN_NUCAST_PKTS		  		0x03030283
#define OID_FDDI_IF_IN_DISCARDS			 		0x03030284
#define OID_FDDI_IF_IN_ERRORS			   		0x03030285
#define OID_FDDI_IF_IN_UNKNOWN_PROTOS	   		0x03030286
#define OID_FDDI_IF_OUT_OCTETS			  		0x03030287
#define OID_FDDI_IF_OUT_UCAST_PKTS		  		0x03030288
#define OID_FDDI_IF_OUT_NUCAST_PKTS		 		0x03030289
#define OID_FDDI_IF_OUT_DISCARDS				0x0303028A
#define OID_FDDI_IF_OUT_ERRORS			  		0x0303028B
#define OID_FDDI_IF_OUT_QLEN					0x0303028C
#define OID_FDDI_IF_SPECIFIC					0x0303028D
//
// WAN objects
//
#define OID_WAN_PERMANENT_ADDRESS		   		0x04010101
#define OID_WAN_CURRENT_ADDRESS			 		0x04010102
#define OID_WAN_QUALITY_OF_SERVICE		  		0x04010103
#define OID_WAN_PROTOCOL_TYPE			   		0x04010104
#define OID_WAN_MEDIUM_SUBTYPE			  		0x04010105
#define OID_WAN_HEADER_FORMAT			   		0x04010106
#define OID_WAN_GET_INFO						0x04010107
#define OID_WAN_SET_LINK_INFO			   		0x04010108
#define OID_WAN_GET_LINK_INFO			   		0x04010109
#define OID_WAN_LINE_COUNT				  		0x0401010A
#define OID_WAN_GET_BRIDGE_INFO			 		0x0401020A
#define OID_WAN_SET_BRIDGE_INFO			 		0x0401020B
#define OID_WAN_GET_COMP_INFO			   		0x0401020C
#define OID_WAN_SET_COMP_INFO			   		0x0401020D
#define OID_WAN_GET_STATS_INFO			  		0x0401020E
//
// LocalTalk objects
//
#define OID_LTALK_CURRENT_NODE_ID		   		0x05010102
#define OID_LTALK_IN_BROADCASTS			 		0x05020101
#define OID_LTALK_IN_LENGTH_ERRORS		  		0x05020102
#define OID_LTALK_OUT_NO_HANDLERS		   		0x05020201
#define OID_LTALK_COLLISIONS					0x05020202
#define OID_LTALK_DEFERS						0x05020203
#define OID_LTALK_NO_DATA_ERRORS				0x05020204
#define OID_LTALK_RANDOM_CTS_ERRORS		 		0x05020205
#define OID_LTALK_FCS_ERRORS					0x05020206
//
// Arcnet objects
//
#define OID_ARCNET_PERMANENT_ADDRESS			0x06010101
#define OID_ARCNET_CURRENT_ADDRESS		  		0x06010102
#define OID_ARCNET_RECONFIGURATIONS		 		0x06020201
//
// TAPI objects
//
#define OID_TAPI_ACCEPT					 		0x07030101
#define OID_TAPI_ANSWER					 		0x07030102
#define OID_TAPI_CLOSE					  		0x07030103
#define OID_TAPI_CLOSE_CALL				 		0x07030104
#define OID_TAPI_CONDITIONAL_MEDIA_DETECTION	0x07030105
#define OID_TAPI_CONFIG_DIALOG			  		0x07030106
#define OID_TAPI_DEV_SPECIFIC			   		0x07030107
#define OID_TAPI_DIAL					   		0x07030108
#define OID_TAPI_DROP					   		0x07030109
#define OID_TAPI_GET_ADDRESS_CAPS		   		0x0703010A
#define OID_TAPI_GET_ADDRESS_ID			 		0x0703010B
#define OID_TAPI_GET_ADDRESS_STATUS		 		0x0703010C
#define OID_TAPI_GET_CALL_ADDRESS_ID			0x0703010D
#define OID_TAPI_GET_CALL_INFO			  		0x0703010E
#define OID_TAPI_GET_CALL_STATUS				0x0703010F
#define OID_TAPI_GET_DEV_CAPS			   		0x07030110
#define OID_TAPI_GET_DEV_CONFIG			 		0x07030111
#define OID_TAPI_GET_EXTENSION_ID		   		0x07030112
#define OID_TAPI_GET_ID					 		0x07030113
#define OID_TAPI_GET_LINE_DEV_STATUS			0x07030114
#define OID_TAPI_MAKE_CALL				  		0x07030115
#define OID_TAPI_NEGOTIATE_EXT_VERSION	  		0x07030116
#define OID_TAPI_OPEN					   		0x07030117
#define OID_TAPI_PROVIDER_INITIALIZE			0x07030118
#define OID_TAPI_PROVIDER_SHUTDOWN		  		0x07030119
#define OID_TAPI_SECURE_CALL					0x0703011A
#define OID_TAPI_SELECT_EXT_VERSION		 		0x0703011B
#define OID_TAPI_SEND_USER_USER_INFO			0x0703011C
#define OID_TAPI_SET_APP_SPECIFIC		   		0x0703011D
#define OID_TAPI_SET_CALL_PARAMS				0x0703011E
#define OID_TAPI_SET_DEFAULT_MEDIA_DETECTION	0x0703011F
#define OID_TAPI_SET_DEV_CONFIG			 		0x07030120
#define OID_TAPI_SET_MEDIA_MODE			 		0x07030121
#define OID_TAPI_SET_STATUS_MESSAGES			0x07030122
//
// ATM Connection Oriented Ndis
//
#define OID_ATM_SUPPORTED_VC_RATES				0x08010101
#define OID_ATM_SUPPORTED_SERVICE_CATEGORY		0x08010102
#define OID_ATM_SUPPORTED_AAL_TYPES				0x08010103
#define OID_ATM_HW_CURRENT_ADDRESS				0x08010104
#define OID_ATM_MAX_ACTIVE_VCS					0x08010105
#define OID_ATM_MAX_ACTIVE_VCI_BITS				0x08010106
#define OID_ATM_MAX_ACTIVE_VPI_BITS				0x08010107
#define OID_ATM_MAX_AAL0_PACKET_SIZE			0x08010108
#define OID_ATM_MAX_AAL1_PACKET_SIZE			0x08010109
#define OID_ATM_MAX_AAL34_PACKET_SIZE			0x0801010A
#define OID_ATM_MAX_AAL5_PACKET_SIZE			0x0801010B
#define OID_ATM_SIGNALING_VPIVCI				0x08010201
#define OID_ATM_ASSIGNED_VPI					0x08010202
#define OID_ATM_ACQUIRE_ACCESS_NET_RESOURCES	0x08010203
#define OID_ATM_RELEASE_ACCESS_NET_RESOURCES	0x08010204
#define OID_ATM_ILMI_VPIVCI						0x08010205
#define OID_ATM_DIGITAL_BROADCAST_VPIVCI		0x08010206
#define	OID_ATM_GET_NEAREST_FLOW				0x08010207
#define OID_ATM_ALIGNMENT_REQUIRED				0x08010208
//
//      ATM specific statistics OIDs.
//
#define	OID_ATM_RCV_CELLS_OK					0x08020101
#define	OID_ATM_XMIT_CELLS_OK					0x08020102
#define	OID_ATM_RCV_CELLS_DROPPED				0x08020103
#define	OID_ATM_RCV_INVALID_VPI_VCI				0x08020201
#define	OID_ATM_CELLS_HEC_ERROR					0x08020202
#define	OID_ATM_RCV_REASSEMBLY_ERROR			0x08020203
//
// PCCA (Wireless) object
//
//
// All WirelessWAN devices must support the following OIDs
//
#define OID_WW_GEN_NETWORK_TYPES_SUPPORTED		0x09010101
#define OID_WW_GEN_NETWORK_TYPE_IN_USE			0x09010102
#define OID_WW_GEN_HEADER_FORMATS_SUPPORTED		0x09010103
#define OID_WW_GEN_HEADER_FORMAT_IN_USE			0x09010104
#define OID_WW_GEN_INDICATION_REQUEST			0x09010105
#define OID_WW_GEN_DEVICE_INFO					0x09010106
#define OID_WW_GEN_OPERATION_MODE				0x09010107
#define OID_WW_GEN_LOCK_STATUS					0x09010108
#define OID_WW_GEN_DISABLE_TRANSMITTER			0x09010109
#define OID_WW_GEN_NETWORK_ID					0x0901010A
#define OID_WW_GEN_PERMANENT_ADDRESS			0x0901010B
#define OID_WW_GEN_CURRENT_ADDRESS				0x0901010C
#define OID_WW_GEN_SUSPEND_DRIVER				0x0901010D
#define OID_WW_GEN_BASESTATION_ID				0x0901010E
#define OID_WW_GEN_CHANNEL_ID					0x0901010F
#define OID_WW_GEN_ENCRYPTION_SUPPORTED			0x09010110
#define OID_WW_GEN_ENCRYPTION_IN_USE			0x09010111
#define OID_WW_GEN_ENCRYPTION_STATE				0x09010112
#define OID_WW_GEN_CHANNEL_QUALITY				0x09010113
#define OID_WW_GEN_REGISTRATION_STATUS			0x09010114
#define OID_WW_GEN_RADIO_LINK_SPEED				0x09010115
#define OID_WW_GEN_LATENCY						0x09010116
#define OID_WW_GEN_BATTERY_LEVEL				0x09010117
#define OID_WW_GEN_EXTERNAL_POWER				0x09010118
//
// Network Dependent OIDs - Mobitex:
//
#define OID_WW_MBX_SUBADDR						0x09050101
// OID 0x09050102 is reserved and may not be used
#define OID_WW_MBX_FLEXLIST						0x09050103
#define OID_WW_MBX_GROUPLIST					0x09050104
#define OID_WW_MBX_TRAFFIC_AREA					0x09050105
#define OID_WW_MBX_LIVE_DIE						0x09050106
#define OID_WW_MBX_TEMP_DEFAULTLIST				0x09050107
//
// Network Dependent OIDs - Pinpoint:
//
#define OID_WW_PIN_LOC_AUTHORIZE				0x09090101
#define OID_WW_PIN_LAST_LOCATION				0x09090102
#define OID_WW_PIN_LOC_FIX						0x09090103
//
// Network Dependent - CDPD:
//
#define OID_WW_CDPD_SPNI						0x090D0101
#define OID_WW_CDPD_WASI						0x090D0102
#define OID_WW_CDPD_AREA_COLOR					0x090D0103
#define OID_WW_CDPD_TX_POWER_LEVEL				0x090D0104
#define OID_WW_CDPD_EID							0x090D0105
#define OID_WW_CDPD_HEADER_COMPRESSION			0x090D0106
#define OID_WW_CDPD_DATA_COMPRESSION			0x090D0107
#define OID_WW_CDPD_CHANNEL_SELECT				0x090D0108
#define OID_WW_CDPD_CHANNEL_STATE				0x090D0109
#define OID_WW_CDPD_NEI							0x090D010A
#define OID_WW_CDPD_NEI_STATE					0x090D010B
#define OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER	0x090D010C
#define OID_WW_CDPD_SLEEP_MODE					0x090D010D
#define OID_WW_CDPD_CIRCUIT_SWITCHED			0x090D010E
#define	OID_WW_CDPD_TEI							0x090D010F
#define	OID_WW_CDPD_RSSI						0x090D0110
//
// Network Dependent - Ardis:
//
#define OID_WW_ARD_SNDCP						0x09110101
#define OID_WW_ARD_TMLY_MSG						0x09110102
#define OID_WW_ARD_DATAGRAM						0x09110103
//
// Network Dependent - DataTac:
//
#define OID_WW_TAC_COMPRESSION					0x09150101
#define OID_WW_TAC_SET_CONFIG					0x09150102
#define OID_WW_TAC_GET_STATUS					0x09150103
#define OID_WW_TAC_USER_HEADER					0x09150104
//
// Network Dependent - Metricom:
//
#define OID_WW_MET_FUNCTION						0x09190101
//
// IRDA objects
//
#define OID_IRDA_RECEIVING						0x0A010100
#define OID_IRDA_TURNAROUND_TIME				0x0A010101
#define OID_IRDA_SUPPORTED_SPEEDS				0x0A010102
#define OID_IRDA_LINK_SPEED						0x0A010103
#define OID_IRDA_MEDIA_BUSY						0x0A010104
#define OID_IRDA_EXTRA_RCV_BOFS					0x0A010200
#define OID_IRDA_RATE_SNIFF						0x0A010201
#define OID_IRDA_UNICAST_LIST					0x0A010202
#define OID_IRDA_MAX_UNICAST_LIST_SIZE			0x0A010203
#define OID_IRDA_MAX_RECEIVE_WINDOW_SIZE		0x0A010204
#define OID_IRDA_MAX_SEND_WINDOW_SIZE			0x0A010205
//
// Medium the Ndis Driver is running on (OID_GEN_MEDIA_SUPPORTED/
// OID_GEN_MEDIA_IN_USE).
//
typedef enum _NDIS_MEDIUM {
    NdisMedium802_3,
    NdisMedium802_5,
    NdisMediumFddi,
    NdisMediumWan,
    NdisMediumLocalTalk,
    NdisMediumDix,		// defined for convenience, not a real medium
     NdisMediumArcnetRaw,
    NdisMediumArcnet878_2,
    NdisMediumAtm,
    NdisMediumWirelessWan,
    NdisMediumIrda,
    NdisMediumMax		// Not a real medium, defined as an upper-bound
} NDIS_MEDIUM, *PNDIS_MEDIUM;

//
// Hardware status codes (OID_GEN_HARDWARE_STATUS).
//
typedef enum _NDIS_HARDWARE_STATUS {
    NdisHardwareStatusReady,
    NdisHardwareStatusInitializing,
    NdisHardwareStatusReset,
    NdisHardwareStatusClosing,
    NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;

//
// this is the type passed in the OID_GEN_GET_TIME_CAPS request
//
typedef struct _GEN_GET_TIME_CAPS {
    ULONG Flags;		// Bits defined below

    ULONG ClockPrecision;
} GEN_GET_TIME_CAPS, *PGEN_GET_TIME_CAPS;

#define	READABLE_LOCAL_CLOCK					0x000000001
#define	CLOCK_NETWORK_DERIVED					0x000000002
#define	CLOCK_PRECISION							0x000000004
#define	RECEIVE_TIME_INDICATION_CAPABLE			0x000000008
#define	TIMED_SEND_CAPABLE						0x000000010
#define	TIME_STAMP_CAPABLE						0x000000020
//
//
// this is the type passed in the OID_GEN_GET_NETCARD_TIME request
//
typedef struct _GEN_GET_NETCARD_TIME {
    ULONG ReadTime;
} GEN_GET_NETCARD_TIME, *PGEN_GET_NETCARD_TIME;

//
// Defines the attachment types for FDDI (OID_FDDI_ATTACHMENT_TYPE).
//
typedef enum _NDIS_FDDI_ATTACHMENT_TYPE {
    NdisFddiTypeIsolated = 1,
    NdisFddiTypeLocalA,
    NdisFddiTypeLocalB,
    NdisFddiTypeLocalAB,
    NdisFddiTypeLocalS,
    NdisFddiTypeWrapA,
    NdisFddiTypeWrapB,
    NdisFddiTypeWrapAB,
    NdisFddiTypeWrapS,
    NdisFddiTypeCWrapA,
    NdisFddiTypeCWrapB,
    NdisFddiTypeCWrapS,
    NdisFddiTypeThrough
} NDIS_FDDI_ATTACHMENT_TYPE, *PNDIS_FDDI_ATTACHMENT_TYPE;

//
// Defines the ring management states for FDDI (OID_FDDI_RING_MGT_STATE).
//
typedef enum _NDIS_FDDI_RING_MGT_STATE {
    NdisFddiRingIsolated = 1,
    NdisFddiRingNonOperational,
    NdisFddiRingOperational,
    NdisFddiRingDetect,
    NdisFddiRingNonOperationalDup,
    NdisFddiRingOperationalDup,
    NdisFddiRingDirected,
    NdisFddiRingTrace
} NDIS_FDDI_RING_MGT_STATE, *PNDIS_FDDI_RING_MGT_STATE;

//
// Defines the Lconnection state for FDDI (OID_FDDI_LCONNECTION_STATE).
//
typedef enum _NDIS_FDDI_LCONNECTION_STATE {
    NdisFddiStateOff = 1,
    NdisFddiStateBreak,
    NdisFddiStateTrace,
    NdisFddiStateConnect,
    NdisFddiStateNext,
    NdisFddiStateSignal,
    NdisFddiStateJoin,
    NdisFddiStateVerify,
    NdisFddiStateActive,
    NdisFddiStateMaintenance
} NDIS_FDDI_LCONNECTION_STATE, *PNDIS_FDDI_LCONNECTION_STATE;

//
// Defines the medium subtypes for WAN medium (OID_WAN_MEDIUM_SUBTYPE).
//
typedef enum _NDIS_WAN_MEDIUM_SUBTYPE {
    NdisWanMediumHub,
    NdisWanMediumX_25,
    NdisWanMediumIsdn,
    NdisWanMediumSerial,
    NdisWanMediumFrameRelay,
    NdisWanMediumAtm,
    NdisWanMediumSonet,
    NdisWanMediumSW56K
} NDIS_WAN_MEDIUM_SUBTYPE, *PNDIS_WAN_MEDIUM_SUBTYPE;

//
// Defines the header format for WAN medium (OID_WAN_HEADER_FORMAT).
//
typedef enum _NDIS_WAN_HEADER_FORMAT {
    NdisWanHeaderNative,	// src/dest based on subtype, followed by NLPID
     NdisWanHeaderEthernet	// emulation of ethernet header
} NDIS_WAN_HEADER_FORMAT, *PNDIS_WAN_HEADER_FORMAT;

//
// Defines the line quality on a WAN line (OID_WAN_QUALITY_OF_SERVICE).
//
typedef enum _NDIS_WAN_QUALITY {
    NdisWanRaw,
    NdisWanErrorControl,
    NdisWanReliable
} NDIS_WAN_QUALITY, *PNDIS_WAN_QUALITY;

//
// Defines the state of a token-ring adapter (OID_802_5_CURRENT_RING_STATE).
//
typedef enum _NDIS_802_5_RING_STATE {
    NdisRingStateOpened = 1,
    NdisRingStateClosed,
    NdisRingStateOpening,
    NdisRingStateClosing,
    NdisRingStateOpenFailure,
    NdisRingStateRingFailure
} NDIS_802_5_RING_STATE, *PNDIS_802_5_RING_STATE;

//
// Defines the state of the LAN media
//
typedef enum _NDIS_MEDIA_STATE {
    NdisMediaStateConnected,
    NdisMediaStateDisconnected
} NDIS_MEDIA_STATE, *PNDIS_MEDIA_STATE;

//
// The following is set on a per-packet basis as OOB data with NdisClass802_3Priority
//
typedef ULONG Priority_802_3;	// 0-7 priority levels
//
//      The following structure is used to query OID_GEN_CO_LINK_SPEED and
//      OID_GEN_CO_MINIMUM_LINK_SPEED.  The first OID will return the current
//      link speed of the adapter.  The second will return the minimum link speed
//      the adapter is capable of.
//

typedef struct _NDIS_CO_LINK_SPEED {
    ULONG Outbound;
    ULONG Inbound;
} NDIS_CO_LINK_SPEED,

*PNDIS_CO_LINK_SPEED;
//
// Ndis Packet Filter Bits (OID_GEN_CURRENT_PACKET_FILTER).
//
#define NDIS_PACKET_TYPE_DIRECTED				0x0001
#define NDIS_PACKET_TYPE_MULTICAST				0x0002
#define NDIS_PACKET_TYPE_ALL_MULTICAST			0x0004
#define NDIS_PACKET_TYPE_BROADCAST				0x0008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING			0x0010
#define NDIS_PACKET_TYPE_PROMISCUOUS			0x0020
#define NDIS_PACKET_TYPE_SMT					0x0040
#define NDIS_PACKET_TYPE_ALL_LOCAL				0x0080
#define NDIS_PACKET_TYPE_MAC_FRAME				0x8000
#define NDIS_PACKET_TYPE_FUNCTIONAL				0x4000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL			0x2000
#define NDIS_PACKET_TYPE_GROUP					0x1000
//
// Ndis Token-Ring Ring Status Codes (OID_802_5_CURRENT_RING_STATUS).
//
#define NDIS_RING_SIGNAL_LOSS					0x00008000
#define NDIS_RING_HARD_ERROR					0x00004000
#define NDIS_RING_SOFT_ERROR					0x00002000
#define NDIS_RING_TRANSMIT_BEACON				0x00001000
#define NDIS_RING_LOBE_WIRE_FAULT				0x00000800
#define NDIS_RING_AUTO_REMOVAL_ERROR			0x00000400
#define NDIS_RING_REMOVE_RECEIVED				0x00000200
#define NDIS_RING_COUNTER_OVERFLOW				0x00000100
#define NDIS_RING_SINGLE_STATION				0x00000080
#define NDIS_RING_RING_RECOVERY					0x00000040
//
// Ndis protocol option bits (OID_GEN_PROTOCOL_OPTIONS).
//
#define NDIS_PROT_OPTION_ESTIMATED_LENGTH   	0x00000001
#define NDIS_PROT_OPTION_NO_LOOPBACK			0x00000002
#define NDIS_PROT_OPTION_NO_RSVD_ON_RCVPKT		0x00000004
//
// Ndis MAC option bits (OID_GEN_MAC_OPTIONS).
//
#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA 	0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED  	0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  	0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK				0x00000008
#define NDIS_MAC_OPTION_FULL_DUPLEX				0x00000010
#define	NDIS_MAC_OPTION_EOTX_INDICATION			0x00000020
#define NDIS_MAC_OPTION_RESERVED				0x80000000
//
//      NDIS MAC option bits for OID_GEN_CO_MAC_OPTIONS.
//
#define	NDIS_CO_MAC_OPTION_DYNAMIC_LINK_SPEED	0x00000001
#ifdef	IRDA
//
// The following is set on a per-packet basis as OOB data with NdisClassIrdaPacketInfo
// This is the per-packet info specified on a per-packet basis
//
typedef struct _NDIS_IRDA_PACKET_INFO {
    UINT ExtraBOFs;
    UINT MinTurnAroundTime;
} NDIS_IRDA_PACKET_INFO, *PNDIS_IRDA_PACKET_INFO;

#endif
#ifdef  WIRELESS_WAN
//
// Wireless WAN structure definitions
//
//
// currently defined Wireless network subtypes
//
typedef enum _NDIS_WW_NETWORK_TYPE {
    NdisWWGeneric,
    NdisWWMobitex,
    NdisWWPinpoint,
    NdisWWCDPD,
    NdisWWArdis,
    NdisWWDataTAC,
    NdisWWMetricom,
    NdisWWGSM,
    NdisWWCDMA,
    NdisWWTDMA,
    NdisWWAMPS,
    NdisWWInmarsat,
    NdisWWpACT
} NDIS_WW_NETWORK_TYPE;

//
// currently defined header formats
//
typedef enum _NDIS_WW_HEADER_FORMAT {
    NdisWWDIXEthernetFrames,
    NdisWWMPAKFrames,
    NdisWWRDLAPFrames,
    NdisWWMDC4800Frames
} NDIS_WW_HEADER_FORMAT;

//
// currently defined encryption types
//
typedef enum _NDIS_WW_ENCRYPTION_TYPE {
    NdisWWUnknownEncryption = -1,
    NdisWWNoEncryption,
    NdisWWDefaultEncryption
} NDIS_WW_ENCRYPTION_TYPE, *PNDIS_WW_ENCRYPTION_TYPE;

//
// OID_WW_GEN_INDICATION_REQUEST
//
typedef struct _NDIS_WW_INDICATION_REQUEST {
    NDIS_OID Oid;		// IN

    UINT uIndicationFlag;	// IN

    UINT uApplicationToken;	// IN OUT

    HANDLE hIndicationHandle;	// IN OUT

    INT iPollingInterval;	// IN OUT

    NDIS_VAR_DATA_DESC InitialValue;	// IN OUT

    NDIS_VAR_DATA_DESC OIDIndicationValue;	// OUT - only valid after indication

    NDIS_VAR_DATA_DESC TriggerValue;	// IN

} NDIS_WW_INDICATION_REQUEST, *PNDIS_WW_INDICATION_REQUEST;

#define OID_INDICATION_REQUEST_ENABLE			0x0000
#define OID_INDICATION_REQUEST_CANCEL			0x0001
//
// OID_WW_GEN_DEVICE_INFO
//
typedef struct _WW_DEVICE_INFO {
    NDIS_VAR_DATA_DESC Manufacturer;
    NDIS_VAR_DATA_DESC ModelNum;
    NDIS_VAR_DATA_DESC SWVersionNum;
    NDIS_VAR_DATA_DESC SerialNum;
} WW_DEVICE_INFO, *PWW_DEVICE_INFO;

//
// OID_WW_GEN_OPERATION_MODE
//
typedef INT WW_OPERATION_MODE;	//  0 = Normal mode
												//  1 = Power saving mode
												// -1 = mode unknown
//
// OID_WW_GEN_LOCK_STATUS
//

typedef INT WW_LOCK_STATUS;	//  0 = unlocked
												//  1 = locked
												// -1 = unknown lock status
//
// OID_WW_GEN_DISABLE_TRANSMITTER
//

typedef INT WW_DISABLE_TRANSMITTER;	//  0 = transmitter enabled
												//  1 = transmitter disabled
												// -1 = unknown value
//
// OID_WW_GEN_NETWORK_ID
//

typedef NDIS_VAR_DATA_DESC WW_NETWORK_ID;
//
// OID_WW_GEN_PERMANENT_ADDRESS 
//
typedef NDIS_VAR_DATA_DESC WW_PERMANENT_ADDRESS;
//
// OID_WW_GEN_CURRENT_ADDRESS   
//
typedef struct _WW_CURRENT_ADDRESS {
    NDIS_WW_HEADER_FORMAT Format;
    NDIS_VAR_DATA_DESC Address;
} WW_CURRENT_ADDRESS, *PWW_CURRENT_ADDRESS;

//
// OID_WW_GEN_SUSPEND_DRIVER
//
typedef BOOLEAN WW_SUSPEND_DRIVER;	// 0 = driver operational
												// 1 = driver suspended
//
// OID_WW_GEN_BASESTATION_ID
//

typedef NDIS_VAR_DATA_DESC WW_BASESTATION_ID;
//
// OID_WW_GEN_CHANNEL_ID
//
typedef NDIS_VAR_DATA_DESC WW_CHANNEL_ID;
//
// OID_WW_GEN_ENCRYPTION_STATE
//
typedef BOOLEAN WW_ENCRYPTION_STATE;	// 0 = if encryption is disabled
												// 1 = if encryption is enabled
//
// OID_WW_GEN_CHANNEL_QUALITY
//

typedef INT WW_CHANNEL_QUALITY;	//  0 = Not in network contact,
												// 1-100 = Quality of Channel (100 is highest quality).
												// -1 = channel quality is unknown
//
// OID_WW_GEN_REGISTRATION_STATUS
//

typedef INT WW_REGISTRATION_STATUS;	//  0 = Registration denied
												//  1 = Registration pending
												//  2 = Registered
												// -1 = unknown registration status
//
// OID_WW_GEN_RADIO_LINK_SPEED
//

typedef UINT WW_RADIO_LINK_SPEED;	// Bits per second.
//
// OID_WW_GEN_LATENCY
//

typedef UINT WW_LATENCY;	//  milliseconds
//
// OID_WW_GEN_BATTERY_LEVEL
//

typedef INT WW_BATTERY_LEVEL;	//  0-100 = battery level in percentage
												//      (100=fully charged)
												// -1 = unknown battery level.
//
// OID_WW_GEN_EXTERNAL_POWER
//

typedef INT WW_EXTERNAL_POWER;	//   0 = no external power connected
												//   1 = external power connected
												//  -1 = unknown
//
// OID_WW_MET_FUNCTION
//

typedef NDIS_VAR_DATA_DESC WW_MET_FUNCTION;
//
// OID_WW_TAC_COMPRESSION
//
typedef BOOLEAN WW_TAC_COMPRESSION;	// Determines whether or not network level compression
												// is being used.
//
// OID_WW_TAC_SET_CONFIG
//

typedef struct _WW_TAC_SETCONFIG {
    NDIS_VAR_DATA_DESC RCV_MODE;
    NDIS_VAR_DATA_DESC TX_CONTROL;
    NDIS_VAR_DATA_DESC RX_CONTROL;
    NDIS_VAR_DATA_DESC FLOW_CONTROL;
    NDIS_VAR_DATA_DESC RESET_CNF;
    NDIS_VAR_DATA_DESC READ_CNF;
} WW_TAC_SETCONFIG, *PWW_TAC_SETCONFIG;

//
// OID_WW_TAC_GET_STATUS
//
typedef struct _WW_TAC_GETSTATUS {
    BOOLEAN Action;		// Set = Execute command.

    NDIS_VAR_DATA_DESC Command;
    NDIS_VAR_DATA_DESC Option;
    NDIS_VAR_DATA_DESC Response;	// The response to the requested command
    // - max. length of string is 256 octets.

} WW_TAC_GETSTATUS, *PWW_TAC_GETSTATUS;

//
// OID_WW_TAC_USER_HEADER
//
typedef NDIS_VAR_DATA_DESC WW_TAC_USERHEADER;	// This will hold the user header - Max. 64 octets.
//
// OID_WW_ARD_SNDCP
//

typedef struct _WW_ARD_SNDCP {
    NDIS_VAR_DATA_DESC Version;	// The version of SNDCP protocol supported.

    INT BlockSize;		// The block size used for SNDCP

    INT Window;			// The window size used in SNDCP

} WW_ARD_SNDCP, *PWW_ARD_SNDCP;

//
// OID_WW_ARD_TMLY_MSG
//
typedef BOOLEAN WW_ARD_CHANNEL_STATUS;	// The current status of the inbound RF Channel.
//
// OID_WW_ARD_DATAGRAM
//

typedef struct _WW_ARD_DATAGRAM {
    BOOLEAN LoadLevel;		// Byte that contains the load level info.

    INT SessionTime;		// Datagram session time remaining.

    NDIS_VAR_DATA_DESC HostAddr;	// Host address.

    NDIS_VAR_DATA_DESC THostAddr;	// Test host address.

} WW_ARD_DATAGRAM, *PWW_ARD_DATAGRAM;

//
// OID_WW_CDPD_SPNI
//
typedef struct _WW_CDPD_SPNI {
    UINT SPNI[10];		//10 16-bit service provider network IDs

    INT OperatingMode;		// 0 = ignore SPNI,
    // 1 = require SPNI from list,
    // 2 = prefer SPNI from list.
    // 3 = exclude SPNI from list.

} WW_CDPD_SPNI, *PWW_CDPD_SPNI;

//
// OID_WW_CDPD_WASI
//
typedef struct _WW_CDPD_WIDE_AREA_SERVICE_ID {
    UINT WASI[10];		//10 16-bit wide area service IDs

    INT OperatingMode;		// 0 = ignore WASI,
    // 1 = Require WASI from list,
    // 2 = prefer WASI from list
    // 3 = exclude WASI from list.

} WW_CDPD_WIDE_AREA_SERVICE_ID, *PWW_CDPD_WIDE_AREA_SERVICE_ID;

//
// OID_WW_CDPD_AREA_COLOR
//
typedef INT WW_CDPD_AREA_COLOR;
//
// OID_WW_CDPD_TX_POWER_LEVEL
//
typedef UINT WW_CDPD_TX_POWER_LEVEL;
//
// OID_WW_CDPD_EID
//
typedef NDIS_VAR_DATA_DESC WW_CDPD_EID;
//
// OID_WW_CDPD_HEADER_COMPRESSION
//
typedef INT WW_CDPD_HEADER_COMPRESSION;		//  0 = no header compression,
												//  1 = always compress headers,
												//  2 = compress headers if MD-IS does
												// -1 = unknown
//
// OID_WW_CDPD_DATA_COMPRESSION
//

typedef INT WW_CDPD_DATA_COMPRESSION;	// 0  = no data compression,
												// 1  = data compression enabled
												// -1 =  unknown
//
// OID_WW_CDPD_CHANNEL_SELECT
//

typedef struct _WW_CDPD_CHANNEL_SELECT {
    UINT ChannelID;		// channel number

    UINT fixedDuration;		// duration in seconds

} WW_CDPD_CHANNEL_SELECT, *PWW_CDPD_CHANNEL_SELECT;

//
// OID_WW_CDPD_CHANNEL_STATE
//
typedef enum _WW_CDPD_CHANNEL_STATE {
    CDPDChannelNotAvail,
    CDPDChannelScanning,
    CDPDChannelInitAcquired,
    CDPDChannelAcquired,
    CDPDChannelSleeping,
    CDPDChannelWaking,
    CDPDChannelCSDialing,
    CDPDChannelCSRedial,
    CDPDChannelCSAnswering,
    CDPDChannelCSConnected,
    CDPDChannelCSSuspended
} WW_CDPD_CHANNEL_STATE, *PWW_CDPD_CHANNEL_STATE;

//
// OID_WW_CDPD_NEI
//
typedef enum _WW_CDPD_NEI_FORMAT {
    CDPDNeiIPv4,
    CDPDNeiCLNP,
    CDPDNeiIPv6
} WW_CDPD_NEI_FORMAT, *PWW_CDPD_NEI_FORMAT;
typedef enum _WW_CDPD_NEI_TYPE {
    CDPDNeiIndividual,
    CDPDNeiMulticast,
    CDPDNeiBroadcast
} WW_CDPD_NEI_TYPE;
typedef struct _WW_CDPD_NEI {
    UINT uNeiIndex;
    WW_CDPD_NEI_FORMAT NeiFormat;
    WW_CDPD_NEI_TYPE NeiType;
    WORD NeiGmid;		// group member identifier, only
    // meaningful if NeiType ==
    // CDPDNeiMulticast

    NDIS_VAR_DATA_DESC NeiAddress;
} WW_CDPD_NEI;

//
// OID_WW_CDPD_NEI_STATE
//
typedef enum _WW_CDPD_NEI_STATE {
    CDPDUnknown,
    CDPDRegistered,
    CDPDDeregistered
} WW_CDPD_NEI_STATE, *PWW_CDPD_NEI_STATE;
typedef enum _WW_CDPD_NEI_SUB_STATE {
    CDPDPending,		// Registration pending
     CDPDNoReason,		// Registration denied - no reason given
     CDPDMDISNotCapable,	// Registration denied - MD-IS not capable of
    //  handling M-ES at this time
     CDPDNEINotAuthorized,	// Registration denied - NEI is not authorized to
    //  use this subnetwork
     CDPDInsufficientAuth,	// Registration denied - M-ES gave insufficient
    //  authentication credentials
     CDPDUnsupportedAuth,	// Registration denied - M-ES gave unsupported
    //  authentication credentials
     CDPDUsageExceeded,		// Registration denied - NEI has exceeded usage
    //  limitations
     CDPDDeniedThisNetwork	// Registration denied on this network, service
    //  may be obtained on alternate Service Provider
    //  network
} WW_CDPD_NEI_SUB_STATE;
typedef struct _WW_CDPD_NEI_REG_STATE {
    UINT uNeiIndex;
    WW_CDPD_NEI_STATE NeiState;
    WW_CDPD_NEI_SUB_STATE NeiSubState;
} WW_CDPD_NEI_REG_STATE, *PWW_CDPD_NEI_REG_STATE;

//
// OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER
//
typedef struct _WW_CDPD_SERVICE_PROVIDER_ID {
    UINT SPI[10];		//10 16-bit service provider IDs

    INT OperatingMode;		// 0 = ignore SPI,
    // 1 = require SPI from list,
    // 2 = prefer SPI from list.
    // 3 = exclude SPI from list.

} WW_CDPD_SERVICE_PROVIDER_ID, *PWW_CDPD_SERVICE_PROVIDER_ID;

//
// OID_WW_CDPD_SLEEP_MODE
//
typedef INT WW_CDPD_SLEEP_MODE;
//
// OID_WW_CDPD_TEI
//
typedef ULONG WW_CDPD_TEI;
//
// OID_WW_CDPD_CIRCUIT_SWITCHED
//
typedef struct _WW_CDPD_CIRCUIT_SWITCHED {
    INT service_preference;	// -1 = unknown,
    //  0 = always use packet switched CDPD,
    //  1 = always use CS CDPD via AMPS,
    //  2 = always use CS CDPD via PSTN,
    //  3 = use circuit switched via AMPS only
    //      when packet switched is not available.
    //  4 = use packet switched only when circuit
    //   switched via AMPS is not available.
    //  5 = device manuf. defined service
    //   preference.
    //  6 = device manuf. defined service
    //   preference.

    INT service_status;		// -1 = unknown,
    //  0 = packet switched CDPD,
    //  1 = circuit switched CDPD via AMPS,
    //  2 = circuit switched CDPD via PSTN.

    INT connect_rate;		//  CS connection bit rate (bits per second).
    //  0 = no active connection,
    // -1 = unknown
    //  Dial code last used to dial.

    NDIS_VAR_DATA_DESC dial_code[20];

    UINT sid;			//  Current AMPS system ID

    INT a_b_side_selection;	// -1 = unknown,
    //  0 = no AMPS service
    //  1 = AMPS "A" side channels selected
    //  2 = AMPS "B" side channels selected

    INT AMPS_channel;		// -1= unknown
    //  0 = no AMPS service.
    //  1-1023 = AMPS channel number in use

    UINT action;		//  0 = no action
    //  1 = suspend (hangup)
    //  2 = dial

    //  Default dial code for CS CDPD service
    //  encoded as specified in the CS CDPD
    //  implementor guidelines.
    NDIS_VAR_DATA_DESC default_dial[20];

    //  Number for the CS CDPD network to call
    //   back the mobile, encoded as specified in
    //   the CS CDPD implementor guidelines.
    NDIS_VAR_DATA_DESC call_back[20];

    UINT sid_list[10];		//  List of 10 16-bit preferred AMPS
    //   system IDs for CS CDPD.

    UINT inactivity_timer;	//  Wait time after last data before dropping
    //   call.
    //  0-65535 = inactivity time limit (seconds).

    UINT receive_timer;		//  secs. per CS-CDPD Implementor Guidelines.

    UINT conn_resp_timer;	//  secs. per CS-CDPD Implementor Guidelines.

    UINT reconn_resp_timer;	//  secs. per CS-CDPD Implementor Guidelines.

    UINT disconn_timer;		//  secs. per CS-CDPD Implementor Guidelines.

    UINT NEI_reg_timer;		//  secs. per CS-CDPD Implementor Guidelines.

    UINT reconn_retry_timer;	//  secs. per CS-CDPD Implementor Guidelines.

    UINT link_reset_timer;	//  secs. per CS-CDPD Implementor Guidelines.

    UINT link_reset_ack_timer;	//  secs. per CS-CDPD Implementor Guidelines.

    UINT n401_retry_limit;	//  per CS-CDPD Implementor Guidelines.

    UINT n402_retry_limit;	//  per CS-CDPD Implementor Guidelines.

    UINT n404_retry_limit;	//  per CS-CDPD Implementor Guidelines.

    UINT n405_retry_limit;	//  per CS-CDPD Implementor Guidelines.

} WW_CDPD_CIRCUIT_SWITCHED, *WW_PCDPD_CIRCUIT_SWITCHED;
typedef UINT WW_CDPD_RSSI;
//
// OID_WW_PIN_LOC_AUTHORIZE
//
typedef INT WW_PIN_AUTHORIZED;	// 0  = unauthorized
												// 1  = authorized
												// -1 = unknown
//
// OID_WW_PIN_LAST_LOCATION
// OID_WW_PIN_LOC_FIX
//

typedef struct _WW_PIN_LOCATION {
    INT Latitude;		// Latitude in hundredths of a second

    INT Longitude;		// Longitude in hundredths of a second

    INT Altitude;		// Altitude in feet

    INT FixTime;		// Time of the location fix, since midnight,  local time (of the
    // current day), in tenths of a second

    INT NetTime;		// Current local network time of the current day, since midnight,
    // in tenths of a second

    INT LocQuality;		// 0-100 = location quality

    INT LatReg;			// Latitude registration offset, in hundredths of a second

    INT LongReg;		// Longitude registration offset, in hundredths of a second

    INT GMTOffset;		// Offset in minutes of the local time zone from GMT

} WW_PIN_LOCATION, *PWW_PIN_LOCATION;

//
// The following is set on a per-packet basis as OOB data with NdisClassWirelessWanMbxMailbox
//
typedef ULONG WW_MBX_MAILBOX_FLAG;	// 1 = set mailbox flag, 0 = do not set mailbox flag
//
// OID_WW_MBX_SUBADDR
//

typedef struct _WW_MBX_PMAN {
    BOOLEAN ACTION;		// 0 = Login PMAN,  1 = Logout PMAN

    UINT MAN;
    UCHAR PASSWORD[8];		// Password should be null for Logout and indications.
    // Maximum length of password is 8 chars.

} WW_MBX_PMAN, *PWW_MBX_PMAN;

//
// OID_WW_MBX_FLEXLIST
//
typedef struct _WW_MBX_FLEXLIST {
    INT count;			//  Number of MAN entries used.
    // -1=unknown.

    UINT MAN[7];		//  List of MANs.

} WW_MBX_FLEXLIST;

//
// OID_WW_MBX_GROUPLIST
//
typedef struct _WW_MBX_GROUPLIST {
    INT count;			//  Number of MAN entries used.
    // -1=unknown.

    UINT MAN[15];		//  List of MANs.

} WW_MBX_GROUPLIST;

//
// OID_WW_MBX_TRAFFIC_AREA
//
typedef enum _WW_MBX_TRAFFIC_AREA {
    unknown_traffic_area,	// The driver has no information about the current traffic area.
     in_traffic_area,		// Mobile unit has entered a subscribed traffic area.
     in_auth_traffic_area,	// Mobile unit is outside traffic area but is authorized.
     unauth_traffic_area	// Mobile unit is outside traffic area but is un-authorized.
} WW_MBX_TRAFFIC_AREA;

//
// OID_WW_MBX_LIVE_DIE
//
typedef INT WW_MBX_LIVE_DIE;	//  0 = DIE last received       
												//  1 = LIVE last received
												// -1 = unknown
//
// OID_WW_MBX_TEMP_DEFAULTLIST
//

typedef struct _WW_MBX_CHANNEL_PAIR {
    UINT Mobile_Tx;
    UINT Mobile_Rx;
} WW_MBX_CHANNEL_PAIR, *PWW_MBX_CHANNEL_PAIR;
typedef struct _WW_MBX_TEMPDEFAULTLIST {
    UINT Length;
    WW_MBX_CHANNEL_PAIR ChannelPair[1];
} WW_MBX_TEMPDEFAULTLIST, *WW_PMBX_TEMPDEFAULTLIST;

#endif				// WIRELESS_WAN
#endif				// _NTDDNDIS_
