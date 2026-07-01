/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/p2p_client_app.c
  * @author  MCD Application Team
  * @brief   Peer to peer Client Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "app_common.h"

#include "dbg_trace.h"

#include "ble.h"
#include "p2p_client_app.h"

#include "stm32_seq.h"
#include "app_ble.h"
#include "pc_protocol.h"
#include "hw_if.h"

/* USER CODE BEGIN Includes */
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

typedef enum
{
  P2P_START_TIMER_EVT,
  P2P_STOP_TIMER_EVT,
  P2P_NOTIFICATION_INFO_RECEIVED_EVT,
} P2P_Client_Opcode_Notification_evt_t;

typedef struct
{
  uint8_t * pPayload;
  uint8_t     Length;
}P2P_Client_Data_t;

typedef struct
{
  P2P_Client_Opcode_Notification_evt_t  P2P_Client_Evt_Opcode;
  P2P_Client_Data_t DataTransfered;
}P2P_Client_App_Notification_evt_t;

typedef struct
{
  /**
   * state of the P2P Client
   * state machine
   */
  APP_BLE_ConnStatus_t state;

  /**
   * connection handle
   */
  uint16_t connHandle;

  /**
   * handle of the P2P service
   */
  uint16_t P2PServiceHandle;

  /**
   * end handle of the P2P service
   */
  uint16_t P2PServiceEndHandle;

  /**
   * handle of the Tx characteristic - Write To Server
   *
   */
  uint16_t P2PWriteToServerCharHdle;

  /**
   * handle of the client configuration
   * descriptor of Tx characteristic
   */
  uint16_t P2PWriteToServerDescHandle;

  /**
   * handle of the Rx characteristic - Notification From Server
   *
   */
  uint16_t P2PNotificationCharHdle;

  /**
   * handle of the client configuration
   * descriptor of Rx characteristic
   */
  uint16_t P2PNotificationDescHandle;

}P2P_ClientContext_t;

/* USER CODE BEGIN PTD */
extern UART_HandleTypeDef huart1;
uint8_t dtcc=0;

typedef struct{
  uint8_t                                     Device_Button_Selection;
  uint8_t                                     Button1;
}P2P_ButtonCharValue_t;

static P2P_ButtonCharValue_t      ButtonStatus;
/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PC_BLE_FRAME_MAX_SIZE                 246U
#define RX_LINK_BLUE_BLINK_MS                 250U

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
#define UNPACK_2_BYTE_PARAMETER(ptr)  \
        (uint16_t)((uint16_t)(*((uint8_t *)ptr))) |   \
        (uint16_t)((((uint16_t)(*((uint8_t *)ptr + 1))) << 8))
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

static P2P_ClientContext_t aP2PClientContext[BLE_CFG_CLT_MAX_NBR_CB];

/**
 * END of Section BLE_APP_CONTEXT
 */
/* USER CODE BEGIN PV */
uint8_t DTCTIME = 0;
#define LEDn                                    3
GPIO_TypeDef* GPIO_PORT[LEDn] = {Blue_Led_GPIO_Port, Green_Led_GPIO_Port, Red_Led_GPIO_Port};
const uint16_t GPIO_PIN[LEDn] = {Blue_Led_Pin, Green_Led_Pin, Red_Led_Pin};
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
void Connagain(void);
void Button1_Trigger_Received(void);
void Button2_Trigger_Received(void);
void Button3_Trigger_Received(void);
static tBleStatus Write_Char(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload);
static uint8_t Pc_Command_Received(const PC_Protocol_Frame_t *pFrame);
static void Pc_SendDiag(const char *text);
static tBleStatus Write_Char_With_Length(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload, uint8_t payload_length);
static uint8_t Pc_ForwardCommandToSensor(const PC_Protocol_Frame_t *pFrame);
static uint8_t Pc_BuildWireFrame(const PC_Protocol_Frame_t *pFrame, uint8_t *pBuffer, uint16_t buffer_length, uint8_t *pFrameLength);
static uint8_t Pc_IsWireFrame(const uint8_t *pPayload, uint8_t length);
static void Button_SendPd2PulseCommand(void);
static uint8_t PcBleFrame[PC_BLE_FRAME_MAX_SIZE];
static uint16_t ButtonCommandSeq = 1U;
static uint8_t LinkBlueBlinkTimerId;
static uint8_t LinkBlueBlinkTimerReady = 0U;
static uint8_t LinkBlueBlinkEnabled = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void Gatt_Notification(P2P_Client_App_Notification_evt_t *pNotification);
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event);
/* USER CODE BEGIN PFP */
static void LinkBlueBlink_Start(void);
static void LinkBlueBlink_Stop(void);
static void LinkBlueBlink_Timeout(void);
static uint32_t MsToTsTicks(uint32_t timeout_ms);

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void P2PC_APP_Init(void)
{
  uint8_t index =0;
/* USER CODE BEGIN P2PC_APP_Init_1 */
  ButtonStatus.Device_Button_Selection=0x00;/* Device1 */
  ButtonStatus.Button1=0x00;
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW1_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button1_Trigger_Received );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW2_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button2_Trigger_Received );
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW3_BUTTON_PUSHED_ID, UTIL_SEQ_RFU, Button3_Trigger_Received );
  PC_Protocol_Init();
  PC_Protocol_RegisterCommandHandler(Pc_Command_Received);
  if (HW_TS_Create(CFG_TIM_PROC_ID_ISR,
                   &LinkBlueBlinkTimerId,
                   hw_ts_Repeated,
                   LinkBlueBlink_Timeout) == hw_ts_Successful)
  {
    LinkBlueBlinkTimerReady = 1U;
  }
