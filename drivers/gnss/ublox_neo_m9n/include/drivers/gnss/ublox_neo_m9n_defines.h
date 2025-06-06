/** @file
 * @brief NEO-M9N gnss module public API definitions header file.
 *
 * Copyright (c) 2025 Liam Mulhern
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_UBLOX_NEO_M9N_DEFINES_H_
#define ZEPHYR_INCLUDE_DRIVERS_GNSS_UBLOX_NEO_M9N_DEFINES_H_

enum interface_description {
    NBYTES_HIGH_ADDR = 0xFD,
    NBYTES_LOW_ADDR = 0xFE,
    DATA_STREAM_ADDR = 0xFF,

    MAX_NMEA_SIZE = 83,
    MAX_PAYLOAD_SIZE = 256,

    UBX_SEC_UNIQID = 0x03,

    UBX_UPD_SOS = 0x14,
};

enum ubx_class_ids {
    UBX_CLASS_NAV = 0x01,
    UBX_CLASS_RXM = 0x02,
    UBX_CLASS_INF = 0x04,
    UBX_CLASS_ACK = 0x05,
    UBX_CLASS_CFG = 0x06,
    UBX_CLASS_UPD = 0x09,
    UBX_CLASS_MON = 0x0A,
    UBX_CLASS_AID = 0x0B,
    UBX_CLASS_TIM = 0x0D,
    UBX_CLASS_ESF = 0x10,
    UBX_CLASS_MGA = 0x13,
    UBX_CLASS_LOG = 0x21,
    UBX_CLASS_SEC = 0x27,
    UBX_CLASS_HNR = 0x28,
    UBX_CLASS_NMEA = 0xF0,
};

enum ubx_ack_nack_messages {
    UBX_ACK_ACK = 0x01,
    UBX_ACK_NAK = 0x00,
};

enum ubx_assistnow_aiding_messages {
    UBX_AID_ALM = 0x30,
    UBX_AID_AOP = 0x33,
    UBX_AID_EPH = 0x31,
    UBX_AID_HUI = 0x02,
    UBX_AID_INI = 0x01,
};

enum ubx_configuration_input_messages {
    UBX_CFG_ANT = 0x13,
    UBX_CFG_BATCH = 0x93,
    UBX_CFG_CFG = 0x09,
    UBX_CFG_DAT = 0x06,
    UBX_CFG_DGNSS = 0x70,
    UBX_CFG_DOSC = 0x61,
    UBX_CFG_ESFALG = 0x56,
    UBX_CFG_ESFAE = 0x4C,
    UBX_CFG_ESFGE = 0x4D,
    UBX_CFG_ESFWTE = 0x82,
    UBX_CFG_ESRCE = 0x60,
    UBX_CFG_GEOFENCE = 0x69,
    UBX_CFG_GNSS = 0x3E,
    UBX_CFG_HNR = 0x5C,
    UBX_CFG_INF = 0x02,
    UBX_CFG_ITFM = 0x39,
    UBX_CFG_LOGFILTER = 0x47,
    UBX_CFG_MSG = 0x01,
    UBX_CFG_NAV5 = 0x24,
    UBX_CFG_NAVX5 = 0x23,
    UBX_CFG_NMEA = 0x17,
    UBX_CFG_ODO = 0x1E,
    UBX_CFG_PM2 = 0x3B,
    UBX_CFG_PMS = 0x86,
    UBX_CFG_PRT = 0x00,
    UBX_CFG_PWR = 0x57,
    UBX_CFG_RATE = 0x08,
    UBX_CFG_RINV = 0x34,
    UBX_CFG_RST = 0x04,
    UBX_CFG_RXM = 0x11,
    UBX_CFG_SBAS = 0x16,
    UBX_CFG_SENIF = 0x88,
    UBX_CFG_SLAS = 0x8D,
    UBX_CFG_SMGR = 0x62,
    UBX_CFG_SPT = 0x64,
    UBX_CFG_TMODE2 = 0x3D,
    UBX_CFG_TMODE3 = 0x71,
    UBX_CFG_TP5 = 0x31,
    UBX_CFG_TXSLOT = 0x53,
    UBX_CFG_USB = 0x1B,
};

enum ubx_external_sensor_fusion_messages {
    UBX_ESF_ALG = 0x14,
    UBX_ESF_INS = 0x15,
    UBX_ESF_MEAS = 0x02,
    UBX_ESF_RAW = 0x03,
    UBX_ESF_STATUS = 0x10,
};

enum ubx_high_rate_navigation_results_messages {
    UBX_HNR_ATT = 0x01,
    UBX_HNR_INS = 0x02,
    UBX_HNR_PVT = 0x00,
};

enum ubx_information_messages {
    UBX_INF_DEBUG = 0x04,
    UBX_INF_ERROR = 0x00,
    UBX_INF_NOTICE = 0x02,
    UBX_INF_TEST = 0x03,
    UBX_INF_WARNING = 0x01,
};

enum ubx_logging_messages {
    UBX_LOG_BATCH = 0x11,
    UBX_LOG_CREATE = 0x07,
    UBX_LOG_ERASE = 0x03,
    UBX_LOG_FINDTIME = 0x0E,
    UBX_LOG_INFO = 0x08,
    UBX_LOG_RETRIEVEBATCH = 0x10,
    UBX_LOG_RETRIEVEPOSEXTRA = 0x0f,
    UBX_LOG_RETRIEVEPOS = 0x0b,
    UBX_LOG_RETRIEVESTRING = 0x0d,
    UBX_LOG_RETRIEVE = 0x09,
    UBX_LOG_STRING = 0x04,
};

enum ubx_multiple_gnss_assistance_messages {
    UBX_MGA_ACK = 0x60,
    UBX_MGA_ANO = 0x20,
    UBX_MGA_BDS = 0x03,
    UBX_MGA_DBD = 0x80,
    UBX_MGA_FLASH = 0x21,
    UBX_MGA_GAL = 0x02,
    UBX_MGA_GLO = 0x06,
    UBX_MGA_GPS = 0x00,
    UBX_MGA_INI = 0x40,
    UBX_MGA_QZSS = 0x05,
};

enum ubx_monitoring_messages {
    UBX_MON_BATCH = 0x32,
    UBX_MON_GNSS = 0x28,
    UBX_MON_HW2 = 0x0B,
    UBX_MON_HW = 0x09,
    UBX_MON_IO = 0x02,
    UBX_MON_MSGPP = 0x06,
    UBX_MON_PATCH = 0x27,
    UBX_MON_RXBUF = 0x07,
    UBX_MON_RXR = 0x21,
    UBX_MON_SMGR = 0x2E,
    UBX_MON_SPT = 0x2F,
    UBX_MON_TXBUF = 0x08,
    UBX_MON_VER = 0x04,
};

enum ubx_nagivation_results_messages {
    UBX_NAV_AOPSTATUS = 0x60,
    UBX_NAV_ATT = 0x05,
    UBX_NAV_CLOCK = 0x22,
    UBX_NAV_COV = 0x36,
    UBX_NAV_DGPS = 0x31,
    UBX_NAV_DOP = 0x04,
    UBX_NAV_EELL = 0x3d,
    UBX_NAV_EOE = 0x61,
    UBX_NAV_GEOFENCE = 0x39,
    UBX_NAV_HPPOSECEF = 0x13,
    UBX_NAV_HPPOSLLH = 0x14,
    UBX_NAV_NMI = 0x28,
    UBX_NAV_ODO = 0x09,
    UBX_NAV_ORB = 0x34,
    UBX_NAV_POSECEF = 0x01,
    UBX_NAV_POSLLH = 0x02,
    UBX_NAV_PVT = 0x07,
    UBX_NAV_RELPOSNED = 0x3C,
    UBX_NAV_RESETODO = 0x10,
    UBX_NAV_SAT = 0x35,
    UBX_NAV_SBAS = 0x32,
    UBX_NAV_SLAS = 0x42,
    UBX_NAV_SOL = 0x06,
    UBX_NAV_STATUS = 0x03,
    UBX_NAV_SVINFO = 0x30,
    UBX_NAV_SVIN = 0x3B,
    UBX_NAV_TIMEBDS = 0x24,
    UBX_NAV_TIMEGAL = 0x25,
    UBX_NAV_TIMEGLO = 0x23,
    UBX_NAV_TIMEGPS = 0x20,
    UBX_NAV_TIMELS = 0x26,
    UBX_NAV_TIMEUTC = 0x21,
    UBX_NAV_VELECEF = 0x11,
    UBX_NAV_VELNED = 0x12,
};

enum ubx_receiver_manager_messages {
    UBX_RXM_IMES = 0x61,
    UBX_RXM_MEASX = 0x14,
    UBX_RXM_PMREQ = 0x41,
    UBX_RXM_RAWX = 0x15,
    UBX_RXM_RLM = 0x59,
    UBX_RXM_RTCM = 0x32,
    UBX_RXM_SFRBX = 0x13,
};

enum ubx_timing_messages {
    UBX_TIM_DOSC = 0x11,
    UBX_TIM_FCHG = 0x16,
    UBX_TIM_HOC = 0x17,
    UBX_TIM_SMEAS = 0x13,
    UBX_TIM_SVIN = 0x04,
    UBX_TIM_TM2 = 0x03,
    UBX_TIM_TOS = 0x12,
    UBX_TIM_TP = 0x01,
    UBX_TIM_VCOCAL = 0x15,
    UBX_TIM_VRFY = 0x06,
};

enum nmea_message_ids {
    NMEA_DTM = 0x0A,
    NMEA_GBQ = 0x44,
    NMEA_GBS = 0x09,
    NMEA_GGA = 0x00,
    NMEA_GLL = 0x01,
    NMEA_GLQ = 0x43,
    NMEA_GNQ = 0x42,
    NMEA_GNS = 0x0D,
    NMEA_GPQ = 0x40,
    NMEA_GRS = 0x06,
    NMEA_GSA = 0x02,
    NMEA_GST = 0x07,
    NMEA_GSV = 0x03,
    NMEA_RMC = 0x04,
    NMEA_THS = 0x0E,
    NMEA_TXT = 0x41,
    NMEA_VLW = 0x0F,
    NMEA_VTG = 0x05,
    NMEA_ZDA = 0x08,
};

#endif /*ZEPHYR_INCLUDE_DRIVERS_GNSS_UBLOX_NEO_M9N_DEFINES_H_*/
