#include "my_malloc.h"

/*void * initialize_malloc(size_t size, int isLock){
    //assert(sbrk(0) != (void *)(-1));
    if(isLock){
        head_metadata = sbrk(metadata_size + size); 
    }
    else{
        pthread_mutex_lock(&lock);
        head_metadata = sbrk(metadata_size + size); 
        pthread_mutex_unlock(&lock);
    }
    head_metadata->allocSize = size;
    return ((void *)head_metadata + metadata_size);
    //void * conversion is used to tell the pointer advances for 1 byte each time, if using int *, the pointer will advance for 4 bytes each time
}*/

void * increaseHeapMemory(size_t size, int isLock){
    metadata * curr_metadata;
    if(isLock){
        curr_metadata = sbrk(metadata_size + size);
    }
    else{
        pthread_mutex_lock(&lock);
        curr_metadata = sbrk(metadata_size + size);
        pthread_mutex_unlock(&lock);
    }
    //assert(curr_metadata != (void *)(-1));
    curr_metadata->allocSize = size;
    return ((void *)curr_metadata + metadata_size);
}

void delete_free_metadata(metadata * curr_free_metadata, metadata ** head_free_metadata, metadata ** tail_free_metadata){
    if(*head_free_metadata == curr_free_metadata){
        if(*head_free_metadata == *tail_free_metadata){
          *head_free_metadata = NULL;
          *tail_free_metadata = NULL;
          return;
        }

        *head_free_metadata = (*head_free_metadata)->next;
        (*head_free_metadata)->prev = NULL;
    }
    else if(*tail_free_metadata == curr_free_metadata){
        *tail_free_metadata = (*tail_free_metadata)->prev;
        (*tail_free_metadata)->next = NULL;
    }
    else{
        ((metadata *)(curr_free_metadata->prev))->next = curr_free_metadata->next;
        ((metadata *)(curr_free_metadata->next))->prev = curr_free_metadata->prev;
    }
}

void add_free_metadata(metadata * curr_free_metadata, metadata ** head_free_metadata, metadata ** tail_free_metadata){
    if(*head_free_metadata == NULL){
        *head_free_metadata = curr_free_metadata;
        *tail_free_metadata = curr_free_metadata;
        curr_free_metadata->prev = NULL;
        curr_free_metadata->next = NULL;
    }
    else{
        metadata * temp_free_metadata = *head_free_metadata;
        
        if(curr_free_metadata < *head_free_metadata){
          *head_free_metadata = curr_free_metadata;
          curr_free_metadata->next = temp_free_metadata;
          curr_free_metadata->prev = NULL;

          temp_free_metadata->prev = curr_free_metadata;
        }
        else{
          while (temp_free_metadata!= NULL)
          {
            if(temp_free_metadata > curr_free_metadata){
              break;
            }
            temp_free_metadata = temp_free_metadata->next;
          }

          if(temp_free_metadata == NULL){
            metadata * prev_free_metadata = *tail_free_metadata;
            *tail_free_metadata = curr_free_metadata;
            curr_free_metadata->next = NULL;
            curr_free_metadata->prev = prev_free_metadata;

            prev_free_metadata->next = curr_free_metadata;
          }
          else{
            curr_free_metadata->prev = temp_free_metadata->prev;
            curr_free_metadata->next = temp_free_metadata;

            ((metadata *)temp_free_metadata->prev)->next = curr_free_metadata;
            temp_free_metadata->prev = curr_free_metadata;
            
          }
          
        }
    }
}
void splitBlock(metadata * curr_free_metadata, size_t size, metadata ** head_free_metadata, metadata ** tail_free_metadata){
    metadata * new_free_metadata = (void *)curr_free_metadata + metadata_size + size;
    new_free_metadata->allocSize = curr_free_metadata->allocSize - size - metadata_size;
    curr_free_metadata->allocSize = size;
    
    if(curr_free_metadata == *head_free_metadata){
      if(*head_free_metadata == *tail_free_metadata){
        *head_free_metadata = new_free_metadata;
        *tail_free_metadata = new_free_metadata;
        new_free_metadata->prev = NULL;
        new_free_metadata->next = NULL;
      }
      else{
        new_free_metadata->prev = NULL;
        new_free_metadata->next = (*head_free_metadata)->next;
        ((metadata *)(*head_free_metadata)->next)->prev = new_free_metadata;
        *head_free_metadata = new_free_metadata;
      }
    }
    else if (curr_free_metadata == *tail_free_metadata)
    {
      new_free_metadata->prev = (*tail_free_metadata)->prev;
      new_free_metadata->next = NULL;
      ((metadata *)(*tail_free_metadata)->prev)->next = new_free_metadata;
      *tail_free_metadata = new_free_metadata;
    }
    else {
      new_free_metadata->prev = curr_free_metadata->prev;
      new_free_metadata->next = curr_free_metadata->next;
      ((metadata *)(curr_free_metadata->prev))->next = new_free_metadata;
      ((metadata *)(curr_free_metadata->next))->prev = new_free_metadata;
    }
}