/* USER CODE END P2PC_APP_Init_1 */
  for(index = 0; index < BLE_CFG_CLT_MAX_NBR_CB; index++)
  {
    aP2PClientContext[index].state= APP_BLE_IDLE;
  }

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterCltHandler(Event_Handler);

#if(CFG_DEBUG_APP_TRACE != 0)
  APP_DBG_MSG("-- P2P CLIENT INITIALIZED \n");
#endif

/* USER CODE BEGIN P2PC_APP_Init_2 */

/* USER CODE END P2PC_APP_Init_2 */
  return;
}

void P2PC_APP_Notification(P2PC_APP_ConnHandle_Not_evt_t *pNotification)
{
/* USER CODE BEGIN P2PC_APP_Notification_1 */

/* USER CODE END P2PC_APP_Notification_1 */
  switch(pNotification->P2P_Evt_Opcode)
  {
/* USER CODE BEGIN P2P_Evt_Opcode */

/* USER CODE END P2P_Evt_Opcode */

  case PEER_CONN_HANDLE_EVT :
/* USER CODE BEGIN PEER_CONN_HANDLE_EVT */
      LED_On(GREEN);
      LED_Off(RED);
      LinkBlueBlink_Start();
      Pc_SendDiag("RX_BLE_CONNECTED");
/* USER CODE END PEER_CONN_HANDLE_EVT */
      break;

    case PEER_DISCON_HANDLE_EVT :
/* USER CODE BEGIN PEER_DISCON_HANDLE_EVT */
      LinkBlueBlink_Stop();
      LED_Off(GREEN);
      LED_On(RED);
      Connagain();
      Pc_SendDiag("RX_BLE_DISCONNECTED");
/* USER CODE END PEER_DISCON_HANDLE_EVT */
      break;

    default:
/* USER CODE BEGIN P2P_Evt_Opcode_Default */

/* USER CODE END P2P_Evt_Opcode_Default */
      break;
  }
/* USER CODE BEGIN P2PC_APP_Notification_2 */

/* USER CODE END P2PC_APP_Notification_2 */
  return;
}
/* USER CODE BEGIN FD */

