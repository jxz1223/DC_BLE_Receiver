/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pc_protocol.c
  * @brief   Binary protocol between BLE receiver and PC host.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "pc_protocol.h"

#include <string.h>

#include "app_conf.h"
#include "hw_if.h"
#include "stm32_seq.h"

#define PC_PROTOCOL_MAGIC_0              0xA5U
#define PC_PROTOCOL_MAGIC_1              0x5AU
#define PC_PROTOCOL_HEADER_LEN           8U
#define PC_PROTOCOL_FRAME_OVERHEAD       (2U + PC_PROTOCOL_HEADER_LEN + 2U)
#define PC_PROTOCOL_RX_QUEUE_SIZE        512U

typedef enum
{
  PC_RX_WAIT_MAGIC_0,
  PC_RX_WAIT_MAGIC_1,
  PC_RX_HEADER,
  PC_RX_PAYLOAD,
  PC_RX_CRC_LOW,
  PC_RX_CRC_HIGH
} PC_Protocol_RxState_t;

static uint8_t RxByte;
static volatile uint16_t RxHead;
static volatile uint16_t RxTail;
static volatile uint8_t RxOverflow;
static uint8_t RxQueue[PC_PROTOCOL_RX_QUEUE_SIZE];
static PC_Protocol_CommandHandler_t CommandHandler;

static PC_Protocol_RxState_t ParserState;
static uint8_t Header[PC_PROTOCOL_HEADER_LEN];
static uint8_t HeaderIndex;
static PC_Protocol_Frame_t CurrentFrame;
static uint16_t PayloadIndex;
static uint16_t RunningCrc;
static uint8_t ReceivedCrcLow;
static uint8_t TxBuffer[PC_PROTOCOL_FRAME_OVERHEAD + PC_PROTOCOL_MAX_PAYLOAD];

static void PC_Protocol_RxCpltCallback(void);
static void PC_Protocol_ParseByte(uint8_t byte);
static void PC_Protocol_ResetParser(void);
static void PC_Protocol_HandleFrame(const PC_Protocol_Frame_t *pFrame);
static void PC_Protocol_Crc16Update(uint8_t byte);
static uint8_t PC_Protocol_RxPop(uint8_t *pByte);

void PC_Protocol_Init(void)
{
  RxHead = 0U;
  RxTail = 0U;
  RxOverflow = 0U;
  CommandHandler = 0;
  PC_Protocol_ResetParser();

  UTIL_SEQ_RegTask(1U << CFG_TASK_PC_UART_RX_ID, UTIL_SEQ_RFU, PC_Protocol_ProcessRxTask);
  HW_UART_Receive_IT(hw_uart1, &RxByte, 1U, PC_Protocol_RxCpltCallback);
}

void PC_Protocol_RegisterCommandHandler(PC_Protocol_CommandHandler_t handler)
{
  CommandHandler = handler;
}

void PC_Protocol_ProcessRxTask(void)
{
  uint8_t byte;

  if (RxOverflow != 0U)
  {
    RxOverflow = 0U;
    (void)PC_Protocol_SendNack(0U, 0U, 0U, PC_PROTOCOL_STATUS_RX_OVERFLOW);
  }

  while (PC_Protocol_RxPop(&byte) != 0U)
  {
    PC_Protocol_ParseByte(byte);
  }
}

uint8_t PC_Protocol_SendFrame(uint8_t type,
                              uint8_t flags,
                              uint16_t scan_id,
                              uint16_t seq,
                              const uint8_t *payload,
                              uint8_t payload_len)
{
  uint16_t crc;
  uint16_t index = 0U;
  hw_status_t status;

  if ((payload_len > PC_PROTOCOL_MAX_PAYLOAD) ||
      ((payload_len > 0U) && (payload == 0)))
  {
    return PC_PROTOCOL_STATUS_BAD_LENGTH;
  }

  TxBuffer[index++] = PC_PROTOCOL_MAGIC_0;
  TxBuffer[index++] = PC_PROTOCOL_MAGIC_1;
  TxBuffer[index++] = PC_PROTOCOL_VERSION;
  TxBuffer[index++] = type;
  TxBuffer[index++] = flags;
  TxBuffer[index++] = (uint8_t)(scan_id);
  TxBuffer[index++] = (uint8_t)(scan_id >> 8);
  TxBuffer[index++] = (uint8_t)(seq);
  TxBuffer[index++] = (uint8_t)(seq >> 8);
  TxBuffer[index++] = payload_len;

  if (payload_len > 0U)
  {
    memcpy(&TxBuffer[index], payload, payload_len);
    index = (uint16_t)(index + payload_len);
  }

  crc = PC_Protocol_Crc16Ccitt(&TxBuffer[2], (uint16_t)(PC_PROTOCOL_HEADER_LEN + payload_len));
  TxBuffer[index++] = (uint8_t)(crc);
  TxBuffer[index++] = (uint8_t)(crc >> 8);

  status = HW_UART_Transmit(hw_uart1, TxBuffer, index, 1000U);
  return (status == hw_uart_ok) ? PC_PROTOCOL_STATUS_OK : PC_PROTOCOL_STATUS_BUSY;
}

