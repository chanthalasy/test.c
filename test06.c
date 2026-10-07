#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N_GEN 100
#define N_SEARCH 50
#define MAX_VAL 1000

typedef struct Node {
    int key;
    struct Node *left, *right;
    int height; /* AVL용 (노드 수 기준 높이) */
} Node;

static long cmp_count; /* 숫자 비교 횟수 카운터 */

static Node *new_node(int key) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->key = key; n->left = n->right = NULL; n->height = 1;
    return n;
}

/* ---------- 배열 ---------- */
/* 존재하면 1, 아니면 0. 비교 횟수는 cmp_count에 누적 */
static int seq_search(const int *a, int n, int key) {
    for (int i = 0; i < n; i++) {
        cmp_count++;
        if (a[i] == key) return 1;
    }
    return 0;
}

/* ---------- BST ---------- */
static Node *bst_insert(Node *root, int key) {
    if (!root) return new_node(key);
    Node *cur = root;
    while (1) {
        cmp_count++;                       /* 노드 하나당 비교 1회 */
        if (key == cur->key) return root;  /* 중복 */
        if (key < cur->key) {
            if (!cur->left) { cur->left = new_node(key); return root; }
            cur = cur->left;
        } else {
            if (!cur->right) { cur->right = new_node(key); return root; }
            cur = cur->right;
        }
    }
}

static int tree_search(Node *root, int key) {
    Node *cur = root;
    while (cur) {
        cmp_count++;
        if (key == cur->key) return 1;
        cur = (key < cur->key) ? cur->left : cur->right;
    }
    return 0;
}

static int bst_height(Node *n) {
    if (!n) return 0;
    int l = bst_height(n->left), r = bst_height(n->right);
    return 1 + (l > r ? l : r);
}

/* ---------- AVL ---------- */
static int h(Node *n) { return n ? n->height : 0; }
static int mx(int a, int b) { return a > b ? a : b; }
static void upd(Node *n) { n->height = 1 + mx(h(n->left), h(n->right)); }
static int bf(Node *n) { return n ? h(n->left) - h(n->right) : 0; }

static Node *rot_right(Node *y) {
    Node *x = y->left;
    y->left = x->right; x->right = y;
    upd(y); upd(x);
    return x;
}
static Node *rot_left(Node *x) {
    Node *y = x->right;
    x->right = y->left; y->left = x;
    upd(x); upd(y);
    return y;
}

static int dup_found; /* 재귀 삽입 중 중복 여부 전달 */

static Node *avl_insert_rec(Node *node, int key) {
    if (!node) return new_node(key);
    cmp_count++;
    if (key == node->key) { dup_found = 1; return node; }
    if (key < node->key) node->left = avl_insert_rec(node->left, key);
    else                 node->right = avl_insert_rec(node->right, key);
    if (dup_found) return node;

    upd(node);
    int b = bf(node);
    if (b > 1) {
        if (bf(node->left) < 0) node->left = rot_left(node->left); /* LR */
        return rot_right(node);                                    /* LL */
    }
    if (b < -1) {
        if (bf(node->right) > 0) node->right = rot_right(node->right); /* RL */
        return rot_left(node);                                         /* RR */
    }
    return node;
}
static Node *avl_insert(Node *root, int key) {
    dup_found = 0;
    return avl_insert_rec(root, key);
}

static void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left); free_tree(n->right); free(n);
}

int main(int argc, char **argv) {
    unsigned seed = (argc > 1) ? (unsigned)strtoul(argv[1], NULL, 10) : (unsigned)time(NULL);
    srand(seed);
    printf("Seed : %u\n\n", seed);

    int gen[N_GEN], arr[N_GEN], n = 0;
    Node *bst = NULL, *avl = NULL;
    long arr_c = 0, bst_c = 0, avl_c = 0;
    int dup = 0;

    for (int i = 0; i < N_GEN; i++) gen[i] = rand() % (MAX_VAL + 1);

    printf("Generated numbers (%d):\n", N_GEN);
    for (int i = 0; i < N_GEN; i++) printf("%4d%s", gen[i], (i % 10 == 9) ? "\n" : " ");
    printf("\n");

    for (int i = 0; i < N_GEN; i++) {
        int v = gen[i];

        cmp_count = 0;
        int found = seq_search(arr, n, v);
        arr_c += cmp_count;
        if (!found) arr[n++] = v; else dup++;

        cmp_count = 0; bst = bst_insert(bst, v); bst_c += cmp_count;
        cmp_count = 0; avl = avl_insert(avl, v); avl_c += cmp_count;
    }

    printf("Stored values      : %d\n", n);
    printf("Duplicates skipped : %d\n\n", dup);
    printf("Construction\n");
    printf("Array comparisons : %ld\n", arr_c);
    printf("BST comparisons   : %ld\n", bst_c);
    printf("AVL comparisons   : %ld\n\n", avl_c);
    printf("Structure\n");
    printf("Array length : %d\n", n);
    printf("BST height   : %d\n", bst_height(bst));
    printf("AVL height   : %d\n\n", bst_height(avl));

    int keys[N_SEARCH];
    for (int i = 0; i < N_SEARCH; i++) keys[i] = rand() % (MAX_VAL + 1);
    printf("Search keys (%d):\n", N_SEARCH);
    for (int i = 0; i < N_SEARCH; i++) printf("%4d%s", keys[i], (i % 10 == 9) ? "\n" : " ");
    printf("\n");

    long ts = 0, tb = 0, ta = 0;
    for (int i = 0; i < N_SEARCH; i++) {
        int k = keys[i];
        cmp_count = 0; int rs = seq_search(arr, n, k); long cs = cmp_count;
        cmp_count = 0; int rb = tree_search(bst, k);   long cb = cmp_count;
        cmp_count = 0; int ra = tree_search(avl, k);   long ca = cmp_count;
        ts += cs; tb += cb; ta += ca;
        printf("Search Key : %d\n\n", k);
        printf("Sequential Search\nResult      : %s\nComparisons : %ld\n\n", rs ? "Found" : "Not Found", cs);
        printf("BST Search\nResult      : %s\nComparisons : %ld\n\n", rb ? "Found" : "Not Found", cb);
        printf("AVL Search\nResult      : %s\nComparisons : %ld\n", ra ? "Found" : "Not Found", ca);
        printf("----------------------------------------\n");
    }

    printf("\nSearches : %d\n\n", N_SEARCH);
    printf("Sequential Search\nTotal comparisons   : %ld\nAverage comparisons : %.2f\n\n", ts, (double)ts / N_SEARCH);
    printf("BST Search\nTotal comparisons   : %ld\nAverage comparisons : %.2f\n\n", tb, (double)tb / N_SEARCH);
    printf("AVL Search\nTotal comparisons   : %ld\nAverage comparisons : %.2f\n", ta, (double)ta / N_SEARCH);

    free_tree(bst); free_tree(avl);
    return 0;
}