/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *event_pckt;
  evt_blecore_aci *blecore_evt;

  P2P_Client_App_Notification_evt_t Notification;

  return_value = SVCCTL_EvtNotAck;
  event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)Event)->data);

  switch(event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
    {
      blecore_evt = (evt_blecore_aci*)event_pckt->data;
      switch(blecore_evt->ecode)
      {

        case ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE:
        {
          aci_att_read_by_group_type_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t numServ, i, idx;
          uint16_t uuid, handle;

          uint8_t index;
          handle = pr->Connection_Handle;
          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].state != APP_BLE_IDLE))
          {
            APP_BLE_ConnStatus_t status;

            status = APP_BLE_Get_Client_Connection_Status(aP2PClientContext[index].connHandle);

            if((aP2PClientContext[index].state == APP_BLE_CONNECTED_CLIENT)&&
                    (status == APP_BLE_IDLE))
            {
              /* Handle deconnected */

              aP2PClientContext[index].state = APP_BLE_IDLE;
              aP2PClientContext[index].connHandle = 0xFFFF;
              break;
            }
            index++;
          }

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {
            aP2PClientContext[index].connHandle= handle;

            numServ = (pr->Data_Length) / pr->Attribute_Data_Length;

            /* the event data will be
             * 2bytes start handle
             * 2bytes end handle
             * 2 or 16 bytes data
             * we are interested only if the UUID is 16 bit.
             * So check if the data length is 6
             */
#if (UUID_128BIT_FORMAT==1)
          if (pr->Attribute_Data_Length == 20)
          {
            idx = 16;
#else
          if (pr->Attribute_Data_Length == 6)
          {
            idx = 4;
#endif
              for (i=0; i<numServ; i++)
              {
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx]);
                if(uuid == DT_SERVICE_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : P2P_SERVICE_UUID FOUND - connection handle 0x%x \n", aP2PClientContext[index].connHandle);
#endif
                  Pc_SendDiag("RX_DT_SERVICE_FOUND");
#if (UUID_128BIT_FORMAT==1)
                aP2PClientContext[index].P2PServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx-16]);
                aP2PClientContext[index].P2PServiceEndHandle = UNPACK_2_BYTE_PARAMETER (&pr->Attribute_Data_List[idx-14]);
#else
                aP2PClientContext[index].P2PServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx-4]);
                aP2PClientContext[index].P2PServiceEndHandle = UNPACK_2_BYTE_PARAMETER (&pr->Attribute_Data_List[idx-2]);
#endif
                  aP2PClientContext[index].state = APP_BLE_DISCOVER_CHARACS ;
                }
                idx += 20;
              }
            }
          }
        }
        break;

        case ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE:
        {

          aci_att_read_by_type_resp_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t idx;
          uint16_t uuid, handle;

          /* the event data will be
           * 2 bytes start handle
           * 1 byte char properties
           * 2 bytes handle
           * 2 or 16 bytes data
           */

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            /* we are interested in only 16 bit UUIDs */
#if (UUID_128BIT_FORMAT==1)
            idx = 17;
            if (pr->Handle_Value_Pair_Length == 21)
#else
              idx = 5;
            if (pr->Handle_Value_Pair_Length == 7)
#endif
            {
              pr->Data_Length -= 1;
              while(pr->Data_Length > 0)
              {
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx]);
                /* store the characteristic handle not the attribute handle */
#if (UUID_128BIT_FORMAT==1)
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx-14]);
#else
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx-2]);
#endif
                if(uuid == DT_RX_CHAR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : DT_RX_CHAR_UUID FOUND - connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  Pc_SendDiag("RX_DT_RX_FOUND");
                  //aP2PClientContext[index].state = APP_BLE_DISCOVER_WRITE_DESC;
                  aP2PClientContext[index].P2PWriteToServerCharHdle = handle;
                }

                if(uuid == DT_TX_CHAR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : NOTIFICATION_CHAR_UUID FOUND  - connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  Pc_SendDiag("RX_DT_TX_FOUND");
                  aP2PClientContext[index].state = APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC;
                  aP2PClientContext[index].P2PNotificationCharHdle = handle;
                }
#if (UUID_128BIT_FORMAT==1)
                pr->Data_Length -= 21;
                idx += 21;
#else
                pr->Data_Length -= 7;
                idx += 7;
