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
  bool is_free;
  struct my_metadata_t *next_list;
  struct my_metadata_t *next_neighbour;
  struct my_metadata_t *prev_neighbour;
} my_metadata_t;

typedef struct my_heap_t {
  my_metadata_t dummy;
  int free_head_list_size;
  my_metadata_t *free_head_list[10];  // MAGIC: 配列の長さ
  my_metadata_t *neighbour_head;
} my_heap_t;

//
// Static variables (DO NOT ADD ANOTHER STATIC VARIABLES!)
//
my_heap_t my_heap;

//
// Helper functions (feel free to add/remove/edit!)
//

//
// ------------- 単方向リスト free list と metadata のメンバー is_free の更新 -------------- //
// ------------------- 必ず free_head_list[i] から始まり, dummy で終わる ------------------ //
//

void my_initialize_free_list(){
  my_heap.free_head_list_size = sizeof(my_heap.free_head_list) / sizeof(my_metadata_t *);
  for(int i=0; i < my_heap.free_head_list_size; ++i){
    my_heap.free_head_list[i] = &my_heap.dummy;
  }
  my_heap.dummy.is_free = true;
  my_heap.dummy.next_list = NULL;
}

// 指定のサイズの free list を担当している my_heap.free_head_list のインデックスを返す
int my_find_list_index(size_t size){
  const size_t bin_range = 100;   // MAGIC: 1つの bin が担当する size の範囲
  for(int i=0; i < my_heap.free_head_list_size; ++i){
    if(i*bin_range <= size && size < (i+1)*bin_range)return i;
  }
  return my_heap.free_head_list_size -1;
}

void my_add_to_free_list(my_metadata_t *metadata) {
  assert(!metadata->next_list);
  metadata->is_free = true;
  int list_index = my_find_list_index(metadata->size);
  metadata->next_list = my_heap.free_head_list[list_index];
  my_heap.free_head_list[list_index] = metadata;
}

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  metadata->is_free = false;
  if (prev) {
    prev->next_list = metadata->next_list;
  } else {
    int list_index = my_find_list_index(metadata->size);
    my_heap.free_head_list[list_index] = metadata->next_list;
  }
  metadata->next_list = NULL;
}

my_metadata_t *my_find_prev_metadata(my_metadata_t *metadata){
  int my_list_index = my_find_list_index(metadata->size);
  my_metadata_t *candidate = my_heap.free_head_list[my_list_index];
  my_metadata_t *candidate_prev = NULL;
  while (candidate) {
    if(candidate == metadata){
      return candidate_prev;
    }
    candidate_prev = candidate;
    candidate = candidate->next_list;
  }
  assert(false);
}

//
// ------------------- 双方向リスト neighbour の更新 ------------------ //
// ---- 空き領域, 使用中領域を合わせたすべての領域をアドレス順に並べる ---- //
// -- 新たに map した領域は先頭に加えるが, これは隣接しているとは限らない - //
// ---------- 必ず neighbour_head から始まり, dummy で終わる ----------- //
//

void my_initialize_neighbour(){
  my_heap.neighbour_head = &my_heap.dummy;
  my_heap.dummy.next_neighbour = NULL;
  my_heap.dummy.prev_neighbour = NULL;
}

void my_insert_to_head_of_neighbours(my_metadata_t *metadata){
  // neighbour_head = a
  // → neighbour_head = metadata -> a
  my_metadata_t *a = my_heap.neighbour_head;
  metadata->prev_neighbour = NULL;
  metadata->next_neighbour = a;
  a->prev_neighbour = metadata;
  my_heap.neighbour_head = metadata;
}

void my_insert_neighbour_after(my_metadata_t *new_data, my_metadata_t *cur_data){
  // cur_data(!= NULL) -> a
  // → cur_data -> new_data -> a
  new_data->next_neighbour = cur_data->next_neighbour;
  cur_data->next_neighbour->prev_neighbour = new_data;
  cur_data->next_neighbour = new_data;
  new_data->prev_neighbour = cur_data;
}

