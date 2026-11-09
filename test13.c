#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
 
#define FIRST   0
#define RANDOM  1
#define MEDIAN3 2
 
static long long comparisons;
 
/* ---- 메모리 측정: 재귀 호출 스택은 실제 프레임 주소 차이로, 힙은 할당 바이트로 ---- */
static char *stack_base;     /* 정렬 시작 시점의 스택 주소 */
static char *stack_min;      /* 가장 깊은 호출에서의 스택 주소 */
static size_t heap_bytes;    /* 정렬 중 추가로 할당한 힙 바이트 */
 
static inline void track_stack(void) {
    char *p = (char *)__builtin_frame_address(0);
    if (p < stack_min) stack_min = p;
}
 
/* 재현 가능한 난수 (xorshift32) */
static unsigned int rng_state = 12345u;
static unsigned int next_rand(void) {
    unsigned int x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rng_state = x;
}
 
static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }
 
/* ================= 병합 정렬 ================= */
static void merge(int *a, int *tmp, int lo, int mid, int hi) {
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) {
        comparisons++;                       /* 두 값의 비교 */
        if (a[i] <= a[j]) tmp[k++] = a[i++];
        else              tmp[k++] = a[j++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi)  tmp[k++] = a[j++];
    memcpy(a + lo, tmp + lo, sizeof(int) * (size_t)(hi - lo + 1));
}
 
static void merge_rec(int *a, int *tmp, int lo, int hi) {
    track_stack();
    if (lo >= hi) return;
    int mid = lo + (hi - lo) / 2;
    merge_rec(a, tmp, lo, mid);
    merge_rec(a, tmp, mid + 1, hi);
    merge(a, tmp, lo, mid, hi);
}
 
static void merge_sort(int *a, int n) {
    int *tmp = (int *)malloc(sizeof(int) * (size_t)n);   /* 임시 배열 */
    heap_bytes += sizeof(int) * (size_t)n;
    merge_rec(a, tmp, 0, n - 1);
    free(tmp);
}
 
/* ================= 퀵 정렬 ================= */
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
 
static int partition(int *a, int lo, int hi, int method) {
    int idx;
    if (method == RANDOM)       idx = lo + (int)(next_rand() % (unsigned)(hi - lo + 1));
    else if (method == MEDIAN3) idx = median_index(a, lo, lo + (hi - lo) / 2, hi);
    else                        idx = lo;
    swap(&a[lo], &a[idx]);
    int pivot = a[lo], i = lo;
    for (int j = lo + 1; j <= hi; j++) {
        comparisons++;                       /* 데이터 값 vs 피벗 */
        if (a[j] < pivot) { i++; swap(&a[i], &a[j]); }
    }
    swap(&a[lo], &a[i]);
    return i;
}
 
static void quick_rec(int *a, int lo, int hi, int method) {
    track_stack();
    if (lo >= hi) return;
    int p = partition(a, lo, hi, method);
    quick_rec(a, lo, p - 1, method);
    quick_rec(a, p + 1, hi, method);
}
 
/* ================= 공통 도구 ================= */
static int is_sorted(const int *a, int n) {
    for (int i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0;
    return 1;
}
 
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
 
/* algo: 0=Merge, 1=Quick First, 2=Quick Random, 3=Quick Median3 */
static void run_algo(int algo, int *a, int n) {
    rng_state = 12345u;
    if (algo == 0) merge_sort(a, n);
    else           quick_rec(a, 0, n - 1, algo - 1);
}
 
int main(void) {
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const char *names[] = {"Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)"};
    const int REPEAT = 200;                  /* 시간은 동일 조건에서 200회 평균 */
    srand(2026);
 
    for (int s = 0; s < 6; s++) {
        int n = sizes[s];
        int *orig = (int *)malloc(sizeof(int) * n);
        int *work = (int *)malloc(sizeof(int) * n);
        for (int i = 0; i < n; i++) orig[i] = rand() % 100000;   /* 중복 허용 */
 
        printf("Data Size : %d\n\n", n);
        printf("%-16s%-15s%-12s%s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");
 
        for (int algo = 0; algo < 4; algo++) {
            /* 1) 비교 횟수 + 메모리 측정 (1회) */
            memcpy(work, orig, sizeof(int) * n);
            comparisons = 0; heap_bytes = 0;
            char marker;
            stack_base = &marker;
            stack_min = stack_base;
            run_algo(algo, work, n);
            if (!is_sorted(work, n)) { fprintf(stderr, "Sort failed: %s\n", names[algo]); return 1; }
            long long cmp = comparisons;
            size_t stack_bytes = (size_t)(stack_base - stack_min);
            size_t mem = heap_bytes + stack_bytes;
 
            /* 2) 실행시간 측정 (복사 시간 제외, REPEAT회 평균) */
            double total = 0;
            for (int r = 0; r < REPEAT; r++) {
                memcpy(work, orig, sizeof(int) * n);
                double t0 = now_ms();
                run_algo(algo, work, n);
                total += now_ms() - t0;
            }
            printf("%-16s%-15lld%-12.4f%zu\n", names[algo], cmp, total / REPEAT, mem);
        }
        printf("\n");
        free(orig); free(work);
    }
    return 0;
}
 
