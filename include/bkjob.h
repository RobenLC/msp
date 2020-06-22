
#define	MSWork_Code			1


#define BKJOB_A7_CHANNEL   1
#define MSGTYPE_CMD  0x100
#define MSGTYPE_RSP  0x200


#define BKJOB_M4_CHANNEL   2

//*****************************************************************************
//  command code define
//*****************************************************************************
#define BKCMD_ABORT             0x0030
#define BKCMD_IMAGE_IN          0x0031
#define BKCMD_IMAGE_IN_RSP      0x0131
#define BKCMD_REQUIRE_AREA      0x8032      // bit15=1=rjob1
#define BKCMD_REQUIRE_AREA_RSP  0x8132      // bit15=1=rjob1
#define BKCMD_SEND_AREA         0x0033
#define BKCMD_DONE_AREA         0x8034      // bit15=1=rjob1
#define BKCMD_DONE_AREA_RSP     0x8134      // bit15=1=rjob1
#define BKCMD_IMAGE_COMPLETE    0x8035      // bit15=1=rjob1
#define BKCMD_IMAGE_COMPLETE_RSP 0x8135      // bit15=1=rjob1


//*****************************************************************************
//  t_bk_cmd is the same with t_rjob_cmd
//*****************************************************************************
typedef struct __attribute__((__packed__))
{
    long            msgtype;    // for msg , don't modify
    unsigned long   tag;        // for user
    unsigned short  cmd;        // for user
    unsigned short  rsp;        // for user
    unsigned short  dSize;      // for user, attached data size/buffer size
    void *          dPtr;       // for user, attached data_ptr
    void *          mPtr;       // Driver internal use
}   t_bk_cmd;


//*****************************************************************************
//
//*****************************************************************************

typedef struct ts_chan_param
{
    int     mqCmd;        // Cmd message Queue
    int     mqRsp;        // Rsp message Queue
    int     mqPostman;    // Postman Message queue
    int     mqPostmanRX;
    int     mqOperator;
    int     mqWaiter;
    int     rjob0_fh;
    int     rjob1_fh;
    pthread_t     pthread;
    sem_t         *job_limit_sem;
    pthread_mutex_t mutex;
    unsigned int  sharemem_size;
    unsigned char *sharemem;
//    struct ts_chan_param *operator_chan;
//    struct ts_chan_param *waiter_chan;
    int           job_waiting_nr;
    int     argc;
    char    **argv;
}   t_chan_param;

typedef struct
{
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
}   t_rect;

typedef struct
{
    unsigned short total_area;
    t_rect         area[0];
}   t_areas;


typedef struct
{
 	unsigned short w;
	unsigned short h;
    unsigned short idx;
    unsigned short reserved;
	char           filename[32];
    unsigned char  data[0];			
}   t_image_param;          // used in BKCMD_SEND_AREA , BKCMD_DONE_AREA


//*****************************************************************************
//
//*****************************************************************************

void *operator_chan_start(void * arg);
void *waiter_chan_start(void * arg);
void *postman_start(void *arg);
void *postmanRX_start(void *arg);

void bkjob_get_cmd( int mqID, t_bk_cmd *cmd );
void bkjob_get_rsp( int mqID, t_bk_cmd *rsp );
int bkjob_try_get_cmd( int mqID, t_bk_cmd *pcmd);
void bkjob_send_cmd( int mqID, t_bk_cmd *cmd );
void bkjob_send_rsp( int mqID, t_bk_cmd *rsp );
void bkjob_send_rsp_alt( int mqID, t_bk_cmd cmdrsp, unsigned short rsp  );
void send_cmd_to_waiter( t_bk_cmd *pcmd );

void send_cmd_to_m4( t_chan_param *chan, t_bk_cmd *pcmd );

extern unsigned char image_buffer[16*1024];
unsigned int GetUint32_BE( void *src );
unsigned short GetUint16_BE( void *src );



