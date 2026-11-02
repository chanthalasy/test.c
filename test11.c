#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 데이터 값 사이의 비교 횟수 (정렬 시작 전에 0으로 초기화) */
static long long cmp_count;

/* [1] 삽입 정렬 */
static void insertion_sort(int *a, int n)
{
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0) {
            cmp_count++;               /* a[j] > key 비교 1회 */
            if (a[j] > key) {
                a[j + 1] = a[j];
                j--;
            } else {
                break;
            }
        }
        a[j + 1] = key;
    }
}

/* 간격 gap에 대한 삽입 정렬 (쉘 정렬 공통) */
static void gap_insertion(int *a, int n, int gap)
{
    for (int i = gap; i < n; i++) {
        int key = a[i];
        int j = i - gap;
        while (j >= 0) {
            cmp_count++;               /* a[j] > key 비교 1회 */
            if (a[j] > key) {
                a[j + gap] = a[j];
                j -= gap;
            } else {
                break;
            }
        }
        a[j + gap] = key;
    }
}

/* [2] 쉘 정렬 - 기본 간격: n/2, n/4, ..., 1 */
static void shell_sort_original(int *a, int n)
{
    for (int gap = n / 2; gap >= 1; gap /= 2)
        gap_insertion(a, n, gap);
}

/* [3] 쉘 정렬 - Knuth 간격: h(1)=1, h(k+1)=3h(k)+1 */
static void shell_sort_knuth(int *a, int n)
{
    int h = 1;
    while (3 * h + 1 < n)              /* n보다 작은 가장 큰 간격 찾기 */
        h = 3 * h + 1;
    for (; h >= 1; h = (h - 1) / 3)    /* 역순 적용: h = (h-1)/3 */
        gap_insertion(a, n, h);
}

static int is_sorted(const int *a, int n)
{
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

int main(void)
{
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    srand(12345);                      /* 재현 가능한 실험을 위한 고정 시드 */

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        int *orig = malloc(sizeof(int) * n);
        int *work = malloc(sizeof(int) * n);

        /* 원본 데이터 생성 (측정 제외) */
        for (int i = 0; i < n; i++)
            orig[i] = rand();

        /* 삽입 정렬 */
        memcpy(work, orig, sizeof(int) * n);
        cmp_count = 0;
        insertion_sort(work, n);
        long long c_ins = cmp_count;
        int ok_ins = is_sorted(work, n);

        /* 쉘 정렬 (기본 간격) */
        memcpy(work, orig, sizeof(int) * n);
        cmp_count = 0;
        shell_sort_original(work, n);
        long long c_org = cmp_count;
        int ok_org = is_sorted(work, n);

        /* 쉘 정렬 (Knuth 간격) */
        memcpy(work, orig, sizeof(int) * n);
        cmp_count = 0;
        shell_sort_knuth(work, n);
        long long c_knu = cmp_count;
        int ok_knu = is_sorted(work, n);

        printf("Data Size : %d\n\n", n);
        printf("Insertion Sort       : %lld comparisons\n", c_ins);
        printf("Shell Sort (Original): %lld comparisons\n", c_org);
        printf("Shell Sort (Knuth)   : %lld comparisons\n", c_knu);
        if (!(ok_ins && ok_org && ok_knu))
            printf("[ERROR] 정렬 결과가 올바르지 않습니다.\n");
        printf("\n");

        free(orig);
        free(work);
    }
    return 0;
}
