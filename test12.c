#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
 
#define FIRST   0
#define RANDOM  1
#define MEDIAN3 2
 
static long long comparisons = 0;   /* 비교 횟수 */
 
/* 재현 가능한 난수 (xorshift32) */
static unsigned int rng_state = 12345u;
static unsigned int next_rand(void) {
    unsigned int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}
 
static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }
 
/* 세 값의 중앙값 위치 (비교 2~3회, 모두 집계) */
static int median_index(const int *a, int i, int j, int k) {
    comparisons++;
    if (a[i] < a[j]) {
        comparisons++;
        if (a[j] < a[k]) return j;
        comparisons++;
        return (a[i] < a[k]) ? k : i;
    } else {
        comparisons++;
        if (a[i] < a[k]) return i;
        comparisons++;
        return (a[j] < a[k]) ? k : j;
    }
}
 
/* 피벗을 골라 a[lo]로 옮긴다 (세 방법에서 이 부분만 다름) */
static void choose_pivot(int *a, int lo, int hi, int method) {
    int idx;
    if (method == RANDOM)
        idx = lo + (int)(next_rand() % (unsigned)(hi - lo + 1)); /* 비교 횟수 제외 */
    else if (method == MEDIAN3)
        idx = median_index(a, lo, lo + (hi - lo) / 2, hi);
    else
        idx = lo;
    swap(&a[lo], &a[idx]);
}
 
/* 분할 (세 방법 모두 동일) */
static int partition(int *a, int lo, int hi, int method) {
    choose_pivot(a, lo, hi, method);
    int pivot = a[lo];
    int i = lo;
    for (int j = lo + 1; j <= hi; j++) {
        comparisons++;                 /* 데이터 값 vs 피벗 값 */
        if (a[j] < pivot) {
            i++;
            swap(&a[i], &a[j]);
        }
    }
    swap(&a[lo], &a[i]);
    return i;
}
 
/* 재귀적 퀵 정렬 */
static void quick_recursive(int *a, int lo, int hi, int method) {
    if (lo >= hi) return;
    int p = partition(a, lo, hi, method);
    quick_recursive(a, lo, p - 1, method);
    quick_recursive(a, p + 1, hi, method);
}
 
/* 반복적 퀵 정렬 (명시적 스택) */
static void quick_iterative(int *a, int n, int method) {
    int *stack = (int *)malloc(sizeof(int) * (2 * (size_t)n + 4));
    int top = -1;
    stack[++top] = 0;
    stack[++top] = n - 1;
    while (top >= 0) {
        int hi = stack[top--];
        int lo = stack[top--];
        if (lo >= hi) continue;
        int p = partition(a, lo, hi, method);
        /* 재귀와 같은 순서(왼쪽 먼저)가 되도록 오른쪽을 먼저 push */
        stack[++top] = p + 1;
        stack[++top] = hi;
        stack[++top] = lo;
        stack[++top] = p - 1;
    }
    free(stack);
}
 
static int is_sorted(const int *a, int n) {
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}
 
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
 
int main(void) {
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const char *names[] = {"First   ", "Random  ", "Median-3"};
    srand(2026);
 
    for (int s = 0; s < 6; s++) {
        int n = sizes[s];
        int *original = (int *)malloc(sizeof(int) * n);
        int *copy = (int *)malloc(sizeof(int) * n);
        for (int i = 0; i < n; i++) original[i] = rand() % 100000; /* 중복 허용 */
 
        printf("Data Size : %d\n\n", n);
        for (int m = 0; m < 3; m++) {
            memcpy(copy, original, sizeof(int) * n);  /* 동일한 원본의 복사본 */
            comparisons = 0;
            rng_state = 12345u;
            quick_recursive(copy, 0, n - 1, m);
            if (!is_sorted(copy, n)) {
                fprintf(stderr, "정렬 실패: %s\n", names[m]);
                return 1;
            }
            printf("Quick Sort (%s) : %lld comparisons\n", names[m], comparisons);
        }
        printf("\n");
        free(original);
        free(copy);
    }
 
    /* ---------- 보너스: 재귀 vs 반복 실행시간 ---------- */
    int n = 10000, rounds = 500;
    int *original = (int *)malloc(sizeof(int) * n);
    int *c1 = (int *)malloc(sizeof(int) * n);
    int *c2 = (int *)malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) original[i] = rand() % 100000;
 
    double rec_total = 0, itr_total = 0;
    for (int r = 0; r < rounds; r++) {
        memcpy(c1, original, sizeof(int) * n);   /* 복사 시간은 측정 제외 */
        memcpy(c2, original, sizeof(int) * n);
 
        rng_state = 12345u;
        double t0 = now_ms();
        quick_recursive(c1, 0, n - 1, MEDIAN3);
        rec_total += now_ms() - t0;
 
        rng_state = 12345u;
        t0 = now_ms();
        quick_iterative(c2, n, MEDIAN3);
        itr_total += now_ms() - t0;
 
        if (!is_sorted(c1, n) || !is_sorted(c2, n)) {
            fprintf(stderr, "정렬 실패\n");
            return 1;
        }
    }
 
    printf("Data Size : %d\n", n);
    printf("Pivot     : Median-of-Three\n\n");
    printf("Recursive Quick Sort : %.4f ms\n", rec_total / rounds);
    printf("Iterative Quick Sort : %.4f ms\n", itr_total / rounds);
    printf("(average of %d runs)\n", rounds);
 
    free(original);
    free(c1);
    free(c2);
    return 0;
}