#endif
              }
            }
          }
        }
        break;

        case ACI_ATT_FIND_INFO_RESP_VSEVT_CODE:
        {
          aci_att_find_info_resp_event_rp0 *pr = (void*)blecore_evt->data;

          uint8_t numDesc, idx, i;
          uint16_t uuid, handle;

          /*
           * event data will be of the format
           * 2 bytes handle
           * 2 bytes UUID
           */

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))

            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            numDesc = (pr->Event_Data_Length) / 4;
            /* we are interested only in 16 bit UUIDs */
            idx = 0;
            if (pr->Format == UUID_TYPE_16)
            {
              for (i=0; i<numDesc; i++)
              {
                handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx]);
                uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx+2]);

                if(uuid == CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID)
                {
#if(CFG_DEBUG_APP_TRACE != 0)
                  APP_DBG_MSG("-- GATT : CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID- connection handle 0x%x\n", aP2PClientContext[index].connHandle);
#endif
                  if( aP2PClientContext[index].state == APP_BLE_DISCOVER_NOTIFICATION_CHAR_DESC)
                  {

                    aP2PClientContext[index].P2PNotificationDescHandle = handle;
                    aP2PClientContext[index].state = APP_BLE_ENABLE_NOTIFICATION_DESC;
                    Pc_SendDiag("RX_NOTIFY_DESC_FOUND");

                  }
                }
                idx += 4;
              }
            }
          }
        }
        break; /*ACI_ATT_FIND_INFO_RESP_VSEVT_CODE*/

        case ACI_GATT_NOTIFICATION_VSEVT_CODE:
        {
          aci_gatt_notification_event_rp0 *pr = (void*)blecore_evt->data;
          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {
            {

              Notification.P2P_Client_Evt_Opcode = P2P_NOTIFICATION_INFO_RECEIVED_EVT;
              Notification.DataTransfered.Length = pr->Attribute_Value_Length;
              Notification.DataTransfered.pPayload = &pr->Attribute_Value[0];

              Gatt_Notification(&Notification);

              /* INFORM APPLICATION BUTTON IS PUSHED BY END DEVICE */

            }
          }
        }
        break;/* end ACI_GATT_NOTIFICATION_VSEVT_CODE */

        case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
        {
          aci_gatt_proc_complete_event_rp0 *pr = (void*)blecore_evt->data;
#if(CFG_DEBUG_APP_TRACE != 0)
          APP_DBG_MSG("-- GATT : ACI_GATT_PROC_COMPLETE_VSEVT_CODE \n");
          APP_DBG_MSG("\n");
#endif

          uint8_t index;

          index = 0;
          while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
                  (aP2PClientContext[index].connHandle != pr->Connection_Handle))
            index++;

          if(index < BLE_CFG_CLT_MAX_NBR_CB)
          {

            UTIL_SEQ_SetTask( 1<<CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0);

          }
        }
        break; /*ACI_GATT_PROC_COMPLETE_VSEVT_CODE*/
        default:
          break;
      }
    }

    break; /* HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE */

    default:
      break;
  }

  return(return_value);
}/* end BLE_CTRL_Event_Acknowledged_Status_t */

void Gatt_Notification(P2P_Client_App_Notification_evt_t *pNotification)
{
/* USER CODE BEGIN Gatt_Notification_1*/

/* USER CODE END Gatt_Notification_1 */
  switch(pNotification->P2P_Client_Evt_Opcode)
  {
/* USER CODE BEGIN P2P_Client_Evt_Opcode */

/* USER CODE END P2P_Client_Evt_Opcode */

    case P2P_NOTIFICATION_INFO_RECEIVED_EVT:
/* USER CODE BEGIN P2P_NOTIFICATION_INFO_RECEIVED_EVT */
    	dtcc=dtcc+1;
      if (dtcc > 40)
      {
        dtcc = 0;
      }
		if (Pc_IsWireFrame(pNotification->DataTransfered.pPayload,
							pNotification->DataTransfered.Length) != 0U)
		{
			(void)HW_UART_Transmit(hw_uart1,
								   pNotification->DataTransfered.pPayload,
								   pNotification->DataTransfered.Length,
								   1000U);
		}
		else
		{
			(void)PC_Protocol_SendFrame(PC_PROTOCOL_EVT_BLE_RAW,
										0,
										0,
										dtcc,
										pNotification->DataTransfered.pPayload,
										pNotification->DataTransfered.Length);
		}
/* USER CODE END P2P_NOTIFICATION_INFO_RECEIVED_EVT */
      break;

    default:
/* USER CODE BEGIN P2P_Client_Evt_Opcode_Default */

/* USER CODE END P2P_Client_Evt_Opcode_Default */
      break;
  }
/* USER CODE BEGIN Gatt_Notification_2*/

/* USER CODE END Gatt_Notification_2 */
  return;
}

uint8_t P2P_Client_APP_Get_State( void ) {
  return aP2PClientContext[0].state;
}
/* USER CODE BEGIN LF */
static void Pc_SendDiag(const char *text)
{
  uint16_t length;

  if (text == 0)
  {
    return;
  }

  length = (uint16_t)strlen(text);
  if (length > PC_PROTOCOL_MAX_PAYLOAD)
  {
    length = PC_PROTOCOL_MAX_PAYLOAD;
  }

  (void)PC_Protocol_SendFrame(PC_PROTOCOL_EVT_BLE_RAW,
                              0U,
                              0U,
                              0U,
                              (const uint8_t *)text,
                              (uint8_t)length);
}

