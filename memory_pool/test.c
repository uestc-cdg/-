/* main.c */
#include <stdio.h>
#include <time.h>
#include "mpool.h"

typedef struct {
    int id;
    char name[32];
    double value;
} my_data_t;

int main() {
    printf("=== Memory Pool Demo ===\n");
    printf("Size of my_data_t: %zu bytes\n", sizeof(my_data_t));

    // 1. 创建内存池
    // 单元大小为 my_data_t，每次扩容申请 4KB 的大块
    mpool_t* pool = mpool_create(sizeof(my_data_t), 4096);
    if (!pool) {
        fprintf(stderr, "Failed to create pool\n");
        return 1;
    }

    // 2. 分配测试
    const int TEST_COUNT = 100;
    my_data_t* ptrs[TEST_COUNT];

    for (int i = 0; i < TEST_COUNT; i++) {
        ptrs[i] = (my_data_t*)mpool_alloc(pool);
        if (ptrs[i]) {
            ptrs[i]->id = i;
            sprintf(ptrs[i]->name, "Object-%d", i);
            ptrs[i]->value = i * 3.14;
        }
    }

    // 3. 查看状态
    mpool_stats_t stats;
    mpool_get_stats(pool, &stats);
    printf("\n[Stats after alloc]\n");
    printf("  Unit Size: %zu\n", stats.unit_size);
    printf("  Total Blocks: %zu\n", stats.total_blocks);
    printf("  Total Units: %zu\n", stats.total_units);
    printf("  Free Units: %zu\n", stats.free_units);

    // 4. 使用数据
    printf("\nData verification: ptrs[50]->name = %s\n", ptrs[50]->name);

    // 5. 释放部分数据
    printf("\nFreeing first 50 objects...\n");
    for (int i = 0; i < 50; i++) {
        mpool_free(pool, ptrs[i]);
    }

    mpool_get_stats(pool, &stats);
    printf("[Stats after partial free]\n");
    printf("  Free Units: %zu\n", stats.free_units); // 应该增加了50

    // 6. 销毁内存池
    mpool_destroy(pool);
    printf("\nPool destroyed.\n");

    return 0;
}