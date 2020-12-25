//*****************************************************************************
// ArrayedQueue :
//      this queue is a circular queque and suitable for small item
//      use user allocated array to store item
//      no malloc()
//*****************************************************************************
#include <string.h>

#include "arrayed_queue.h"

void AQ_Init(t_ArrayedQueue *qq , void *q_buf, int elem_size, int max_elem)
{
    qq->max         = max_elem;
    qq->elem_size   = elem_size;
    qq->size        = 0;
    qq->curr_idx    = 0;
    qq->end_idx     = 0;
    qq->q_buffer    = q_buf;
}

//*****************************************************************************
//  Adds an element to the back of the queue.
//  return :
//      0 = success
//      -1 = full or fail
//*****************************************************************************
int AQ_Push(t_ArrayedQueue *qq, void *item)
{
    if( qq->size >= qq->max )
        return -1;
    memcpy(((char*)(qq->q_buffer) + (qq->end_idx*qq->elem_size)), item, qq->elem_size);

    qq->size++;
    qq->end_idx++;
    if( qq->end_idx >= qq->max )  qq->end_idx = 0;
    return 0;
}

//*****************************************************************************
//  allocate a queue buffer to user, user can modify this element inside queue
//  this will reduce a memcpy()
//  return :
//      != NULL : item pointer
//      NULL : Queue full
//*****************************************************************************
void *AQ_AllocPush(t_ArrayedQueue *qq)
{
    void *pitem;
    if( qq->size >= qq->max )
        return NULL;
    pitem = (char*)(qq->q_buffer) + (qq->end_idx*qq->elem_size);

    qq->size++;
    qq->end_idx++;
    if( qq->end_idx >= qq->max )  qq->end_idx = 0;
    return pitem;
}

//*****************************************************************************
//  Removes an element from the front of the queue.
//  return :
//      0 = success
//      -1 = full or fail
//*****************************************************************************
int AQ_Pop(t_ArrayedQueue *qq, void *item)
{
    if( qq->size == 0 )
        return -1;
    memcpy(item, ((char*)(qq->q_buffer) + (qq->curr_idx*qq->elem_size)), qq->elem_size);

    qq->size--;
    qq->curr_idx++;
    if( qq->curr_idx >= qq->max )   qq->curr_idx = 0;
    return 0;
}

//*****************************************************************************
//  Returns a reference to the first element at the front of the queue.
//  return :
//      NULL = no item to get
//      not NULL = *item
//*****************************************************************************
void *AQ_Front(t_ArrayedQueue *qq )
{
    void *item;
    if( qq->size == 0 )
        return NULL;
    item = ((char*)(qq->q_buffer) + (qq->curr_idx*qq->elem_size));
    return item;
}

//*****************************************************************************
//  Drop front element
//  return :
//      0 = success
//      -1 = full or fail
//*****************************************************************************
int AQ_DropFront( t_ArrayedQueue *qq )
{
    if( qq->size == 0 )
        return -1;

    qq->size--;
    qq->curr_idx++;
    if( qq->curr_idx >= qq->max )   qq->curr_idx = 0;
    return 0;
}

//*****************************************************************************
//  get current element number in queue
//*****************************************************************************
int AQ_Size(t_ArrayedQueue *qq)
{
    return qq->size;
}

//*****************************************************************************
//  clear queue
//*****************************************************************************
void AQ_Clear(t_ArrayedQueue *qq)
{
    qq->size = 0;
    qq->curr_idx = 0;
    qq->end_idx = 0;
}