uint8_t PC_Protocol_SendAck(uint8_t acked_type, uint16_t scan_id, uint16_t seq, uint8_t status)
{
  uint8_t payload[2];
  payload[0] = acked_type;
  payload[1] = status;
  return PC_Protocol_SendFrame(PC_PROTOCOL_EVT_ACK, 0U, scan_id, seq, payload, sizeof(payload));
}

uint8_t PC_Protocol_SendNack(uint8_t rejected_type, uint16_t scan_id, uint16_t seq, uint8_t status)
{
  uint8_t payload[2];
  payload[0] = rejected_type;
  payload[1] = status;
  return PC_Protocol_SendFrame(PC_PROTOCOL_EVT_NACK, 0U, scan_id, seq, payload, sizeof(payload));
}

uint16_t PC_Protocol_Crc16Ccitt(const uint8_t *data, uint16_t length)
{
  uint16_t crc = 0xFFFFU;
  uint16_t i;
  uint8_t bit;

  for (i = 0U; i < length; i++)
  {
    crc ^= (uint16_t)data[i] << 8;
    for (bit = 0U; bit < 8U; bit++)
    {
      if ((crc & 0x8000U) != 0U)
      {
        crc = (uint16_t)((crc << 1) ^ 0x1021U);
      }
      else
      {
        crc = (uint16_t)(crc << 1);
      }
    }
  }

  return crc;
}

static void PC_Protocol_RxCpltCallback(void)
{
  uint16_t next_head = (uint16_t)((RxHead + 1U) % PC_PROTOCOL_RX_QUEUE_SIZE);

  if (next_head == RxTail)
  {
    RxOverflow = 1U;
  }
  else
  {
    RxQueue[RxHead] = RxByte;
    RxHead = next_head;
  }

  HW_UART_Receive_IT(hw_uart1, &RxByte, 1U, PC_Protocol_RxCpltCallback);
  UTIL_SEQ_SetTask(1U << CFG_TASK_PC_UART_RX_ID, CFG_SCH_PRIO_0);
}

static uint8_t PC_Protocol_RxPop(uint8_t *pByte)
{
  if (RxTail == RxHead)
  {
    return 0U;
  }

  *pByte = RxQueue[RxTail];
  RxTail = (uint16_t)((RxTail + 1U) % PC_PROTOCOL_RX_QUEUE_SIZE);
  return 1U;
}

