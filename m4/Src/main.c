//*****************************************************************************
//  Feature :
//      1. UART7 to output log
//      2. OPENAMP for communicate with A7 using rjob command
//      3. handle command function in cmd_handler.c
//*****************************************************************************



/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "log.h"
#include "arrayed_queue.h"
#include <string.h>
//#include <sys/unistd.h>
//test
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
enum {
    TRANSFER_WAIT,
    TRANSFER_COMPLETE,
    TRANSFER_ERROR
};

//#define RPMSG_SERVICE_NAME              "rpmsg-client-sample"
#define RPMSG_SERVICE_NAME              "rjob"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
IPCC_HandleTypeDef hipcc;
SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_rx;
DMA_HandleTypeDef hdma_spi4_tx;
struct rpmsg_endpoint resmgr_ept;
struct rpmsg_endpoint resmgr_ept2;

/* USER CODE BEGIN PV */

t_rpmsg_status  rpmsg_status ;
t_rjob_cmd      rjob0_cmd;
t_rjob_cmd      rjob1_cmd;


#define         RJOB_QCMD_MAX   4
t_rjob_cmd      rjob0_qcmd[RJOB_QCMD_MAX];
t_rjob_cmd      rjob1_qcmd[RJOB_QCMD_MAX];
t_ArrayedQueue  rjob0_queue;
t_ArrayedQueue  rjob1_queue;



