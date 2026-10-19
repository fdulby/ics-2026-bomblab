#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/* 输出一行无符号十进制整数；不把换行写入 out。 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t middle = (uint32_t)(challenge >> 16);
    uint32_t first = high ^ (middle + UINT32_C(0xc208));
    uint32_t second = low * UINT32_C(4) + (high ^ UINT32_C(0x729f));
    uint32_t values[] = {second * UINT32_C(3) ^ (first + UINT32_C(0x2192))};
    return write_answer(out, cap, values, 1);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (uint32_t)((challenge >> 4) & 63) * UINT32_C(3);
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t piece = (uint32_t)((challenge >> (7 + 3 * i)) & 15);
        values[i] = (values[i - 1] + piece) ^ (UINT32_C(9) * i);
    }
    return write_answer(out, cap, values, 6);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t index = (uint32_t)challenge & 7;
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    uint32_t value;
    /* 按实际跳转表的顺序恢复，不按 case 函数名的编号排列。 */
    switch (index) {
        case 0: value = x * UINT32_C(2) ^ UINT32_C(0x6999); break;
        case 1: value = x * UINT32_C(3) + UINT32_C(0xa9b8); break;
        case 2: value = x * UINT32_C(4) ^ UINT32_C(0x7eae); break;
        case 3: value = x * UINT32_C(3) + UINT32_C(0x8a81); break;
        case 4: value = x * UINT32_C(5) + UINT32_C(0x43189); break;
        case 5: value = x * UINT32_C(5) + UINT32_C(0x24a81); break;
        case 6: value = (x ^ UINT32_C(0x479b)) + (x >> 1); break;
        default: value = (x ^ UINT32_C(0xccdc)) + (x >> 2); break;
    }
    uint32_t values[] = {index, value};
    return write_answer(out, cap, values, 2);
}

static uint32_t walk(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (middle == target) return (middle + UINT32_C(0xdc)) ^ salt;
    uint32_t result;
    if (target < middle) {
        result = walk(target, low, middle - 1, salt);
        return (result * UINT32_C(5)) ^ (middle + salt) ^ UINT32_C(2);
    }
    result = walk(target, middle + 1, high, salt);
    return (result * UINT32_C(5)) ^ (middle + salt) ^ UINT32_C(0x1f);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    uint32_t values[] = {walk((uint32_t)challenge & 127, 0, 127, salt)};
    return write_answer(out, cap, values, 1);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    /* 从二进制静态节点区恢复：value、tag、next 的节点编号。 */
    static const struct {
        uint32_t value;
        uint32_t tag;
        unsigned next;
    } nodes[] = {
        {0x1108, 0x227, 4}, {0xf610, 0x684, 10},
        {0x8fc4, 0x30b, 7}, {0xd5c4, 0x120, 12},
        {0xf07d, 0x923, 1}, {0x80a6, 0x56d, 2},
        {0xff48, 0xa17, 8}, {0x17cb, 0x830, 3},
        {0xd4f5, 0x04f, 5}, {0x5f4f, 0x435, 11},
        {0x5e1e, 0xb20, 9}, {0x47df, 0x77f, 6}
    };
    static const unsigned heads[] = {0, 4, 1, 10, 9, 11, 6, 8};
    unsigned node = heads[challenge & 7];
    unsigned hops = (unsigned)((challenge >> 3) & 3);
    uint32_t value = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t tag = 0;
    for (unsigned i = 0; i <= hops; ++i) {
        value ^= nodes[node].value;
        if (i == 0) value ^= UINT32_C(0x1d9b);
        tag = nodes[node].tag;
        value += tag;
        if (i < hops) node = nodes[node].next;
    }
    uint32_t values[] = {value, tag};
    return write_answer(out, cap, values, 2);
}

static uint32_t rotate_left(uint32_t value, unsigned count) {
    return (value << count) | (value >> (32 - count));
}

static uint32_t apply_operation(unsigned index, uint32_t x) {
    /* 八项函数指针表映射到六个实际运算，部分表项重复。 */
    switch (index) {
        case 0: return (x >> 3) | (x << 29);
        case 1: return (x ^ UINT32_C(0x8eaa)) + (x << 5);
        case 2:
        case 4:
        case 6: return ((x >> 2) ^ x) + UINT32_C(0xca1d);
        case 3: return (x + UINT32_C(0xd8d6)) ^ (x >> 5);
        case 5: return ((x << 7) + UINT32_C(0x1396)) ^ (x * UINT32_C(3));
        default: return rotate_left(x, 1) ^ UINT32_C(0xf45e);
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    x = apply_operation((unsigned)(challenge & 7), x) ^ UINT32_C(0x438);
    x = apply_operation((unsigned)((challenge >> 3) & 7), x) + UINT32_C(0x3dc0);
    uint32_t values[] = {x};
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t folded_challenge = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t folded_context = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t index = (uint32_t)context & 3;
    uint32_t mixed = rotate_left(folded_challenge ^ folded_context, 7);
    mixed = (mixed + UINT32_C(0x3070)) ^ UINT32_C(0xaed9);
    uint32_t second = mixed ^ ((index * UINT32_C(0x3070)) ^ (uint32_t)(context >> 16));
    uint32_t third = second ^ (folded_context + index + UINT32_C(0xaed9)
                              + (folded_challenge >> 7));
    /* 前 32 字节填充；OPEN 覆盖相邻门字段；偏移 36 起放三项口令。 */
    int size = snprintf(out, cap, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32
                        " %" PRIu32 " %" PRIu32, index, second, third);
    return size < 0 || (size_t)size >= cap;
}