void my_remove_from_neighbours(my_metadata_t *metadata){
  // a -> metadata -> b
  // → a -> b
  // or neighbour_head = metadata -> b
  // → neighbour_head = b
  if(metadata->prev_neighbour){
    metadata->next_neighbour->prev_neighbour = metadata->prev_neighbour;
    metadata->prev_neighbour->next_neighbour = metadata->next_neighbour;
  }
  else{
    my_heap.neighbour_head = metadata->next_neighbour;
    my_heap.neighbour_head->prev_neighbour = NULL;
  }
  metadata->next_neighbour = NULL;
  metadata->prev_neighbour = NULL;
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//

// This is called at the beginning of each challenge.
void my_initialize() {
  my_heap.dummy.size = 0;
  my_initialize_free_list();
  my_initialize_neighbour();
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
      metadata = metadata->next_list;
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
    metadata->next_list = NULL;
    my_add_to_free_list(metadata);
    my_insert_to_head_of_neighbours(metadata);
    // Now, try my_malloc() again. This should succeed.
    return my_malloc(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | smallest_matadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = smallest_matadata + 1;
  size_t remaining_size = smallest_matadata->size - size;
  // Remove the free slot from the free list and drop it from the neighbour list.
  my_remove_from_free_list(smallest_matadata, smallest_prev);
  
  if (remaining_size > sizeof(my_metadata_t)) {
    // Shrink the metadata for the allocated object
    // to separate the rest of the region corresponding to remaining_size.
    // If the remaining_size is not large enough to make a new metadata,
    // this code path will not be taken and the region will be managed
    // as a part of the allocated object.
    smallest_matadata->size = size;
    smallest_matadata->next_list = NULL;
    // Create a new metadata for the remaining free slot.
    //
    // ... | metadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><----------------------->
    //                   size       remaining size
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next_list = NULL;
    // Add the remaining free slot to the free list.
    my_insert_neighbour_after(new_metadata, smallest_matadata); 
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}


// metadata の右隣の領域が空き領域だったら結合（右隣の領域を削除して metadata->size を拡大）する。
// ただし mmap したページはまたがない。
void my_joint_right(my_metadata_t *metadata){
  my_metadata_t *right_metadata = metadata->next_neighbour;
  if (right_metadata != &my_heap.dummy && (uintptr_t)right_metadata % 4096 != 0 
      && right_metadata->is_free) {
    my_remove_from_free_list(right_metadata, my_find_prev_metadata(right_metadata));
    my_remove_from_neighbours(right_metadata);
    metadata->size += right_metadata->size + sizeof(my_metadata_t);
    return;
  }
  return;
}

// metadata の左隣の領域が空き領域だったら結合（現在の領域を削除して left->size を拡大）し、更新後の metadata ポインタを返す。
// ただし mmap したページはまたがない。
my_metadata_t * my_joint_left(my_metadata_t *metadata){
  my_metadata_t *left_metadata = metadata->prev_neighbour;
  if ((uintptr_t)metadata % 4096 != 0 && left_metadata != NULL && left_metadata->is_free) {
    my_remove_from_free_list(left_metadata, my_find_prev_metadata(left_metadata));
    my_remove_from_neighbours(metadata);
    left_metadata->size += metadata->size + sizeof(my_metadata_t);
    return left_metadata;
  }
  return metadata;
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

  my_joint_right(metadata);
  metadata = my_joint_left(metadata);

  // metadata % 4096 == 0 ⇔ metadata はページの先頭。ページ全体が空き領域なら ummap する。
  if((uintptr_t)metadata % 4096 == 0 && metadata->size + sizeof(my_metadata_t) == 4096){
    my_remove_from_neighbours(metadata);
    munmap_to_system(metadata, 4096);
  }
  
  // Add the free slot to the free list.
  else{
    my_add_to_free_list(metadata);
  }
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