void merge_free_metadata(metadata * curr_free_metadata, metadata ** head_free_metadata, metadata ** tail_free_metadata){
    if(curr_free_metadata->next != NULL){
      if(curr_free_metadata->next == ((void *)curr_free_metadata + metadata_size + curr_free_metadata->allocSize)){
        curr_free_metadata->allocSize += metadata_size + ((metadata *)(curr_free_metadata->next))->allocSize; 
        curr_free_metadata->next = ((metadata *)curr_free_metadata->next)->next;
        
         //after merge, if curr_metadata becomes the last metadata
        if(curr_free_metadata->next == NULL){
            *tail_free_metadata = curr_free_metadata;
        }
        else{ //if not, update the prev of the next metadata adjacent to curr_metadata to curr_metadata
            ((metadata *)curr_free_metadata->next)->prev = curr_free_metadata;
        }
      }
    }

    if(curr_free_metadata->prev != NULL){
      metadata * temp_free_metadata = curr_free_metadata->prev;
      if(curr_free_metadata == ((void *)temp_free_metadata + metadata_size + temp_free_metadata->allocSize)){
        temp_free_metadata->allocSize += metadata_size + curr_free_metadata->allocSize; 
        temp_free_metadata->next = curr_free_metadata->next;
        
         //after merge, if temp_metadata becomes the last metadata
        if(temp_free_metadata->next == NULL){
            *tail_free_metadata = temp_free_metadata;
        }
        else{ //if not, update the prev of the next metadata adjacent to curr_metadata to curr_metadata
            ((metadata *)temp_free_metadata->next)->prev = temp_free_metadata;
        }
      }
    }
}

void *bf_malloc(size_t size, metadata ** head_free_metadata, metadata ** tail_free_metadata, int isLock){
  // if(head_metadata==NULL){
  //       return initialize_malloc(size, isLock);
  // }
  // else{
      metadata * curr_free_metadata = *head_free_metadata;
      metadata * best_free_metadata = NULL;
      size_t best_allocSize = INT_MAX;

      while (curr_free_metadata != NULL)
      {
          if(curr_free_metadata->allocSize >= size){
              if(best_allocSize > curr_free_metadata->allocSize){
                best_allocSize = curr_free_metadata->allocSize;
                best_free_metadata = curr_free_metadata;
              }
          }

          if(best_allocSize == size){
              break;
          }
          curr_free_metadata = curr_free_metadata->next;
      }
      
      if(best_free_metadata == NULL){
          return increaseHeapMemory(size, isLock);
      }
      else{
          if(best_free_metadata->allocSize > (size + metadata_size)){
              //split
              splitBlock(best_free_metadata, size, head_free_metadata, tail_free_metadata);
          }
          else{
              //not split
              delete_free_metadata(best_free_metadata, head_free_metadata, tail_free_metadata);
          }

          return (void *)best_free_metadata + metadata_size;
      }
      
  // }
}

void bf_free(void *ptr, metadata ** head_free_metadata, metadata ** tail_free_metadata){
    if(ptr == NULL){
        return;
    }

    metadata * curr_free_metadata = ptr - metadata_size;

    add_free_metadata(curr_free_metadata, head_free_metadata, tail_free_metadata);  
    merge_free_metadata(curr_free_metadata, head_free_metadata, tail_free_metadata); 
}

void *ts_malloc_lock(size_t size){
    pthread_mutex_lock(&lock);
    void * ptr = bf_malloc(size, &head_free_metadata_lock, &tail_free_metadata_lock, 1);
    pthread_mutex_unlock(&lock);
    return ptr;
}

void ts_free_lock(void *ptr){
    pthread_mutex_lock(&lock);
    bf_free(ptr, &head_free_metadata_lock, &tail_free_metadata_lock);
    pthread_mutex_unlock(&lock);
}

void *ts_malloc_nolock(size_t size){
    void * ptr = bf_malloc(size, &head_free_metadata_nolock, &tail_free_metadata_nolock, 0);
    return ptr;
}

void ts_free_nolock(void *ptr){
    bf_free(ptr, &head_free_metadata_nolock, &tail_free_metadata_nolock);
}