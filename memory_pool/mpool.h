/* mpool.h */
#ifndef _MPOOL_H_
#define _MPOOL_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 内存池句柄不透明声明 */
typedef struct mpool_s mpool_t;

/**
 * @brief 创建内存池
 * 
 * @param unit_size 每个内存单元的大小（字节），例如 sizeof(MyStruct)
 * @param block_size 内部每次向系统申请的大块内存大小（字节），建议为 unit_size 的倍数
 * @return mpool_t* 成功返回内存池句柄，失败返回 NULL
 */
mpool_t* mpool_create(size_t unit_size, size_t block_size);

/**
 * @brief 销毁内存池
 *        释放该池申请的所有系统内存，之后该池分配的所有指针均不可用
 * 
 * @param pool 内存池句柄
 */
void mpool_destroy(mpool_t* pool);

/**
 * @brief 从内存池中分配一个单元
 * 
 * @param pool 内存池句柄
 * @return void* 指向分配内存的指针，失败返回 NULL
 */
void* mpool_alloc(mpool_t* pool);

/**
 * @brief 将单元归还给内存池
 * 
 * @param pool 内存池句柄
 * @param ptr 待释放的指针
 */
void mpool_free(mpool_t* pool, void* ptr);

/**
 * @brief 获取内存池当前状态信息（调试用）
 */
typedef struct {
    size_t unit_size;       // 单元大小
    size_t total_blocks;    // 向系统申请的大块数量
    size_t total_units;     // 总单元数量
    size_t free_units;      // 当前可用单元数量
} mpool_stats_t;

void mpool_get_stats(mpool_t* pool, mpool_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // _MPOOL_H_