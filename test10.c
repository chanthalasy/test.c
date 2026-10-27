#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N          10000      /* 데이터 개수 */
#define TRIALS     100        /* 무작위 반복 횟수 */
#define KEY_MAX    1000000    /* 키 범위: 0 ~ 1,000,000 */
#define SEED       20240601u  /* 재현 가능한 실험을 위한 고정 시드 */

#define STAB_N     1000       /* 안정성 검증용 데이터 개수 */
#define STAB_RANGE 50         /* 안정성 검증용 Key1 범위 (0~49, 중복 다수) */

#define PARTIAL_SWAPS (N / 20) /* 부분 정렬: 정렬된 데이터에서 5% 만큼 무작위 교환 */

typedef struct {
    int key1;   /* 정렬 기준 키 */
    int key2;   /* 정렬에 사용하지 않는 키 */
    int order;  /* 원래 입력 순서 (안정성 검증용, 정렬 기준 아님) */
} Data;

typedef struct {
    unsigned long long cmp;  /* Key1 비교 횟수 */
    unsigned long long mov;  /* 데이터 이동 횟수 */
} Stats;

/* 알고리즘 인덱스 */
enum { SEL = 0, BUB, BUBE, INS_GT, INS_GE, ALGO_COUNT };
static const char *ALGO_NAME[ALGO_COUNT] = {
    "Selection Sort",
    "Bubble Sort",
    "Bubble Sort (Early Termination)",
    "Insertion Sort (>)",
    "Insertion Sort (>=)"
};

/* ------------------------------------------------------------------ */
/* 정렬 알고리즘                                                        */
/* ------------------------------------------------------------------ */

/* 선택 정렬: 최소값 위치가 현재 위치와 다를 때만 교환(이동 3회) */
static void selection_sort(Data *a, int n, Stats *s)
{
    for (int i = 0; i < n - 1; i++) {
        int min = i;
        for (int j = i + 1; j < n; j++) {
            s->cmp++;
            if (a[j].key1 < a[min].key1)
                min = j;
        }
        if (min != i) {
            Data temp = a[i];  s->mov++;
            a[i] = a[min];     s->mov++;
            a[min] = temp;     s->mov++;
        }
    }
}

/* 버블 정렬: early == 0 이면 모든 순회 수행, early == 1 이면 교환 없는 순회에서 종료 */
static void bubble_sort(Data *a, int n, Stats *s, int early)
{
    for (int pass = 0; pass < n - 1; pass++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - pass; j++) {
            s->cmp++;
            if (a[j].key1 > a[j + 1].key1) {
                Data temp = a[j];   s->mov++;
                a[j] = a[j + 1];    s->mov++;
                a[j + 1] = temp;    s->mov++;
                swapped = 1;
            }
        }
        if (early && !swapped)
            break;
    }
}

/* 삽입 정렬: use_ge == 0 -> Case A (data[j].key1 >  current.key1)
 *            use_ge == 1 -> Case B (data[j].key1 >= current.key1) */
static void insertion_sort(Data *a, int n, Stats *s, int use_ge)
{
    for (int i = 1; i < n; i++) {
        Data current = a[i];  s->mov++;          /* 임시 변수에 저장 */
        int j = i - 1;
        while (j >= 0) {
            int shift;
            s->cmp++;
            if (use_ge)
                shift = (a[j].key1 >= current.key1);
            else
                shift = (a[j].key1 >  current.key1);
            if (!shift)
                break;
            a[j + 1] = a[j];  s->mov++;          /* 한 칸 이동 */
            j--;
        }
        a[j + 1] = current;   s->mov++;          /* 최종 위치에 대입 */
    }
}

static void run_algo(int algo, Data *a, int n, Stats *s)
{
    s->cmp = 0;
    s->mov = 0;
    switch (algo) {
    case SEL:    selection_sort(a, n, s);      break;
    case BUB:    bubble_sort(a, n, s, 0);      break;
    case BUBE:   bubble_sort(a, n, s, 1);      break;
    case INS_GT: insertion_sort(a, n, s, 0);   break;
    case INS_GE: insertion_sort(a, n, s, 1);   break;
    }
}

/* ------------------------------------------------------------------ */
/* 검증 / 보조 함수                                                     */
/* ------------------------------------------------------------------ */

