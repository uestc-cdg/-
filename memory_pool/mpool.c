/* mpool.c */
#include "mpool.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 内存对齐辅助宏 (通常按 void* 大小对齐) */
#define ALIGN_SIZE sizeof(void*)
#define ALIGN(size) (((size) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))

/* 内存块结构 (向系统申请的大块内存) */
typedef struct mpool_block_s {
    struct mpool_block_s* next; // 指向下一个大块，用于销毁时遍历
    char data[];                // 柔性数组，实际内存区域
} mpool_block_t;

/* 内存池管理结构 */
struct mpool_s {
    size_t unit_size;           // 经过对齐后的单元大小
    size_t block_data_size;     // 每个大块中用于存储数据的区域大小
    mpool_block_t* block_head;  // 大块链表头 (用于释放)
    void* free_list_head;       // 空闲单元链表头
    
    /* 统计信息 */
    size_t total_blocks;
    size_t total_units;
    size_t free_count;
};

mpool_t* mpool_create(size_t unit_size, size_t block_size) {
    if (unit_size == 0 || block_size == 0) return NULL;

    mpool_t* pool = (mpool_t*)malloc(sizeof(mpool_t));
    if (!pool) return NULL;

    /* 1. 强制对齐 unit_size */
    size_t aligned_unit = ALIGN(unit_size);
    /* 2. 单元大小至少要能存下一个指针 (用于空闲链表) */
    if (aligned_unit < sizeof(void*)) {
        aligned_unit = sizeof(void*);
    }

    pool->unit_size = aligned_unit;
    
    /* 计算大块中实际可用的数据大小 (扣除头部开销) */
    /* block_size 最好大于 sizeof(mpool_block_t) + unit_size */
    if (block_size < sizeof(mpool_block_t) + aligned_unit) {
        // 如果传入的block太小，自动调整为至少能放一个unit
        block_size = sizeof(mpool_block_t) + aligned_unit * 4; 
    }
    pool->block_data_size = block_size - sizeof(mpool_block_t);

    pool->block_head = NULL;
    pool->free_list_head = NULL;
    pool->total_blocks = 0;
    pool->total_units = 0;
    pool->free_count = 0;

    return pool;
}

void mpool_destroy(mpool_t* pool) {
    if (!pool) return;

    mpool_block_t* curr = pool->block_head;
    while (curr) {
        mpool_block_t* next = curr->next;
        free(curr);
        curr = next;
    }
    free(pool);
}

/* 内部函数：申请一个新的大块并切割 */
static int mpool_grow(mpool_t* pool) {
    size_t total_size = sizeof(mpool_block_t) + pool->block_data_size;
    mpool_block_t* block = (mpool_block_t*)malloc(total_size);
    if (!block) return -1;

    // 1. 链接到大块管理链表
    block->next = pool->block_head;
    pool->block_head = block;
    pool->total_blocks++;

    // 2. 切割内存块并挂入空闲链表
    // 我们从大块的末尾开始切，这样挂入链表后，第一次取出的就是大块头部的内存
    // 有助于缓存局部性
    char* data_start = block->data;
    size_t num_units = pool->block_data_size / pool->unit_size;
    
    for (size_t i = 0; i < num_units; ++i) {
        char* unit_ptr = data_start + (i * pool->unit_size);
        
        // 将当前 pool->free_list_head 的值写入该单元的前 sizeof(void*) 字节
        // 相当于 node->next = head;
        *(void**)unit_ptr = pool->free_list_head;
        
        // 更新头指针
        pool->free_list_head = (void*)unit_ptr;
    }

    pool->total_units += num_units;
    pool->free_count += num_units;
    
    return 0;
}

void* mpool_alloc(mpool_t* pool) {
    if (!pool) return NULL;

    // 如果没有空闲单元，申请新的大块
    if (pool->free_list_head == NULL) {
        if (mpool_grow(pool) != 0) {
            return NULL; // 内存耗尽
        }
    }

    // 从空闲链表头部取出一个单元
    void* ptr = pool->free_list_head;
    
    // 移动头指针指向下一个空闲单元
    // 这里的逻辑是：取出 ptr 指向的内容（即下一个单元的地址）
    pool->free_list_head = *(void**)ptr;
    
    pool->free_count--;
    return ptr;
}

void mpool_free(mpool_t* pool, void* ptr) {
    if (!pool || !ptr) return;

    // 将 ptr 插入到 free_list_head 的头部
    // 1. 将当前的 head 地址写入 ptr 指向的内存
    *(void**)ptr = pool->free_list_head;
    
    // 2. 更新 head 为 ptr
    pool->free_list_head = ptr;
    
    pool->free_count++;
}

void mpool_get_stats(mpool_t* pool, mpool_stats_t* stats) {
    if (!pool || !stats) return;
    stats->unit_size = pool->unit_size;
    stats->total_blocks = pool->total_blocks;
    stats->total_units = pool->total_units;
    stats->free_units = pool->free_count;
}