#include "utils/icraft_list.h"
#include "icraft_errno.h"


icraft_return
icraft_list_node_create(icraft_list_node_t **node, void *data){
    *node = (icraft_list_node_t*)malloc(sizeof(icraft_list_node_t));
    if(*node == NULL){
        return ICRAFT_LIST_NODE_MALLOC_ERROR;
    }
    (*node)->data = data;
    (*node)->next = NULL;
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_list_append(icraft_list_node_t **head, void *data){
    icraft_return ret;
    icraft_list_node_t *new_node;
    ret = icraft_list_node_create(&new_node, data);
    if(ret){
        return ret;
    }
    else{
        if((*head) == NULL){
            *head = new_node;
        }
        else{
            icraft_list_node_t *current = *head;
            while(current->next != NULL){
                current = current->next;
            }
            current->next = new_node;
        }
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_list_len(icraft_list_node_t *node, uint32_t *len){
    (*len) = 0;
    while(node != NULL){
        (*len)++;
        node = node->next;
    }
    return ICRAFT_SUCCESS;
}

void icraft_list_destroy(icraft_list_node_t* node){
    while(node != NULL){
        icraft_list_node_t* tmp = node;
        node = node->next;
        free(tmp);
    }
}