/* Key1 오름차순이며, 원본 데이터의 순열(order 번호가 0..n-1 한 번씩)인지 확인 */
static int verify_sorted(const Data *a, int n)
{
    char *seen = calloc((size_t)n, 1);
    int ok = 1;
    for (int i = 0; i < n && ok; i++) {
        if (i > 0 && a[i - 1].key1 > a[i].key1) ok = 0;
        if (a[i].order < 0 || a[i].order >= n || seen[a[i].order]) ok = 0;
        else seen[a[i].order] = 1;
    }
    free(seen);
    return ok;
}

/* 같은 Key1을 가진 인접 쌍 중 원래 순서가 뒤집힌 개수 (0이면 안정) */
static int count_stability_violations(const Data *a, int n)
{
    int v = 0;
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 == a[i + 1].key1 && a[i].order > a[i + 1].order)
            v++;
    return v;
}

static int rand_key(int range_max)
{
    /* rand()가 15비트인 환경에서도 0~1,000,000을 고르게 만들기 위해 조합 */
    unsigned long long r = ((unsigned long long)rand() << 15) ^ (unsigned long long)rand();
    r = (r << 15) ^ (unsigned long long)rand();
    return (int)(r % ((unsigned long long)range_max + 1ULL));
}

static void generate_random(Data *a, int n, int key1_max)
{
    for (int i = 0; i < n; i++) {
        a[i].key1  = rand_key(key1_max);
        a[i].key2  = rand_key(KEY_MAX);
        a[i].order = i;
    }
}

/* 시험 입력 준비용 사전 정렬 (qsort 사용, 비교/이동 횟수에 포함하지 않음)
 * Key1 오름차순, Key1이 같으면 원래 순서(order) 오름차순 */
static int cmp_asc(const void *x, const void *y)
{
    const Data *p = x, *q = y;
    if (p->key1 != q->key1) return (p->key1 < q->key1) ? -1 : 1;
    return (p->order < q->order) ? -1 : (p->order > q->order);
}

static void print_stats_block(const char *title, const Stats *r, const int *ok)
{
    printf("%s\n", title);
    printf("Comparisons : %llu\n", r->cmp);
    printf("Movements   : %llu\n", r->mov);
    printf("Sorted OK   : %s\n\n", *ok ? "YES" : "NO");
}

/* 한 입력 상태에 대해 5가지 알고리즘을 모두 수행하고 결과 출력 */
static void run_case(const char *label, const Data *base, int n)
{
    Data *work = malloc(sizeof(Data) * (size_t)n);
    printf("%s\n\n", label);
    for (int k = 0; k < ALGO_COUNT; k++) {
        Stats s;
        memcpy(work, base, sizeof(Data) * (size_t)n);  /* 원본 복사: 횟수에 포함 X */
        run_algo(k, work, n, &s);
        int ok = verify_sorted(work, n);
        print_stats_block(ALGO_NAME[k], &s, &ok);
    }
    free(work);
    printf("------------------------------------\n\n");
}

/* ------------------------------------------------------------------ */
/* 안정성 검증                                                          */
/* ------------------------------------------------------------------ */

static void print_small(const char *title, const Data *a, int n)
{
    printf("  %-20s:", title);
    for (int i = 0; i < n; i++)
        printf(" (%d,%d)", a[i].key1, a[i].key2);
    printf("\n");
}