void DTC_App_LinkReadyNotification(uint16_t ConnectionHandle)
{
	tBleStatus status;
	uint8_t index =0;
	uint8_t doneprj =0;
  aP2PClientContext[index].connHandle = ConnectionHandle;
  if ((DTCTIME == 0)&(doneprj ==0))
  {
	  DTCTIME = 1;
	  doneprj = 1;
	  APP_DBG_MSG("Discover all char of service \n");

	  	status = aci_gatt_disc_all_char_of_service(
	  			aP2PClientContext[index].connHandle,
	  			aP2PClientContext[index].P2PServiceHandle,
	  			aP2PClientContext[index].P2PServiceEndHandle);
	  	if (status != BLE_STATUS_SUCCESS)
	  	{
	  		APP_DBG_MSG("Discover all char of service cmd failure: 0x%x\n", status);
	  	}

	  	APP_DBG_MSG("DTC_DISCOVER_CHARACS complete event received \n");

  }
 // DISCOVER_CHARACS;

  if ((DTCTIME == 1)&(doneprj ==0))
    {
	  DTCTIME = 2;
	  doneprj = 1;
	  APP_DBG_MSG("Discover char descriptors \n");

		status = aci_gatt_disc_all_char_desc(
				aP2PClientContext[index].connHandle,
				aP2PClientContext[index].P2PServiceHandle,
				aP2PClientContext[index].P2PServiceEndHandle);
		if (status != BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("Discover char descriptors cmd failure: 0x%x\n", status);
		}
		APP_DBG_MSG("DTC_DISCOVER_DESC complete event received \n");
    }
  //DISCOVER_DESC;

  if ((DTCTIME == 2)&(doneprj ==0))
    {
	  //ENABLE_TX_NOTIFICATION;
	  	DTCTIME = 3;
	  	doneprj = 1;
		uint8_t notification[2] = {0x01, 0x00};

		APP_DBG_MSG("Enable Tx char descriptors \n");
		status = aci_gatt_write_char_desc(aP2PClientContext[index].connHandle,
																			aP2PClientContext[index].P2PNotificationDescHandle,
																			2,
																			(uint8_t *) &notification[0]);

		if (status != BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("Enable Tx char descriptors cmd failure: 0x%x\n", status);
      Pc_SendDiag("RX_NOTIFY_ENABLE_FAIL");
		}
    else
    {
      Pc_SendDiag("RX_NOTIFY_ENABLE_SENT");
    }
		APP_DBG_MSG("DTC_ENABLE_TX_NOTIFICATION complete event received \n");
    }
  return;
}

void LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}

void LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

void LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

static void LinkBlueBlink_Start(void)
{
  LinkBlueBlinkEnabled = 1U;

  if (LinkBlueBlinkTimerReady != 0U)
  {
    LED_Off(BLUE);
    HW_TS_Start(LinkBlueBlinkTimerId, MsToTsTicks(RX_LINK_BLUE_BLINK_MS));
  }
  else
  {
    LED_On(BLUE);
  }
}

static void LinkBlueBlink_Stop(void)
{
  LinkBlueBlinkEnabled = 0U;

  if (LinkBlueBlinkTimerReady != 0U)
  {
    HW_TS_Stop(LinkBlueBlinkTimerId);
  }

  LED_Off(BLUE);
}

static void LinkBlueBlink_Timeout(void)
{
  if (LinkBlueBlinkEnabled != 0U)
  {
    LED_Toggle(BLUE);
  }
  else
  {
    LED_Off(BLUE);
  }
}

static uint32_t MsToTsTicks(uint32_t timeout_ms)
{
  uint32_t timeout_ticks = ((timeout_ms * 1000U) + CFG_TS_TICK_VAL - 1U) / CFG_TS_TICK_VAL;

  return (timeout_ticks == 0U) ? 1U : timeout_ticks;
}

void Connagain(void)
{
	DTCTIME = 0;
	aP2PClientContext[0].state= APP_BLE_IDLE;
}