int spiTransferState = TRANSFER_WAIT;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_IPCC_Init(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI4_Init(void);

static int rjob0_rx_callback(struct rpmsg_endpoint *rp_chnl, void *data, size_t len, uint32_t src, void *priv);
static int rjob1_rx_callback(struct rpmsg_endpoint *rp_chnl, void *data, size_t len, uint32_t src, void *priv);
void LEDBlinking( void );
void LoadBank( int bk );
void ReadSpiFlash( uint32_t addr, void *buf, uint16_t size );

/* Private user code ---------------------------------------------------------*/


static void init_variable(void)
{
    rpmsg_status.rjob0_rx_status = TRANSFER_WAIT;
    rpmsg_status.rjob1_rx_status = TRANSFER_WAIT;
    rpmsg_status.tx_status = TRANSFER_WAIT;
}

static volatile int sleep_counter=0;
static void msleep_usr( int n )
{
    int i1, i2;
    for( i1=0; i1<n; i1++)
        for( i2=0;i2<16200;i2++)
            sleep_counter++;

}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    int8_t  ret;
  /* USER CODE BEGIN 1 */
//  struct rpmsg_endpoint resmgr_ept;


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  init_variable();

  /* Reset of all peripherals, Initialize the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  if(IS_ENGINEERING_BOOT_MODE())
  {
    /* Configure the system clock */
    SystemClock_Config();
  }
  /* USER CODE END Init */

  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  MX_IPCC_Init();
  MX_OPENAMP_Init(RPMSG_REMOTE, NULL);

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  BSP_LED_Init(LED7);
  InitLogUART7();
  log_info_nh("\r\n");
  log_sys("Cortex-M4 boot successful with STM32Cube %s %s\r\n",__DATE__,__TIME__);

  MX_DMA_Init();
  //MX_SPI4_Init();

  AQ_Init(&rjob0_queue, &rjob0_qcmd, sizeof(rjob0_qcmd[0]), RJOB_QCMD_MAX );
  AQ_Init(&rjob1_queue, &rjob1_qcmd, sizeof(rjob1_qcmd[0]), RJOB_QCMD_MAX );

  /* USER CODE BEGIN 2 */
  /* Create an rpmsg channel to communicate with the Master processor CPU1(CA7) */
  ret = OPENAMP_create_endpoint(&resmgr_ept, RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY,
          rjob0_rx_callback, NULL);
  if( ret != 0 )
      log_sys("OPENAMP_create_endpoint 1 ret = %d\r\n", ret);
  else
      log_sys("OPENAMP_create_endpoint 1 ret = %d name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept.name, resmgr_ept.addr, resmgr_ept.dest_addr);
      
  ret = OPENAMP_create_endpoint(&resmgr_ept2, RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY,
          rjob1_rx_callback, NULL);
  if( ret != 0 )
      log_sys("OPENAMP_create_endpoint 2 ret = %d\r\n", ret);
  else
      log_sys("OPENAMP_create_endpoint 2 ret = %d name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept2.name, resmgr_ept2.addr, resmgr_ept2.dest_addr);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */



    while( 1 )
    {

        if((AQ_Size(&rjob0_queue) == 0) &&
           (AQ_Size(&rjob1_queue) == 0))
        {
            //log_info("OPENAMP_check_for_message()\r\n");
            OPENAMP_check_for_message();
        }

        if( AQ_Size(&rjob0_queue) > 0 )
        {

            t_rjob_cmd *pcmd;
            pcmd = AQ_Front(&rjob0_queue);
            cmd_dispatcher(pcmd);
            AQ_DropFront(&rjob0_queue);
        }

        if( AQ_Size(&rjob1_queue) > 0 )
        {
            t_rjob_cmd *pcmd;
            pcmd = AQ_Front(&rjob1_queue);
            cmd_dispatcher(pcmd);
            AQ_DropFront(&rjob1_queue);
        }

    }
  /* USER CODE END WHILE */
}

//*****************************************************************************
//  wait_rjob_cmd
//  blocking when cmd not reveived
//*****************************************************************************
void *wait_rjob1_rsp()
{
    t_rjob_cmd *pcmd;
    while( AQ_Size(&rjob1_queue) == 0 )
    {
        OPENAMP_check_for_message();
    };

    pcmd = AQ_Front(&rjob1_queue);
    return pcmd;
}
void remove_rjob1_rsp()
{
    AQ_DropFront(&rjob1_queue);
}


//*****************************************************************************
//  send_rjob_rsp
//  send "cmd" response "rsp" to host
//  Note :  resmgr_ept.dest_addr is 0xfffffff when begining ,
//          it need to receive a command from A7 to update it to correct value
//          otherwise OPENAMP_send() will fail
//*****************************************************************************
int8_t send_rjob_rsp(  t_rjob_cmd *rsp )
{
    int ret;
    if( rsp->mPtr != 0 )
        rsp->mPtr = rsp->mPtr - (int)__RJob_ShareMem;
    if ((ret = OPENAMP_send(&resmgr_ept2, rsp, sizeof( t_rjob_cmd) )) < 0)
    {
        log_err("Failed to send rspmessage ret=%d 2:name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept2.name, resmgr_ept2.addr, resmgr_ept2.dest_addr);
        log_err("try to send rspmessage ret=%d 1:name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept.name, resmgr_ept.addr, resmgr_ept.dest_addr);
        //Error_Handler()
        if ((ret = OPENAMP_send(&resmgr_ept, rsp, sizeof( t_rjob_cmd) )) < 0)
        {
            log_err("Failed to send rspmessage ret=%d 1:name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept.name, resmgr_ept.addr, resmgr_ept.dest_addr);
        }
    }
    log_info("rjob  send rsp %d byte (cmd=%04x tag=%04lx rsp=%04x dsize=%5d dPtr=%08x mPtr=%08x)\r\n",
            sizeof(t_rjob_cmd),
            rsp->cmd,
            rsp->tag,
            rsp->rsp,
            rsp->dSize,
            (int)rsp->dPtr,
            (int)rsp->mPtr);

    return 0;
}

//*****************************************************************************
//  send_rjob_cmd
//  Note :  resmgr_ept.dest_addr is 0xfffffff when begining ,
//          it need to receive a command from A7 to update it to correct value
//          otherwise OPENAMP_send() will fail
//*****************************************************************************
int8_t send_rjob_cmd( t_rjob_cmd *pcmd )
{
    int ret;
    if( pcmd->mPtr != 0 )
        pcmd->mPtr = pcmd->mPtr - (int)__RJob_ShareMem;
    if ((ret = OPENAMP_send(&resmgr_ept2, pcmd, sizeof( t_rjob_cmd) )) < 0)
    {
        log_err("Failed to send cmdmessage ret=%d 2:name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept2.name, resmgr_ept2.addr, resmgr_ept2.dest_addr);
        log_err("try to send cmdmessage ret=%d 1:name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept.name, resmgr_ept.addr, resmgr_ept.dest_addr);
        //Error_Handler();
        if ((ret = OPENAMP_send(&resmgr_ept, pcmd, sizeof( t_rjob_cmd) )) < 0)
        {
            log_err("Failed to send cmdmessage ret=%d 1:name:%s src: 0x%.8x dst: 0x%.8x \r\n", ret, resmgr_ept.name, resmgr_ept.addr, resmgr_ept.dest_addr);
        }
    }
    log_info("rjob  send cmd %d byte (cmd=%04x tag=%04lx rsp=%04x dsize=%5d dPtr=%08x mPtr=%08x)\r\n",
            sizeof(t_rjob_cmd),
            pcmd->cmd,
            pcmd->tag,
            pcmd->rsp,
            pcmd->dSize,
            (int)pcmd->dPtr,
            (int)pcmd->mPtr);
    return 0;
}

//*****************************************************************************
//
//*****************************************************************************

/**
  * @brief IPPC Initialization Function
  * @param None
  * @retval None
  */
static void MX_IPCC_Init(void)
{

  hipcc.Instance = IPCC;
  if (HAL_IPCC_Init(&hipcc) != HAL_OK)
  {
     Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static int rjob0_rx_callback(struct rpmsg_endpoint *rp_chnl, void *data, size_t len, uint32_t src, void *priv)
{
  /* copy received msg, and raise a flag  */
    t_rjob_cmd *pcmd;

    pcmd = AQ_AllocPush(&rjob0_queue);
    if( pcmd == NULL )
    {
        log_err("Error : rjob0 RX buff full\r\n");
    }
    else
    {
        memcpy(pcmd, data, len > sizeof(t_rjob_cmd) ? sizeof(t_rjob_cmd) : len);
        log_info("rjob0 rcv  %d byte (cmd=%04x tag=%04lx rsp=%04x dsize=%5d dPtr=%08x mPtr=%08x)\r\n",
                len,
                pcmd->cmd,
                pcmd->tag,
                pcmd->rsp,
                pcmd->dSize,
                (int)pcmd->dPtr,
                (int)pcmd->mPtr);

        pcmd->mPtr = __RJob_ShareMem + (int)pcmd->mPtr;
        rpmsg_status.rjob0_rx_status = TRANSFER_COMPLETE;
        rpmsg_status.rjob0_rx_len = len;
    }
    return 0;
}

static int rjob1_rx_callback(struct rpmsg_endpoint *rp_chnl, void *data, size_t len, uint32_t src, void *priv)
{
  /* copy received msg, and raise a flag  */
    t_rjob_cmd *pcmd;
    pcmd = AQ_AllocPush(&rjob1_queue);
    if( pcmd == NULL )
    {
        log_err("Error : rjob1 RX buff full\r\n");
    }
    else
    {
        memcpy(pcmd, data, len > sizeof(t_rjob_cmd) ? sizeof(t_rjob_cmd) : len);
        pcmd->mPtr = __RJob_ShareMem + (int)pcmd->mPtr;
        rpmsg_status.rjob1_rx_status = TRANSFER_COMPLETE;
        rpmsg_status.rjob1_rx_len = len;
        log_info("rjob1 rcv  %d byte (cmd=%04x tag=%04lx rsp=%04x dsize=%5d dPtr=%08x mPtr=%08x)\r\n",
                len,
                pcmd->cmd,
                pcmd->tag,
                pcmd->rsp,
                pcmd->dSize,
                (int)pcmd->dPtr,
                (int)pcmd->mPtr);
    }
    return 0;
}

/* SPI4 init function */
static void MX_SPI4_Init( void )
{
    log_info( "+MX_SPI4_Init()\r\n");
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi4.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  //hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 7;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  //hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;

  HAL_StatusTypeDef err;
  if ((err = HAL_SPI_Init(&hspi4)) != HAL_OK)
  {
    log_err( "HAL_SPI_Init() error %x\r\n",err);
    Error_Handler();
  }

  log_info( "-MX_SPI4_Init()\r\n");
}


/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMAMUX_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}


/** Configure pins as

*/
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
}


/* USER CODE END 4 */

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  spiTransferState = TRANSFER_COMPLETE;
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  spiTransferState = TRANSFER_COMPLETE;
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  spiTransferState = TRANSFER_COMPLETE;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    log_err("HAL_SPI_ErrorCallback\r\n");
    BSP_LED_Off(LED7);
    BSP_LED_Off(LED7);
    while(1)
    {
      /* Toggle LED7 for error */
      BSP_LED_Toggle(LED7);
      HAL_Delay(1000);
    }
    spiTransferState = TRANSFER_ERROR;
}




/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  log_err("Error_Handler\r\n");
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  log_err("OOOps: file %s, line %d\r\n", __FILE__, __LINE__);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */



void LEDBlinking( void )
{
    BSP_LED_On(LED7);
    HAL_Delay(100);
    BSP_LED_Off(LED7);
    HAL_Delay(100);
    BSP_LED_On(LED7);
    HAL_Delay(100);
    BSP_LED_Off(LED7);
    HAL_Delay(100);
}

void ReadSpiFlash( uint32_t addr, void *buf, uint16_t size )
{
    uint8_t cmd[4];
    HAL_StatusTypeDef err;

    cmd[0] = 0x03;
    cmd[1] = addr>>16;
    cmd[2] = addr>>8;
    cmd[3] = addr;

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET);
    do
    {
        //if( (err = HAL_SPI_Transmit(&hspi4, cmd, 4 ,100)) != HAL_OK )    break;
        spiTransferState = TRANSFER_WAIT;
        if( (err = HAL_SPI_Transmit_DMA(&hspi4, cmd, 4 )) != HAL_OK )    break;
        while( spiTransferState == TRANSFER_WAIT);

        //if( (err = HAL_SPI_Receive(&hspi4, buf, size ,100)) != HAL_OK )  break;
        spiTransferState = TRANSFER_WAIT;
        if( (err = HAL_SPI_Receive_DMA(&hspi4, buf, size )) != HAL_OK )  break;
        while( spiTransferState == TRANSFER_WAIT);

    } while(0);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);

    if( err != HAL_OK )
        log_err( "SPI read Flash error %X\r\n", err );


}



//*****************************************************************************
//  Example :
//      t_INT_FUNC_INT *fp;
//      LoadBauk( 1 );
//      *fp=(t_INT_FUNC_INT *)(__BanKEntryTable[3]);
//      fp(count);
//*****************************************************************************
void LoadBank( int bk )
{
#if 0
    const uint32_t bankFlashAddr[] = { 0x10000, 0x11000   };
    const uint32_t bankFlashSize[] = { 0x400, 0x400 };

    if( bk >= 2 )
    {
        Error_Handler();
    }

    ReadSpiFlash( bankFlashAddr[bk], (void*)__BanKEntryTable, bankFlashSize[bk]);
#endif
}
