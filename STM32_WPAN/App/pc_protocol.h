/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pc_protocol.h
  * @brief   Binary protocol between BLE receiver and PC host.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef PC_PROTOCOL_H
#define PC_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PC_PROTOCOL_VERSION              0x01U
#define PC_PROTOCOL_MAX_PAYLOAD          246U

#define PC_PROTOCOL_CMD_SCAN_START       0x10U
#define PC_PROTOCOL_CMD_SET_DAC          0x11U
#define PC_PROTOCOL_CMD_ABORT            0x12U
#define PC_PROTOCOL_CMD_PD2_PULSE        0x13U

#define PC_PROTOCOL_EVT_ACK              0x80U
#define PC_PROTOCOL_EVT_NACK             0x81U
#define PC_PROTOCOL_EVT_SCAN_BEGIN       0x90U
#define PC_PROTOCOL_EVT_SCAN_POINTS      0x91U
#define PC_PROTOCOL_EVT_SCAN_END         0x92U
#define PC_PROTOCOL_EVT_SENSOR_READINGS  0x93U
#define PC_PROTOCOL_EVT_BLE_RAW          0xA0U

#define PC_PROTOCOL_STATUS_OK            0x00U
#define PC_PROTOCOL_STATUS_CRC_ERROR     0x01U
#define PC_PROTOCOL_STATUS_BAD_VERSION   0x02U
#define PC_PROTOCOL_STATUS_BAD_LENGTH    0x03U
#define PC_PROTOCOL_STATUS_UNKNOWN_TYPE  0x04U
#define PC_PROTOCOL_STATUS_NO_LINK       0x05U
#define PC_PROTOCOL_STATUS_BUSY          0x06U
#define PC_PROTOCOL_STATUS_UNSUPPORTED   0x07U
#define PC_PROTOCOL_STATUS_RX_OVERFLOW   0x08U
#define PC_PROTOCOL_STATUS_DEFERRED      0xFFU

typedef struct
{
  uint8_t Type;
  uint8_t Flags;
  uint16_t ScanId;
  uint16_t Seq;
  uint8_t PayloadLen;
  uint8_t Payload[PC_PROTOCOL_MAX_PAYLOAD];
} PC_Protocol_Frame_t;

typedef uint8_t (*PC_Protocol_CommandHandler_t)(const PC_Protocol_Frame_t *pFrame);

void PC_Protocol_Init(void);
void PC_Protocol_RegisterCommandHandler(PC_Protocol_CommandHandler_t handler);
void PC_Protocol_ProcessRxTask(void);
uint8_t PC_Protocol_SendFrame(uint8_t type,
                              uint8_t flags,
                              uint16_t scan_id,
                              uint16_t seq,
                              const uint8_t *payload,
                              uint8_t payload_len);
uint8_t PC_Protocol_SendAck(uint8_t acked_type, uint16_t scan_id, uint16_t seq, uint8_t status);
uint8_t PC_Protocol_SendNack(uint8_t rejected_type, uint16_t scan_id, uint16_t seq, uint8_t status);
uint16_t PC_Protocol_Crc16Ccitt(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* PC_PROTOCOL_H */
