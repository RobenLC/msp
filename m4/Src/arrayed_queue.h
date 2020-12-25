//*****************************************************************************
// ArrayedQueue :
//      this queue is a circular queque and suitable for small item
//      use user allocated array to store item
//      no malloc()
//*****************************************************************************

typedef struct
{
    int max;
    int elem_size;
    int size;
    int curr_idx;
    int end_idx;
    void *q_buffer;
}   t_ArrayedQueue;

void AQ_Init(t_ArrayedQueue *qq , void *q_buf, int elem_size, int max_elem);
int AQ_Push(t_ArrayedQueue *qq, void *item);
void *AQ_AllocPush(t_ArrayedQueue *qq);
int AQ_Pop(t_ArrayedQueue *qq, void *item);
void *AQ_Front(t_ArrayedQueue *qq );
int AQ_DropFront( t_ArrayedQueue *qq );
int AQ_Size(t_ArrayedQueue *qq);
void AQ_Clear(t_ArrayedQueue *qq);



