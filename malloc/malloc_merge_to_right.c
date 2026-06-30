//
// >>>> malloc challenge! <<<<
//
// Your task is to improve utilization and speed of the following malloc
// implementation.
// Initial implementation is the same as the one implemented in simple_malloc.c.
// For the detailed explanation, please refer to simple_malloc.c.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Interfaces to get memory pages from OS
//

void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);

//
// Struct definitions
//

typedef struct my_metadata_t {
  size_t size;
  struct my_metadata_t *next;
} my_metadata_t;

typedef struct my_heap_t {
  int free_head_list_size;
  my_metadata_t *free_head_list[10];  // MAGIC: 配列の長さ
  my_metadata_t dummy;
} my_heap_t;

//
// Static variables (DO NOT ADD ANOTHER STATIC VARIABLES!)
//
my_heap_t my_heap;

//
// Helper functions (feel free to add/remove/edit!)
//

// 指定のサイズの free list を担当している my_heap.free_head_list のインデックスを返す
int my_find_list_index(size_t size){
  const size_t bin_range = 200;   // MAGIC: 1つの bin が担当する size の範囲
  for(int i=0; i < my_heap.free_head_list_size; ++i){
    if(i*bin_range <= size && size < (i+1)*bin_range)return i;
  }
  return my_heap.free_head_list_size -1;
}

void my_add_to_free_list(my_metadata_t *metadata) {
  assert(!metadata->next);
  int list_index = my_find_list_index(metadata->size);
  metadata->next = my_heap.free_head_list[list_index];
  my_heap.free_head_list[list_index] = metadata;
}

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  if (prev) {
    prev->next = metadata->next;
  } else {
    int list_index = my_find_list_index(metadata->size);
    my_heap.free_head_list[list_index] = metadata->next;
  }
  metadata->next = NULL;
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//

// This is called at the beginning of each challenge.
void my_initialize() {
  my_heap.free_head_list_size = sizeof(my_heap.free_head_list) / sizeof(my_metadata_t *);
  for(int i=0; i < my_heap.free_head_list_size; ++i){
    my_heap.free_head_list[i] = &my_heap.dummy;
  }
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
}

// my_malloc() is called every time an object is allocated.
// |size| is guaranteed to be a multiple of 8 bytes and meets 8 <= |size| <=
// 4000. You are not allowed to use any library functions other than
// mmap_from_system() / munmap_to_system().
void *my_malloc(size_t size) {
  // Best-fit: Find the smallest free slot the object fits.
  my_metadata_t *smallest_matadata = NULL;
  my_metadata_t *smallest_prev = NULL;

  // smallest_list_index より前の bin には size 以上の大きさの空き領域がない 
  int smallest_list_index = my_find_list_index(size);
  for(int i=smallest_list_index; i < my_heap.free_head_list_size; ++i){
    my_metadata_t *metadata = my_heap.free_head_list[i];
    my_metadata_t *prev = NULL;
    while (metadata) {
      if (metadata->size >= size &&
          (smallest_matadata == NULL || smallest_matadata->size > metadata->size)) {
        smallest_matadata = metadata;
        smallest_prev = prev;
      }
      prev = metadata;
      metadata = metadata->next;
    }
    // これ以降の metadata は smallest_matadata より大きいので、smallest_matadata があったら決定
    if (smallest_matadata)break;
  }

  if (!smallest_matadata) {
    // There was no free slot available. We need to request a new memory region
    // from the system by calling mmap_from_system().
    //
    //     | smallest_matadata | free slot |
    //     ^
    //     metadata
    //     <---------------------->
    //            buffer_size
    size_t buffer_size = 4096;
    my_metadata_t *metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    metadata->size = buffer_size - sizeof(my_metadata_t);
    metadata->next = NULL;
    // Add the memory region to the free list.
    my_add_to_free_list(metadata);
    // Now, try my_malloc() again. This should succeed.
    // TODO: ここを smallest = heap.free_heap にしたらより速そう
    return my_malloc(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | smallest_matadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = smallest_matadata + 1;
  size_t remaining_size = smallest_matadata->size - size;
  // Remove the free slot from the free list.
  my_remove_from_free_list(smallest_matadata, smallest_prev);

  if (remaining_size > sizeof(my_metadata_t)) {
    // Shrink the metadata for the allocated object
    // to separate the rest of the region corresponding to remaining_size.
    // If the remaining_size is not large enough to make a new metadata,
    // this code path will not be taken and the region will be managed
    // as a part of the allocated object.
    smallest_matadata->size = size;
    smallest_matadata->next = NULL;
    // Create a new metadata for the remaining free slot.
    //
    // ... | smallest_matadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><---------------------->
    //                   size       remaining size
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    // Add the remaining free slot to the free list.
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}


// metadata の右隣の領域が空き領域だったら結合（右隣の領域を削除して metadata->size を拡大）する。
void my_joint_to_right(my_metadata_t *metadata, void *ptr){
  my_metadata_t *right_metadata = (my_metadata_t *)((char *)ptr + metadata->size);
  if((uintptr_t)right_metadata % 4096 == 0){
    return;
  }

  for(int i=0; i < my_heap.free_head_list_size; ++i){
    my_metadata_t *candidate = my_heap.free_head_list[i];
    my_metadata_t *candidate_prev = NULL;
    while (candidate) {
      if(candidate == right_metadata){
        metadata->size += candidate->size + sizeof(my_metadata_t);
        my_remove_from_free_list(candidate, candidate_prev);
        return;
      }
      candidate_prev = candidate;
      candidate = candidate->next;
    }
  }

  return;
}

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;

  my_joint_to_right(metadata, ptr);

  // metadata % 4096 == 0 ⇔ metadata はページの先頭。ページ全体が空き領域なら ummap する。
  if((uintptr_t)metadata % 4096 == 0 && metadata->size + sizeof(my_metadata_t) == 4096){
    munmap_to_system(metadata, 4096);
  }
  // Add the free slot to the free list.
  else my_add_to_free_list(metadata);
}

// This is called at the end of each challenge.
void my_finalize() {
  // Nothing is here for now.
  // feel free to add something if you want!
}

void test() {
  // Implement here!
  assert(1 == 1); /* 1 is 1. That's always true! (You can remove this.) */
}
