#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define SIZE_COUNT 6
#define REPS 11   /* 시간 측정 반복 횟수 (중앙값 사용) */

static int dataSizes[SIZE_COUNT] = {1000, 2000, 4000, 8000, 16000, 20000};

static long long gComp = 0;     
static long long gMov = 0;       
static int gMaxDepth = 0;    
static long gNodes = 0;          
static long gPeakNodes = 0;     


typedef struct { int low; int high; int pivotIndex; int pivot; int newPivot; } QuickFrame;

typedef struct AvlNode {
    int value;
    int count;                 
    int height;
    struct AvlNode *left, *right;
} AvlNode;

typedef struct { AvlNode *node; int value; int depth; } AvlFrame;

typedef struct {
    double tBuild, tOps;
    long long cBuild, cOps, mBuild, mOps;
    size_t mem;
    long finalCount;
    int ins, delDone, delSkip;
    int ok;
} Run;


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


static int compareValues(int a, int b) {
    gComp++;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int isSorted(const int *a, int n) {
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

static int selectMedian3(int arr[], int low, int high) {
    int a = low, b = low + (high - low) / 2, d = high;
    if (compareValues(arr[a], arr[b]) < 0) {
        if (compareValues(arr[b], arr[d]) < 0) return b;
        if (compareValues(arr[a], arr[d]) < 0) return d;
        return a;
    } else {
        if (compareValues(arr[a], arr[d]) < 0) return a;
        if (compareValues(arr[b], arr[d]) < 0) return d;
        return b;
    }
}


#define SWAP(x, y) do { int t_ = (x); (x) = (y); (y) = t_; gMov += 3; } while (0)

static int partition(int arr[], int low, int high) {
    int p = selectMedian3(arr, low, high);
    SWAP(arr[p], arr[high]);
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (compareValues(arr[j], pivot) < 0) {
            i++;
            SWAP(arr[i], arr[j]);
        }
    }
    SWAP(arr[i + 1], arr[high]);
    return i + 1;
}


static void quickRec(int arr[], int low, int high, int depth) {
    while (low < high) {
        if (depth > gMaxDepth) gMaxDepth = depth;
        int p = partition(arr, low, high);
        if (p - low < high - p) {
            if (low < p - 1) quickRec(arr, low, p - 1, depth + 1);
            low = p + 1;
        } else {
            if (p + 1 < high) quickRec(arr, p + 1, high, depth + 1);
            high = p - 1;
        }
    }
}


static void arrayInsert(int arr[], int *len, int v) {
    int i = 0;
    while (i < *len && compareValues(arr[i], v) < 0) i++;   
    for (int k = *len; k > i; k--) { arr[k] = arr[k - 1]; gMov++; }
    arr[i] = v; gMov++;
    (*len)++;
}

static int arrayDelete(int arr[], int *len, int v) {
    int i = 0;
    while (i < *len) {
        int r = compareValues(arr[i], v);
        if (r < 0) { i++; continue; }
        if (r > 0) return 0;          
        break;                       
    }
    if (i >= *len) return 0;
    for (int k = i; k < *len - 1; k++) { arr[k] = arr[k + 1]; gMov++; }
    (*len)--;
    return 1;
}

static void runQuick(int buf[], int n, const int opT[], const int opV[], int m,
                     int verify, Run *r) {
    memset(r, 0, sizeof(*r));
    r->ok = 1;
    gComp = gMov = 0;
    gMaxDepth = 1;

    double t0 = nowMs();
    quickRec(buf, 0, n - 1, 1);
    double t1 = nowMs();
    r->cBuild = gComp; r->mBuild = gMov;
    if (verify && !isSorted(buf, n)) r->ok = 0;

    int len = n, peak = n;
    gComp = gMov = 0;
    double t2 = nowMs();
    for (int i = 0; i < m; i++) {
        if (opT[i] == 0) {                       
            arrayInsert(buf, &len, opV[i]);
            r->ins++;
            if (len > peak) peak = len;
        } else {                                 /* 삭제 */
            if (arrayDelete(buf, &len, opV[i])) r->delDone++;
            else r->delSkip++;
        }
    }
    double t3 = nowMs();
    r->cOps = gComp; r->mOps = gMov;
    r->tBuild = t1 - t0; r->tOps = t3 - t2;
    r->finalCount = len;
    r->mem = (size_t)peak * sizeof(int) + (size_t)gMaxDepth * sizeof(QuickFrame);
    if (verify && !isSorted(buf, len)) r->ok = 0;
}


static int hgt(AvlNode *n) { return n ? n->height : 0; }
static void upd(AvlNode *n) {
    int a = hgt(n->left), b = hgt(n->right);
    n->height = 1 + (a > b ? a : b);
}

static AvlNode *rotR(AvlNode *y) {
    AvlNode *x = y->left;
    y->left = x->right;
    x->right = y;
    upd(y); upd(x);
    return x;
}
static AvlNode *rotL(AvlNode *x) {
    AvlNode *y = x->right;
    x->right = y->left;
    y->left = x;
    upd(x); upd(y);
    return y;
}

static AvlNode *balance(AvlNode *n) {
    upd(n);
    int bf = hgt(n->left) - hgt(n->right);   /* 균형 인수: 제어용 비교, 카운트 제외 */
    if (bf > 1) {
        if (hgt(n->left->left) < hgt(n->left->right)) n->left = rotL(n->left);
        return rotR(n);
    }
    if (bf < -1) {
        if (hgt(n->right->right) < hgt(n->right->left)) n->right = rotR(n->right);
        return rotL(n);
    }
    return n;
}

static AvlNode *avlInsert(AvlNode *n, int v, int depth) {
    if (depth > gMaxDepth) gMaxDepth = depth;
    if (!n) {
        AvlNode *t = (AvlNode *)malloc(sizeof(AvlNode));
        if (!t) { printf("Memory allocation failed!\n"); exit(1); }
        t->value = v; t->count = 1; t->height = 1; t->left = t->right = NULL;
        gMov++;                                 /* 값 저장 */
        gNodes++;
        if (gNodes > gPeakNodes) gPeakNodes = gNodes;
        return t;
    }
    int r = compareValues(v, n->value);
    if (r == 0) { n->count++; gMov++; return n; }   /* 중복: 카운트 갱신 */
    if (r < 0) n->left = avlInsert(n->left, v, depth + 1);
    else n->right = avlInsert(n->right, v, depth + 1);
    return balance(n);
}

/* 최솟값 노드를 분리 (값 비교 없음) */
static AvlNode *removeMin(AvlNode *n, AvlNode **minOut, int depth) {
    if (depth > gMaxDepth) gMaxDepth = depth;
    if (!n->left) { *minOut = n; return n->right; }
    n->left = removeMin(n->left, minOut, depth + 1);
    return balance(n);
}

static AvlNode *avlDelete(AvlNode *n, int v, int depth, int *deleted) {
    if (depth > gMaxDepth) gMaxDepth = depth;
    if (!n) return NULL;
    int r = compareValues(v, n->value);
    if (r < 0) n->left = avlDelete(n->left, v, depth + 1, deleted);
    else if (r > 0) n->right = avlDelete(n->right, v, depth + 1, deleted);
    else {
        *deleted = 1;
        if (n->count > 1) { n->count--; gMov++; return n; }   /* 중복: 하나만 제거 */
        if (!n->left || !n->right) {
            AvlNode *c = n->left ? n->left : n->right;
            free(n); gNodes--;
            return c;
        }
        AvlNode *mn;
        n->right = removeMin(n->right, &mn, depth + 1);
        n->value = mn->value;                    /* 후속자의 값과 count 복사 */
        n->count = mn->count;
        gMov += 2;
        free(mn); gNodes--;
    }
    return balance(n);
}

static void freeTree(AvlNode *n) {
    if (!n) return;
    freeTree(n->left); freeTree(n->right); free(n);
}

static long inorder(AvlNode *n, int *out, long pos) {
    if (!n) return pos;
    pos = inorder(n->left, out, pos);
    for (int i = 0; i < n->count; i++) out[pos++] = n->value;
    return inorder(n->right, out, pos);
}

static void runAvl(const int orig[], int n, const int opT[], const int opV[], int m,
                   int verify, const int qFinal[], int qLen, int *tmp, Run *r) {
    memset(r, 0, sizeof(*r));
    r->ok = 1;
    gComp = gMov = 0;
    gMaxDepth = 1;
    gNodes = gPeakNodes = 0;
    AvlNode *root = NULL;

    double t0 = nowMs();
    for (int i = 0; i < n; i++) root = avlInsert(root, orig[i], 1);
    double t1 = nowMs();
    r->cBuild = gComp; r->mBuild = gMov;
    if (verify) {                                   /* 중위 순회로 오름차순 확인 */
        long c = inorder(root, tmp, 0);
        if (c != n || !isSorted(tmp, (int)c)) r->ok = 0;
    }

    gComp = gMov = 0;
    double t2 = nowMs();
    for (int i = 0; i < m; i++) {
        if (opT[i] == 0) {
            root = avlInsert(root, opV[i], 1);
            r->ins++;
        } else {
            int deleted = 0;
            root = avlDelete(root, opV[i], 1, &deleted);
            if (deleted) r->delDone++; else r->delSkip++;
        }
    }
    double t3 = nowMs();
    r->cOps = gComp; r->mOps = gMov;
    r->tBuild = t1 - t0; r->tOps = t3 - t2;
    r->mem = (size_t)gPeakNodes * sizeof(AvlNode) + (size_t)gMaxDepth * sizeof(AvlFrame);

    long cnt = inorder(root, tmp, 0);               /* 최종 데이터 개수 (검증용, 시간 제외) */
    r->finalCount = cnt;
    if (verify) {
        if (!isSorted(tmp, (int)cnt) || cnt != qLen ||
            memcmp(tmp, qFinal, (size_t)cnt * sizeof(int)) != 0) r->ok = 0;
    }
    freeTree(root);
}

/* =====================================================
   3. 측정 / 출력
   ===================================================== */
static int cmpDouble(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static double median(double *v, int k) {
    qsort(v, k, sizeof(double), cmpDouble);
    return v[k / 2];
}

int main(void) {
    srand(2026);
    printf("===============================================================\n");
    printf("     QUICK SORT(ARRAY) vs AVL TREE : SORT + INSERT/DELETE\n");
    printf("===============================================================\n");

    for (int s = 0; s < SIZE_COUNT; s++) {
        int n = dataSizes[s];
        int m = n / 5;

        int *orig = (int *)malloc((size_t)n * sizeof(int));
        int *opT = (int *)malloc((size_t)m * sizeof(int));
        int *opV = (int *)malloc((size_t)m * sizeof(int));
        int *qbuf = (int *)malloc((size_t)(n + m) * sizeof(int));   /* 삽입 대비 여유 공간 */
        int *tmp = (int *)malloc((size_t)(n + m) * sizeof(int));
        if (!orig || !opT || !opV || !qbuf || !tmp) { printf("alloc fail\n"); return 1; }

        /* 데이터 및 연산 사전 생성 (시간 제외) */
        for (int i = 0; i < n; i++) orig[i] = rand() % 100000;
        for (int i = 0; i < m; i++) {
            opT[i] = (rand() % 2 == 0) ? 0 : 1;      /* 짝수: 삽입(0), 홀수: 삭제(1) */
            opV[i] = rand() % 100000;                /* 실제 값은 새 난수 */
        }

        /* (1) 검증 실행: 비교/이동/메모리/개수 결정 (결정적이라 반복해도 동일) */
        Run qv, av;
        memcpy(qbuf, orig, (size_t)n * sizeof(int));
        runQuick(qbuf, n, opT, opV, m, 1, &qv);
        runAvl(orig, n, opT, opV, m, 1, qbuf, (int)qv.finalCount, tmp, &av);

        /* (2) 시간 측정: 반복 후 중앙값 */
        double qT[REPS], aT[REPS], qB[REPS], qO[REPS], aB[REPS], aO[REPS];
        for (int k = 0; k < REPS; k++) {
            Run r;
            memcpy(qbuf, orig, (size_t)n * sizeof(int));   /* 복사 시간 제외 */
            runQuick(qbuf, n, opT, opV, m, 0, &r);
            qB[k] = r.tBuild; qO[k] = r.tOps; qT[k] = r.tBuild + r.tOps;
            runAvl(orig, n, opT, opV, m, 0, NULL, 0, tmp, &r);
            aB[k] = r.tBuild; aO[k] = r.tOps; aT[k] = r.tBuild + r.tOps;
        }
        double qTot = median(qT, REPS), aTot = median(aT, REPS);
        double qBm = median(qB, REPS), qOm = median(qO, REPS);
        double aBm = median(aB, REPS), aOm = median(aO, REPS);

        printf("\nData Size : %d\n", n);
        printf("Operations: %d\n\n", m);
        printf("                  Quick Sort      AVL Tree\n");
        printf("------------------------------------------------\n");
        printf("Total Time        %-10.4f ms   %-10.4f ms\n", qTot, aTot);
        printf("Comparisons       %-15lld %-15lld\n", qv.cBuild + qv.cOps, av.cBuild + av.cOps);
        printf("Data Movements    %-15lld %-15lld\n", qv.mBuild + qv.mOps, av.mBuild + av.mOps);
        printf("Memory Usage      %-8zu Bytes   %-8zu Bytes\n", qv.mem, av.mem);
        printf("\n");
        printf("Final Data Count  %-15ld %-15ld\n", qv.finalCount, av.finalCount);
        printf("\n[Details]  (sort/build phase | insert/delete phase)\n");
        printf("Time(ms)          %.4f | %.4f      %.4f | %.4f\n", qBm, qOm, aBm, aOm);
        printf("Comparisons       %lld | %lld      %lld | %lld\n", qv.cBuild, qv.cOps, av.cBuild, av.cOps);
        printf("Data Movements    %lld | %lld      %lld | %lld\n", qv.mBuild, qv.mOps, av.mBuild, av.mOps);
        printf("Inserts / Deletes done / Deletes skipped : %d / %d / %d\n", qv.ins, qv.delDone, qv.delSkip);
        printf("Verification      Quick: %s   AVL: %s   (final data identical: %s)\n",
               qv.ok ? "PASS" : "FAIL", av.ok ? "PASS" : "FAIL",
               (av.ok && qv.ok && av.delDone == qv.delDone) ? "YES" : "NO");

        free(orig); free(opT); free(opV); free(qbuf); free(tmp);
    }
    printf("\n=============== TEST COMPLETED ===============\n");
    return 0;
}
