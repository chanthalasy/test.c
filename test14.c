#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define SIZE_COUNT 6
#define REPS 15            /* 시간 측정 반복 횟수 (중앙값 사용) */

int dataSizes[SIZE_COUNT] = {1000, 2000, 4000, 8000, 16000, 20000};

/* ---- 메모리 추정용 구조체 (지역변수/매개변수 크기 기준) ---- */
typedef struct { int root; int child; int end; } HeapWorkspace;
typedef struct { int left; int mid; int right; int i; int j; int k; } MergeFrame;
typedef struct { int low; int high; int pivotIndex; int pivot; int newPivot; } QuickFrame;

typedef struct {
    long long comparisons;
    double timeMs;
    size_t memoryBytes;
    int sorted;
} Result;

enum { ALG_HEAP, ALG_MERGE, ALG_QUICK_FIRST, ALG_QUICK_RANDOM, ALG_QUICK_MEDIAN3 };
enum { FIRST_PIVOT, RANDOM_PIVOT, MEDIAN3_PIVOT };

/* ---- 고해상도 타이머 ---- */
static double nowMs(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}

/* ---- 값 비교 (비교 횟수 1 증가) ---- */
static int compareValues(int a, int b, long long *c) {
    (*c)++;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int isSorted(int arr[], int n) {
    for (int i = 1; i < n; i++)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

static void printLine(void) {
    printf("---------------------------------------------------------------\n");
}

/* ================= 1. 힙 정렬 ================= */
static void heapify(int arr[], int n, int root, long long *c) {
    while (1) {
        int largest = root;
        int left = 2 * root + 1, right = 2 * root + 2;

        if (left < n && compareValues(arr[left], arr[largest], c) > 0)
            largest = left;
        if (right < n && compareValues(arr[right], arr[largest], c) > 0)
            largest = right;
        if (largest == root) break;

        int t = arr[root]; arr[root] = arr[largest]; arr[largest] = t;
        root = largest;
    }
}

static void heapSort(int arr[], int n, long long *c) {
    for (int i = n / 2 - 1; i >= 0; i--) heapify(arr, n, i, c);
    for (int i = n - 1; i > 0; i--) {
        int t = arr[0]; arr[0] = arr[i]; arr[i] = t;
        heapify(arr, i, 0, c);
    }
}

/* ================= 2. 병합 정렬 ================= */
static void merge(int arr[], int temp[], int left, int mid, int right, long long *c) {
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        if (compareValues(arr[i], arr[j], c) <= 0) temp[k++] = arr[i++];
        else temp[k++] = arr[j++];
    }
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];
    for (int x = left; x <= right; x++) arr[x] = temp[x];
}

static void mergeSortRec(int arr[], int temp[], int left, int right,
                         long long *c, int depth, int *maxDepth) {
    if (depth > *maxDepth) *maxDepth = depth;
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortRec(arr, temp, left, mid, c, depth + 1, maxDepth);
    mergeSortRec(arr, temp, mid + 1, right, c, depth + 1, maxDepth);
    merge(arr, temp, left, mid, right, c);
}

/* temp 배열은 호출 전에 할당 (할당 시간은 정렬 시간에 포함하지 않도록 밖에서 처리하지 않고,
   정렬의 일부로 보아 내부에서 할당) */
static void mergeSort(int arr[], int n, long long *c, int *maxDepth) {
    int *temp = (int *)malloc((size_t)n * sizeof(int));
    if (!temp) { printf("Memory allocation failed!\n"); exit(1); }
    *maxDepth = 1;
    mergeSortRec(arr, temp, 0, n - 1, c, 1, maxDepth);
    free(temp);
}

/* ================= 3. 퀵 정렬 ================= */
static int selectPivot(int arr[], int low, int high, int method, long long *c) {
    if (method == FIRST_PIVOT) return low;
    if (method == RANDOM_PIVOT) return low + rand() % (high - low + 1);

    int a = low, b = low + (high - low) / 2, d = high;
    if (compareValues(arr[a], arr[b], c) < 0) {
        if (compareValues(arr[b], arr[d], c) < 0) return b;
        if (compareValues(arr[a], arr[d], c) < 0) return d;
        return a;
    } else {
        if (compareValues(arr[a], arr[d], c) < 0) return a;
        if (compareValues(arr[b], arr[d], c) < 0) return d;
        return b;
    }
}

static int partition(int arr[], int low, int high, int method, long long *c) {
    int p = selectPivot(arr, low, high, method, c);
    int t = arr[p]; arr[p] = arr[high]; arr[high] = t;

    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (compareValues(arr[j], pivot, c) < 0) {
            i++;
            int s = arr[i]; arr[i] = arr[j]; arr[j] = s;
        }
    }
    int s = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = s;
    return i + 1;
}