void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
  switch (GPIO_Pin)
  {
    case SW1_User_Pin:
        UTIL_SEQ_SetTask(1<<CFG_TASK_SW1_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);
      break;

    case SW2_User_Pin:
        UTIL_SEQ_SetTask(1<<CFG_TASK_SW2_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);
      break;

    case SW3_User_Pin:
        UTIL_SEQ_SetTask(1<<CFG_TASK_SW3_BUTTON_PUSHED_ID, CFG_SCH_PRIO_0);
      break;

    default:
      break;

  }
  return;
}

void Button1_Trigger_Received(void)
{
  APP_DBG_MSG("-- P2P APPLICATION CLIENT  : BUTTON PUSHED - WRITE TO SERVER \n ");
  Button_SendPd2PulseCommand();
  return;
}

void Button2_Trigger_Received(void)
{
  APP_DBG_MSG("-- P2P APPLICATION CLIENT  : BUTTON2 PUSHED - WRITE TO SERVER \n ");
  Button_SendPd2PulseCommand();
  return;
}

void Button3_Trigger_Received(void)
{
  APP_DBG_MSG("-- P2P APPLICATION CLIENT  : BUTTON3 PUSHED - WRITE TO SERVER \n ");
  Button_SendPd2PulseCommand();
  return;
}

/**
 * @brief  Feature Characteristic update
 * @param  pFeatureValue: The address of the new value to be written
 * @retval None
 */
tBleStatus Write_Char(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload)
{
  return Write_Char_With_Length(UUID, Service_Instance, pPayload, 2U);
}/* end Write_Char() */

static tBleStatus Write_Char_With_Length(uint16_t UUID, uint8_t Service_Instance, uint8_t *pPayload, uint8_t payload_length)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  uint8_t index;
  index = 0;
  while((index < BLE_CFG_CLT_MAX_NBR_CB) &&
          (aP2PClientContext[index].state != APP_BLE_IDLE))
  {
    switch(UUID)
    {
      case DT_RX_CHAR_UUID: /* SERVER RX -- so CLIENT TX */
        ret = aci_gatt_write_without_resp(aP2PClientContext[index].connHandle,
                                         aP2PClientContext[index].P2PWriteToServerCharHdle,
                                         payload_length, /* charValueLen */
                                         (uint8_t *)  pPayload);
        break;
      default:
        break;
    }
    index++;
  }

  return ret;
}

static uint8_t Pc_Command_Received(const PC_Protocol_Frame_t *pFrame)
{
  if (pFrame == 0)
  {
    return PC_PROTOCOL_STATUS_BAD_LENGTH;
  }

  if (aP2PClientContext[0].state == APP_BLE_IDLE)
  {
    return PC_PROTOCOL_STATUS_NO_LINK;
  }

  /*
   * Forward the complete PC wire frame to the sensor-side DT_RX_CHAR_UUID.
   * The sensor sends ACK/NACK and scan events back through DT_TX_CHAR_UUID.
   */
  switch (pFrame->Type)
  {
    case PC_PROTOCOL_CMD_SCAN_START:
    case PC_PROTOCOL_CMD_SET_DAC:
    case PC_PROTOCOL_CMD_ABORT:
    case PC_PROTOCOL_CMD_PD2_PULSE:
      return Pc_ForwardCommandToSensor(pFrame);

    default:
      return PC_PROTOCOL_STATUS_UNKNOWN_TYPE;
  }
}

static void Button_SendPd2PulseCommand(void)
{
  PC_Protocol_Frame_t frame;
  uint8_t status;
  uint16_t duration_ms = 5000U;

  memset(&frame, 0, sizeof(frame));
  frame.Type = PC_PROTOCOL_CMD_PD2_PULSE;
  frame.Flags = 0U;
  frame.ScanId = 0U;
  frame.Seq = ButtonCommandSeq++;
  frame.PayloadLen = 2U;
  frame.Payload[0] = (uint8_t)duration_ms;
  frame.Payload[1] = (uint8_t)(duration_ms >> 8);

  status = Pc_ForwardCommandToSensor(&frame);
  if (status == PC_PROTOCOL_STATUS_DEFERRED)
  {
    LED_Toggle(GREEN);
    Pc_SendDiag("RX_BUTTON_PD2_PULSE_SENT");
  }
  else
  {
    LED_Toggle(RED);
    Pc_SendDiag("RX_BUTTON_PD2_PULSE_FAIL");
  }
}