static void PC_Protocol_ParseByte(uint8_t byte)
{
  uint16_t received_crc;

  switch (ParserState)
  {
    case PC_RX_WAIT_MAGIC_0:
      if (byte == PC_PROTOCOL_MAGIC_0)
      {
        ParserState = PC_RX_WAIT_MAGIC_1;
      }
      break;

    case PC_RX_WAIT_MAGIC_1:
      if (byte == PC_PROTOCOL_MAGIC_1)
      {
        HeaderIndex = 0U;
        RunningCrc = 0xFFFFU;
        ParserState = PC_RX_HEADER;
      }
      else
      {
        ParserState = (byte == PC_PROTOCOL_MAGIC_0) ? PC_RX_WAIT_MAGIC_1 : PC_RX_WAIT_MAGIC_0;
      }
      break;

    case PC_RX_HEADER:
      Header[HeaderIndex++] = byte;
      PC_Protocol_Crc16Update(byte);
      if (HeaderIndex >= PC_PROTOCOL_HEADER_LEN)
      {
        CurrentFrame.Type = Header[1];
        CurrentFrame.Flags = Header[2];
        CurrentFrame.ScanId = (uint16_t)Header[3] | ((uint16_t)Header[4] << 8);
        CurrentFrame.Seq = (uint16_t)Header[5] | ((uint16_t)Header[6] << 8);
        CurrentFrame.PayloadLen = Header[7];
        PayloadIndex = 0U;

        if (CurrentFrame.PayloadLen > PC_PROTOCOL_MAX_PAYLOAD)
        {
          (void)PC_Protocol_SendNack(CurrentFrame.Type,
                                     CurrentFrame.ScanId,
                                     CurrentFrame.Seq,
                                     PC_PROTOCOL_STATUS_BAD_LENGTH);
          PC_Protocol_ResetParser();
        }
        else if (CurrentFrame.PayloadLen == 0U)
        {
          ParserState = PC_RX_CRC_LOW;
        }
        else
        {
          ParserState = PC_RX_PAYLOAD;
        }
      }
      break;

    case PC_RX_PAYLOAD:
      CurrentFrame.Payload[PayloadIndex++] = byte;
      PC_Protocol_Crc16Update(byte);
      if (PayloadIndex >= CurrentFrame.PayloadLen)
      {
        ParserState = PC_RX_CRC_LOW;
      }
      break;

    case PC_RX_CRC_LOW:
      ReceivedCrcLow = byte;
      ParserState = PC_RX_CRC_HIGH;
      break;

    case PC_RX_CRC_HIGH:
      received_crc = (uint16_t)ReceivedCrcLow | ((uint16_t)byte << 8);
      if (received_crc != RunningCrc)
      {
        (void)PC_Protocol_SendNack(CurrentFrame.Type,
                                   CurrentFrame.ScanId,
                                   CurrentFrame.Seq,
                                   PC_PROTOCOL_STATUS_CRC_ERROR);
      }
      else if (Header[0] != PC_PROTOCOL_VERSION)
      {
        (void)PC_Protocol_SendNack(CurrentFrame.Type,
                                   CurrentFrame.ScanId,
                                   CurrentFrame.Seq,
                                   PC_PROTOCOL_STATUS_BAD_VERSION);
      }
      else
      {
        PC_Protocol_HandleFrame(&CurrentFrame);
      }
      PC_Protocol_ResetParser();
      break;

    default:
      PC_Protocol_ResetParser();
      break;
  }
}

static void PC_Protocol_ResetParser(void)
{
  ParserState = PC_RX_WAIT_MAGIC_0;
  HeaderIndex = 0U;
  PayloadIndex = 0U;
  RunningCrc = 0xFFFFU;
  ReceivedCrcLow = 0U;
}

static void PC_Protocol_HandleFrame(const PC_Protocol_Frame_t *pFrame)
{
  uint8_t status;

  switch (pFrame->Type)
  {
    case PC_PROTOCOL_CMD_SCAN_START:
    case PC_PROTOCOL_CMD_SET_DAC:
    case PC_PROTOCOL_CMD_ABORT:
    case PC_PROTOCOL_CMD_PD2_PULSE:
      status = (CommandHandler != 0) ? CommandHandler(pFrame) : PC_PROTOCOL_STATUS_OK;
      if (status == PC_PROTOCOL_STATUS_DEFERRED)
      {
        /* Response will be generated by the downstream BLE sensor. */
      }
      else if (status == PC_PROTOCOL_STATUS_OK)
      {
        (void)PC_Protocol_SendAck(pFrame->Type, pFrame->ScanId, pFrame->Seq, status);
      }
      else
      {
        (void)PC_Protocol_SendNack(pFrame->Type, pFrame->ScanId, pFrame->Seq, status);
      }
      break;

    default:
      (void)PC_Protocol_SendNack(pFrame->Type,
                                 pFrame->ScanId,
                                 pFrame->Seq,
                                 PC_PROTOCOL_STATUS_UNKNOWN_TYPE);
      break;
  }
}

static void PC_Protocol_Crc16Update(uint8_t byte)
{
  uint8_t bit;

  RunningCrc ^= (uint16_t)byte << 8;
  for (bit = 0U; bit < 8U; bit++)
  {
    if ((RunningCrc & 0x8000U) != 0U)
    {
      RunningCrc = (uint16_t)((RunningCrc << 1) ^ 0x1021U);
    }
    else
    {
      RunningCrc = (uint16_t)(RunningCrc << 1);
    }
  }
}
