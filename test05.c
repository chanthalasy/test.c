#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100        /* 저장할 정수 개수 */
#define M 50         /* 탐색 대상 개수 */
#define MAXV 1000    /* 값의 범위: 0 ~ 1000 */

typedef struct Node {
    int data;
    struct Node *left;
    struct Node *right;
} Node;

/* 비교 횟수 카운터 */
static long build_cmp = 0;

Node *new_node(int v) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->data = v;
    n->left = n->right = NULL;
    return n;
}

/* BST 삽입: 기존 노드와 비교할 때마다 build_cmp 증가
   (같은 값은 없다고 보장되므로 '<' 한 번을 비교 1회로 계산) */
Node *bst_insert(Node *root, int v) {
    if (root == NULL) return new_node(v);
    Node *cur = root;
    while (1) {
        build_cmp++;
        if (v < cur->data) {
            if (cur->left == NULL) { cur->left = new_node(v); break; }
            cur = cur->left;
        } else {
            if (cur->right == NULL) { cur->right = new_node(v); break; }
            cur = cur->right;
        }
    }
    return root;
}

/* BST 탐색: 노드 방문 1회 = 비교 1회로 계산 */
int bst_search(Node *root, int key, int *cmp) {
    Node *cur = root;
    *cmp = 0;
    while (cur != NULL) {
        (*cmp)++;
        if (key == cur->data) return 1;
        else if (key < cur->data) cur = cur->left;
        else cur = cur->right;
    }
    return 0;
}

/* 순차 탐색: 원소와 비교할 때마다 1회 */
int seq_search(int arr[], int n, int key, int *cmp) {
    *cmp = 0;
    for (int i = 0; i < n; i++) {
        (*cmp)++;
        if (arr[i] == key) return 1;
    }
    return 0;
}

int bst_height(Node *r) {
    if (!r) return 0;
    int l = bst_height(r->left), rt = bst_height(r->right);
    return (l > rt ? l : rt) + 1;
}

void free_tree(Node *r) {
    if (!r) return;
    free_tree(r->left);
    free_tree(r->right);
    free(r);
}

int main(void) {
    int arr[N];
    int used[MAXV + 1] = {0};   /* 중복 방지용 */
    Node *root = NULL;

    srand((unsigned)time(NULL));

    /* 1. 중복 없는 100개 생성 → 배열(발생 순서 유지) + BST */
    for (int i = 0; i < N; ) {
        int v = rand() % (MAXV + 1);
        if (used[v]) continue;      /* 중복이면 다시 생성 */
        used[v] = 1;
        arr[i++] = v;
        root = bst_insert(root, v);
    }

    printf("=== 생성된 %d개의 정수 (발생 순서) ===\n", N);
    for (int i = 0; i < N; i++)
        printf("%4d%s", arr[i], (i % 10 == 9) ? "\n" : " ");

    printf("\nBST 생성 과정의 총 비교 횟수 : %ld\n", build_cmp);
    printf("BST 높이                     : %d\n", bst_height(root));

    /* 2~4. 탐색 대상 50개 생성 및 탐색 */
    long seq_total = 0, bst_total = 0;
    int found_cnt = 0;

    printf("\n=== 탐색 결과 ===\n");
    printf("%-4s %-10s %-8s %-12s %-12s\n",
           "No.", "SearchKey", "Result", "SeqCmp", "BSTCmp");

    for (int i = 0; i < M; i++) {
        int key = rand() % (MAXV + 1);
        int sc, bc;
        int sf = seq_search(arr, N, key, &sc);
        int bf = bst_search(root, key, &bc);

        /* 두 방법의 결과는 반드시 일치해야 함 */
        if (sf != bf) printf("오류: 탐색 결과 불일치!\n");

        seq_total += sc;
        bst_total += bc;
        if (sf) found_cnt++;

        printf("%-4d %-10d %-8s %-12d %-12d\n",
               i + 1, key, sf ? "Found" : "NotFound", sc, bc);
    }

    printf("\nNumber of searches: %d (Found %d / Not found %d)\n",
           M, found_cnt, M - found_cnt);

    printf("\nSequential Search\n");
    printf("Total comparisons   : %ld\n", seq_total);
    printf("Average comparisons : %.2f\n", (double)seq_total / M);

    printf("\nBST Search\n");
    printf("Total comparisons   : %ld\n", bst_total);
    printf("Average comparisons : %.2f\n", (double)bst_total / M);

    printf("\n=== 생성 비용 포함 분석 ===\n");
    printf("BST 생성 + 50회 탐색 총 비교 : %ld\n", build_cmp + bst_total);
    printf("순차 탐색 50회 총 비교       : %ld\n", seq_total);
    printf("차이 (순차 - BST전체)        : %ld\n",
           seq_total - (build_cmp + bst_total));

    free_tree(root);
    return 0;
}