static uint8_t Pc_ForwardCommandToSensor(const PC_Protocol_Frame_t *pFrame)
{
  uint8_t frame_length = 0U;
  tBleStatus ble_status;

  if (pFrame == 0)
  {
    return PC_PROTOCOL_STATUS_BAD_LENGTH;
  }

  if (aP2PClientContext[0].state == APP_BLE_IDLE)
  {
    Pc_SendDiag("RX_CMD_NO_LINK");
    return PC_PROTOCOL_STATUS_NO_LINK;
  }

  if (aP2PClientContext[0].P2PWriteToServerCharHdle == 0U)
  {
    Pc_SendDiag("RX_CMD_NO_DT_RX");
    return PC_PROTOCOL_STATUS_UNSUPPORTED;
  }

  if (Pc_BuildWireFrame(pFrame, PcBleFrame, sizeof(PcBleFrame), &frame_length) == 0U)
  {
    Pc_SendDiag("RX_CMD_BUILD_FAIL");
    return PC_PROTOCOL_STATUS_BAD_LENGTH;
  }

  ble_status = Write_Char_With_Length(DT_RX_CHAR_UUID, 0U, PcBleFrame, frame_length);
  if (ble_status == BLE_STATUS_SUCCESS)
  {
    LED_Toggle(GREEN);
    Pc_SendDiag("RX_CMD_FORWARDED");
    return PC_PROTOCOL_STATUS_DEFERRED;
  }

  LED_Toggle(RED);
  Pc_SendDiag("RX_CMD_FORWARD_FAIL");
  return PC_PROTOCOL_STATUS_BUSY;
}

static uint8_t Pc_BuildWireFrame(const PC_Protocol_Frame_t *pFrame, uint8_t *pBuffer, uint16_t buffer_length, uint8_t *pFrameLength)
{
  uint16_t crc;
  uint16_t index = 0U;
  uint16_t total_length;

  if ((pFrame == 0) || (pBuffer == 0) || (pFrameLength == 0))
  {
    return 0U;
  }

  total_length = (uint16_t)(2U + 8U + pFrame->PayloadLen + 2U);
  if ((pFrame->PayloadLen > PC_PROTOCOL_MAX_PAYLOAD) || (total_length > buffer_length))
  {
    return 0U;
  }

  pBuffer[index++] = 0xA5U;
  pBuffer[index++] = 0x5AU;
  pBuffer[index++] = PC_PROTOCOL_VERSION;
  pBuffer[index++] = pFrame->Type;
  pBuffer[index++] = pFrame->Flags;
  pBuffer[index++] = (uint8_t)pFrame->ScanId;
  pBuffer[index++] = (uint8_t)(pFrame->ScanId >> 8);
  pBuffer[index++] = (uint8_t)pFrame->Seq;
  pBuffer[index++] = (uint8_t)(pFrame->Seq >> 8);
  pBuffer[index++] = pFrame->PayloadLen;

  if (pFrame->PayloadLen > 0U)
  {
    memcpy(&pBuffer[index], pFrame->Payload, pFrame->PayloadLen);
    index = (uint16_t)(index + pFrame->PayloadLen);
  }

  crc = PC_Protocol_Crc16Ccitt(&pBuffer[2], (uint16_t)(8U + pFrame->PayloadLen));
  pBuffer[index++] = (uint8_t)crc;
  pBuffer[index++] = (uint8_t)(crc >> 8);
  *pFrameLength = (uint8_t)index;
  return 1U;
}

static uint8_t Pc_IsWireFrame(const uint8_t *pPayload, uint8_t length)
{
  uint8_t payload_len;
  uint16_t total_len;
  uint16_t received_crc;
  uint16_t calc_crc;

  if ((pPayload == 0) || (length < 12U))
  {
    return 0U;
  }

  if ((pPayload[0] != 0xA5U) || (pPayload[1] != 0x5AU) || (pPayload[2] != PC_PROTOCOL_VERSION))
  {
    return 0U;
  }

  payload_len = pPayload[9];
  total_len = (uint16_t)(2U + 8U + payload_len + 2U);
  if ((payload_len > PC_PROTOCOL_MAX_PAYLOAD) || (total_len != length))
  {
    return 0U;
  }

  received_crc = (uint16_t)pPayload[10U + payload_len] | ((uint16_t)pPayload[11U + payload_len] << 8);
  calc_crc = PC_Protocol_Crc16Ccitt(&pPayload[2], (uint16_t)(8U + payload_len));
  return (received_crc == calc_crc) ? 1U : 0U;
}
/* USER CODE END LF */