static void stability_verification(void)
{
    printf("Stability Verification\n\n");

    /* (1) 과제 문서의 예시 데이터 4개로 직접 눈으로 확인 */
    Data ex[4] = { {527, 100, 0}, {527, 200, 1}, {125, 300, 2}, {892, 400, 3} };
    printf("[Small example from the assignment]\n");
    print_small("Original", ex, 4);
    for (int k = 0; k < ALGO_COUNT; k++) {
        Data t[4];
        Stats s;
        memcpy(t, ex, sizeof(ex));
        run_algo(k, t, 4, &s);
        print_small(ALGO_NAME[k], t, 4);
    }
    printf("\n");

    /* (2) 중복이 많은 대량 시험 데이터: Key1 = 0~49, 1000개 */
    Data *base = malloc(sizeof(Data) * STAB_N);
    Data *work = malloc(sizeof(Data) * STAB_N);
    generate_random(base, STAB_N, STAB_RANGE - 1);

    int same_pairs = 0;
    {
        Data *tmp = malloc(sizeof(Data) * STAB_N);
        memcpy(tmp, base, sizeof(Data) * STAB_N);
        qsort(tmp, STAB_N, sizeof(Data), cmp_asc);
        for (int i = 0; i + 1 < STAB_N; i++)
            if (tmp[i].key1 == tmp[i + 1].key1) same_pairs++;
        free(tmp);
    }
    printf("[Large duplicate-key test] n=%d, Key1 range 0..%d, "
           "adjacent equal-key pairs after sorting = %d\n",
           STAB_N, STAB_RANGE - 1, same_pairs);

    for (int k = 0; k < ALGO_COUNT; k++) {
        Stats s;
        memcpy(work, base, sizeof(Data) * STAB_N);
        run_algo(k, work, STAB_N, &s);
        int sorted = verify_sorted(work, STAB_N);
        int viol = count_stability_violations(work, STAB_N);
        printf("%-32s : %s (sorted=%s, order violations=%d)\n",
               ALGO_NAME[k],
               viol == 0 ? "STABLE" : "UNSTABLE",
               sorted ? "YES" : "NO", viol);
    }
    printf("\n");

    free(base);
    free(work);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    srand(SEED);

    Data *base = malloc(sizeof(Data) * N);
    Data *work = malloc(sizeof(Data) * N);

    printf("Data Count : %d\n\n", N);

    /* ---------- 1) 무작위 데이터 100회 반복 ---------- */
    Stats total[ALGO_COUNT];
    memset(total, 0, sizeof(total));
    int all_ok = 1;

    for (int t = 0; t < TRIALS; t++) {
        generate_random(base, N, KEY_MAX);         /* 한 반복당 하나의 난수 데이터 */
        for (int k = 0; k < ALGO_COUNT; k++) {
            Stats s;
            memcpy(work, base, sizeof(Data) * N);  /* 동일한 복사본 제공 */
            run_algo(k, work, N, &s);
            if (!verify_sorted(work, N)) all_ok = 0;
            total[k].cmp += s.cmp;
            total[k].mov += s.mov;
        }
    }

    printf("Random Data\n");
    printf("Number of Trials : %d\n", TRIALS);
    printf("All results sorted correctly : %s\n\n", all_ok ? "YES" : "NO");
    for (int k = 0; k < ALGO_COUNT; k++) {
        printf("%s\n", ALGO_NAME[k]);
        printf("Total Comparisons   : %llu\n", total[k].cmp);
        printf("Average Comparisons : %.2f\n", (double)total[k].cmp / TRIALS);
        printf("Total Movements     : %llu\n", total[k].mov);
        printf("Average Movements   : %.2f\n\n", (double)total[k].mov / TRIALS);
    }
    printf("------------------------------------\n\n");

    /* ---------- 2) 정렬 / 역순 / 부분 정렬 (동일한 원본에서 생성) ---------- */
    Data *sorted_d  = malloc(sizeof(Data) * N);
    Data *reverse_d = malloc(sizeof(Data) * N);
    Data *partial_d = malloc(sizeof(Data) * N);

    generate_random(base, N, KEY_MAX);
    memcpy(sorted_d, base, sizeof(Data) * N);
    qsort(sorted_d, N, sizeof(Data), cmp_asc);              /* 사전 정렬 (카운트 제외) */

    for (int i = 0; i < N; i++)                              /* 역순: 정렬본을 뒤집음 */
        reverse_d[i] = sorted_d[N - 1 - i];

    memcpy(partial_d, sorted_d, sizeof(Data) * N);           /* 부분 정렬: 5% 무작위 교환 */
    for (int c = 0; c < PARTIAL_SWAPS; c++) {
        int p = rand() % N, q = rand() % N;
        Data tmp = partial_d[p];
        partial_d[p] = partial_d[q];
        partial_d[q] = tmp;
    }

    run_case("Sorted Data",           sorted_d,  N);
    run_case("Reverse-Sorted Data",   reverse_d, N);
    printf("(Partially sorted: sorted data + %d random pair swaps = %.0f%% of N)\n\n",
           PARTIAL_SWAPS, 100.0 * PARTIAL_SWAPS / N);
    run_case("Partially Sorted Data", partial_d, N);

    /* ---------- 3) 안정성 검증 ---------- */
    stability_verification();

    free(base); free(work);
    free(sorted_d); free(reverse_d); free(partial_d);
    return 0;
}