/* 작은 쪽만 재귀, 큰 쪽은 반복 -> 재귀 깊이 O(log n) */
static void quickSortRec(int arr[], int low, int high, int method,
                         long long *c, int depth, int *maxDepth) {
    while (low < high) {
        if (depth > *maxDepth) *maxDepth = depth;
        int p = partition(arr, low, high, method, c);
        int leftSize = p - low, rightSize = high - p;

        if (leftSize < rightSize) {
            if (low < p - 1)
                quickSortRec(arr, low, p - 1, method, c, depth + 1, maxDepth);
            low = p + 1;
        } else {
            if (p + 1 < high)
                quickSortRec(arr, p + 1, high, method, c, depth + 1, maxDepth);
            high = p - 1;
        }
    }
    if (depth > *maxDepth) *maxDepth = depth;
}

static void quickSort(int arr[], int n, int method, long long *c, int *maxDepth) {
    *maxDepth = 1;
    quickSortRec(arr, 0, n - 1, method, c, 1, maxDepth);
}

/* ================= 4. 성능 측정 ================= */
static int cmpDouble(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static Result runTest(int original[], int n, int alg) {
    Result r;
    double times[REPS];
    int maxDepth = 0;
    int *arr = (int *)malloc((size_t)n * sizeof(int));
    if (!arr) { printf("Memory allocation failed!\n"); exit(1); }

    for (int rep = 0; rep < REPS; rep++) {
        memcpy(arr, original, (size_t)n * sizeof(int));  /* 복사: 시간 제외 */
        r.comparisons = 0;
        maxDepth = 0;
        if (alg == ALG_QUICK_RANDOM) srand(12345);       /* 반복마다 동일한 난수열 */

        double t0 = nowMs();
        switch (alg) {
            case ALG_HEAP:          heapSort(arr, n, &r.comparisons); break;
            case ALG_MERGE:         mergeSort(arr, n, &r.comparisons, &maxDepth); break;
            case ALG_QUICK_FIRST:   quickSort(arr, n, FIRST_PIVOT, &r.comparisons, &maxDepth); break;
            case ALG_QUICK_RANDOM:  quickSort(arr, n, RANDOM_PIVOT, &r.comparisons, &maxDepth); break;
            case ALG_QUICK_MEDIAN3: quickSort(arr, n, MEDIAN3_PIVOT, &r.comparisons, &maxDepth); break;
        }
        times[rep] = nowMs() - t0;
    }

    qsort(times, REPS, sizeof(double), cmpDouble);
    r.timeMs = times[REPS / 2];                          /* 중앙값 */
    r.sorted = isSorted(arr, n);

    if (alg == ALG_HEAP)
        r.memoryBytes = sizeof(HeapWorkspace);
    else if (alg == ALG_MERGE)
        r.memoryBytes = (size_t)n * sizeof(int) + (size_t)maxDepth * sizeof(MergeFrame);
    else
        r.memoryBytes = (size_t)maxDepth * sizeof(QuickFrame);

    free(arr);
    return r;
}

static void printResult(const char *name, Result r) {
    printf("%-18s %15lld %12.4f %15zu   %s\n",
           name, r.comparisons, r.timeMs, r.memoryBytes, r.sorted ? "PASS" : "FAIL");
}

/* ================= 5. MAIN ================= */
int main(void) {
    const char *names[5] = {"Heap Sort", "Merge Sort", "Quick (First)",
                            "Quick (Random)", "Quick (Median3)"};

    srand(2026);
    printf("\n===============================================================\n");
    printf("       HEAP / MERGE / QUICK SORT PERFORMANCE TEST\n");
    printf("===============================================================\n");

    for (int s = 0; s < SIZE_COUNT; s++) {
        int n = dataSizes[s];
        int *original = (int *)malloc((size_t)n * sizeof(int));
        if (!original) { printf("Memory allocation failed!\n"); return 1; }

        for (int i = 0; i < n; i++) original[i] = rand() % 100000;

        printf("\nData Size : %d\n", n);
        printLine();
        printf("%-18s %15s %12s %15s   %s\n",
               "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)", "Check");
        printLine();

        int allOk = 1;
        for (int a = 0; a < 5; a++) {
            Result r = runTest(original, n, a);
            printResult(names[a], r);
            if (!r.sorted) allOk = 0;
        }
        printLine();
        printf(allOk ? "All algorithms passed sorting verification.\n"
                     : "ERROR: Sorting verification failed!\n");
        free(original);
    }

    printf("\n=============== TEST COMPLETED ===============\n");
    return 0;
}
