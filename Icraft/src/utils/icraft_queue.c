#include "utils/icraft_queue.h"

// icraft_return
// icraft_queue_init(icraft_queue_t **queue, const uint32_t data_size)
// {
//     *queue = (icraft_queue_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(icraft_queue_t)));
//     if((*queue) == NULL){
//         return ICRAFT_QUEUE_MALLOC_ERROR;
//     }
//     (*queue)->data_size = data_size;
//     (*queue)->front = NULL;
//     (*queue)->rear = NULL;
//     return ICRAFT_SUCCESS;
// }


icraft_return
icraft_queue_init(icraft_queue_t **queue)
{
    *queue = (icraft_queue_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(icraft_queue_t)));
    if((*queue) == NULL){
        return ICRAFT_QUEUE_MALLOC_ERROR;
    }
    (*queue)->front = NULL;
    (*queue)->rear = NULL;
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_queue_destroy(icraft_queue_t *queue){
    if(queue == NULL){
        return ICRAFT_QUEUE_NULL;
    }
    icraft_queue_node_t *cur = queue->front;
    if(cur != NULL){
        icraft_queue_node_t *tmp = cur;
        cur = cur->next;
        free(tmp->data);
        free(tmp);
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_queue_enqueue(icraft_queue_t *queue, void *data)
{
    if(queue == NULL){
        return ICRAFT_QUEUE_NULL;
    } 
    icraft_queue_node_t *node = (icraft_queue_node_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(icraft_queue_node_t)));
    if(node == NULL){
        return ICRAFT_QUEUE_MALLOC_ERROR;
    }
    // node->data = aligned_alloc(CACHE_ALIGN_SIZE, queue->data_size);
    // if(node->data == NULL){
    //     return ICRAFT_QUEUE_MALLOC_ERROR;
    // }
    // memcpy(node->data, data, queue->data_size);
    node->data = data;
    node->next = NULL;
    icraft_bool_t is_empty;
    icraft_queue_is_empty(queue, &is_empty);
    if(is_empty){
        queue->front = node;
        queue->rear = node;
    }
    else{
        queue->rear->next = node;
        queue->rear = node;
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_queue_dequeue(icraft_queue_t *queue, void **data){
    if(queue == NULL){
        return ICRAFT_QUEUE_NULL;
    }
    icraft_bool_t is_empty;
    icraft_queue_is_empty(queue, &is_empty);
    if(is_empty){
        return ICRAFT_QUEUE_NO_DATA_DEQUEUE;
    }        
    icraft_queue_node_t *front_node = queue->front;
    // memcpy(data, front_node->data, queue->data_size);
    *data = front_node->data;
    queue->front = front_node->next;
    //free(front_node->data);
    free(front_node);
    if(queue->front == NULL){
        queue->rear = NULL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_queue_size(icraft_queue_t *queue, uint32_t *size){
    if(queue == NULL){
        return ICRAFT_QUEUE_NULL;
    }
    (*size) = 0;    
    icraft_queue_node_t *cur = queue->front;
    while(cur != NULL){
        ++(*size);
        cur = cur->next;
    }
    return ICRAFT_SUCCESS;
}


icraft_return
icraft_queue_is_empty(icraft_queue_t *queue, icraft_bool_t *result){
    if(queue == NULL){
        return ICRAFT_QUEUE_NULL;
    }
    if(queue->front == NULL){
        (*result) = ICRAFT_TRUE;
    }
    else{
        (*result) = ICRAFT_FALSE;
    }
    return ICRAFT_SUCCESS;
}