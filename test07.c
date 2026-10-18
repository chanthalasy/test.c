#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 100
#define SPARSE_E 100
#define DENSE_E 4000
#define TRIALS 1000

static size_t g_alloc = 0;
static void *tracked_malloc(size_t sz) {
    g_alloc += sz;
    return malloc(sz);
}
typedef struct { int n, e; int m[N][N]; } AdjMatrix;

typedef struct Node { int v; struct Node *next; } Node;
typedef struct { int n, e; Node *head[N]; } AdjList;

typedef struct { int u, v; } Edge;
static void mat_init(AdjMatrix *g) { memset(g, 0, sizeof(*g)); g->n = N; }

static int mat_insert(AdjMatrix *g, int u, int v) {
    int cmp = 1;
    if (g->m[u][v] == 0) { g->m[u][v] = g->m[v][u] = 1; g->e++; }
    return cmp;
}

static int mat_delete(AdjMatrix *g, int u, int v) {
    int cmp = 1;
    if (g->m[u][v] == 1) { g->m[u][v] = g->m[v][u] = 0; g->e--; }
    return cmp;
}
static int mat_connected(const AdjMatrix *g, int u, int v, int *res) {
    *res = g->m[u][v];
    return 1;
}
static int mat_neighbors(const AdjMatrix *g, int u, int print) {
    int cmp = 0;
    for (int i = 0; i < N; i++) {
        cmp++;
        if (g->m[u][i] && print) printf("%d ", i);
    }
    return cmp;
}

static void list_init(AdjList *g) { memset(g, 0, sizeof(*g)); g->n = N; }

static void list_push_front(AdjList *g, int u, int v) {
    Node *nd = tracked_malloc(sizeof(Node));
    nd->v = v; nd->next = g->head[u]; g->head[u] = nd;
}
static int list_find_cost(const AdjList *g, int u, int v, int *found) {
    int cmp = 0; *found = 0;
    for (Node *p = g->head[u]; p; p = p->next) {
        cmp++;
        if (p->v == v) { *found = 1; break; }
    }
    return cmp;
}

static int list_insert(AdjList *g, int u, int v) {
    int found, cmp = list_find_cost(g, u, v, &found);
    if (!found) { list_push_front(g, u, v); list_push_front(g, v, u); g->e++; }
    return cmp;
}
static int list_remove_one(AdjList *g, int u, int v) {
    int cmp = 0;
    Node *prev = NULL;
    for (Node *p = g->head[u]; p; prev = p, p = p->next) {
        cmp++;
        if (p->v == v) {
            if (prev) prev->next = p->next; else g->head[u] = p->next;
            free(p);
            break;
        }
    }
    return cmp;
}
static int list_delete(AdjList *g, int u, int v) {
    int cmp = list_remove_one(g, u, v) + list_remove_one(g, v, u);
    g->e--;
    return cmp;
}
static int list_connected(const AdjList *g, int u, int v, int *res) {
    return list_find_cost(g, u, v, res);
}
static int list_neighbors(const AdjList *g, int u, int print) {
    int cmp = 0;
    for (Node *p = g->head[u]; p; p = p->next) {
        cmp++;
        if (print) printf("%d ", p->v);
    }
    return cmp;
}
static int list_degree(const AdjList *g, int u) { return list_neighbors(g, u, 0); }
static int tmp[N][N]; /* 난수 생성용 임시 배열 (측정 제외) */

static int gen_edges(Edge *edges, int count) {
    memset(tmp, 0, sizeof(tmp));
    int k = 0;
    while (k < count) {
        int u = rand() % N, v = rand() % N;
        if (u == v || tmp[u][v]) continue;
        tmp[u][v] = tmp[v][u] = 1;
        edges[k].u = u; edges[k].v = v; k++;
    }
    return k;
}

static void pick_non_edge(int *pu, int *pv) {
    int u, v;
    do { u = rand() % N; v = rand() % N; } while (u == v || tmp[u][v]);
    *pu = u; *pv = v;
}
typedef struct { size_t mem; int ins, del, conn, adj; } Result;

static void print_result(int no, const char *gname, const char *rname, const Result *r) {
    printf("Case %d : %s Graph - %s\n", no, gname, rname);
    printf("Memory                         : %zu Bytes\n", r->mem);
    printf("Edge insertion comparisons     : %d\n", r->ins);
    printf("Edge deletion comparisons      : %d\n", r->del);
    printf("Connectivity check comparisons : %d\n", r->conn);
    printf("Adjacent vertices comparisons  : %d\n\n", r->adj);
}

static void run_graph(const char *gname, int edge_cnt, int case_base) {
    Edge *edges = malloc(sizeof(Edge) * edge_cnt);
    gen_edges(edges, edge_cnt);

    /* 연산 대상 (두 표현 방식에 동일하게 적용) */
    int iu, iv, du, dv, cu, cv, au;
    pick_non_edge(&iu, &iv);                  /* 삽입: 존재하지 않는 간선 */
    int di = rand() % edge_cnt;               /* 삭제: 존재하는 간선 */
    du = edges[di].u; dv = edges[di].v;
    do { cu = rand() % N; cv = rand() % N; } while (cu == cv);  /* 연결 확인 */
    au = rand() % N;                          /* 인접 정점 출력 */

   
    AdjMatrix *mg = malloc(sizeof(AdjMatrix));
    mat_init(mg);
    for (int i = 0; i < edge_cnt; i++) {
        mg->m[edges[i].u][edges[i].v] = mg->m[edges[i].v][edges[i].u] = 1;
        mg->e++;
    }

    g_alloc = 0;
    AdjList *lg = tracked_malloc(sizeof(AdjList));
    list_init(lg);
    for (int i = 0; i < edge_cnt; i++) {
        list_push_front(lg, edges[i].u, edges[i].v);
        list_push_front(lg, edges[i].v, edges[i].u);
        lg->e++;
    }
    size_t list_mem = g_alloc;
    size_t mat_mem = sizeof(AdjMatrix);

    Result rm = {0}, rl = {0};
    rm.mem = mat_mem; rl.mem = list_mem;

    int res_m, res_l;
    rm.ins = mat_insert(mg, iu, iv);
    rl.ins = list_insert(lg, iu, iv);
    rm.del = mat_delete(mg, du, dv);
    rl.del = list_delete(lg, du, dv);
    /* 삽입/삭제를 반영한 뒤 연결 확인 및 인접 정점 측정 */
    rm.conn = mat_connected(mg, cu, cv, &res_m);
    rl.conn = list_connected(lg, cu, cv, &res_l);

    rm.adj = mat_neighbors(mg, au, 0);
    rl.adj = list_neighbors(lg, au, 0);

    print_result(case_base,     gname, "Adjacency Matrix", &rm);
    print_result(case_base + 1, gname, "Adjacency List",   &rl);

    printf("  [측정 대상] 삽입 (%d,%d) / 삭제 (%d,%d) / 연결 확인 (%d,%d) -> %s / 인접 정점 출력 대상 정점 %d\n",
           iu, iv, du, dv, cu, cv, res_m == res_l ? (res_m ? "연결됨" : "연결 안 됨") : "!!불일치!!", au);
    printf("  [Matrix] 정점 %d의 인접 정점 : ", au);
    mat_neighbors(mg, au, 1);
    printf("\n  [List  ] 정점 %d의 인접 정점 : ", au);
    list_neighbors(lg, au, 1);
    printf("\n  Adjacent vertices comparisons -> Matrix: %d, List: %d (정점 %d의 차수 = %d)\n\n",
           rm.adj, rl.adj, au, list_degree(lg, au));

    
    long s_ins = 0, s_del = 0, s_conn = 0, s_adj = 0;

    for (int t = 0; t < TRIALS; t++) {
        int u, v, f;
        do { u = rand() % N; v = rand() % N; } while (u == v || mg->m[u][v]);
        s_ins += list_find_cost(lg, u, v, &f);        /* 비연결 쌍: u 리스트 전체 */
    }
    for (int t = 0; t < TRIALS; t++) {
        int u, v, f1, f2;
        do { u = rand() % N; v = rand() % N; } while (u == v || !mg->m[u][v]);
        s_del += list_find_cost(lg, u, v, &f1) + list_find_cost(lg, v, u, &f2);
    }
    for (int t = 0; t < TRIALS; t++) {
        int u, v, f;
        do { u = rand() % N; v = rand() % N; } while (u == v);
        s_conn += list_find_cost(lg, u, v, &f);
    }
    for (int t = 0; t < TRIALS; t++) s_adj += list_degree(lg, rand() % N);
    printf("  [참고] 임의 연산 %d회 평균 (Matrix는 항상 삽입 1 / 삭제 1 / 연결 1 / 인접 %d)\n", TRIALS, N);
    printf("         List 평균 -> 삽입 %.2f / 삭제 %.2f / 연결 확인 %.2f / 인접 정점 %.2f\n\n",
           (double)s_ins / TRIALS, (double)s_del / TRIALS,
           (double)s_conn / TRIALS, (double)s_adj / TRIALS);
    for (int i = 0; i < N; i++) {
        Node *p = lg->head[i];
        while (p) { Node *nx = p->next; free(p); p = nx; }
    }
    free(lg); free(mg); free(edges);
}

int main(int argc, char **argv) {
    unsigned seed = (argc > 1) ? (unsigned)atoi(argv[1]) : 2024;
    srand(seed);
    printf("(seed = %u, sizeof(int)=%zu, sizeof(Node)=%zu, sizeof(void*)=%zu)\n\n",
           seed, sizeof(int), sizeof(Node), sizeof(void *));
    run_graph("Sparse", SPARSE_E, 1);
    run_graph("Dense", DENSE_E, 3);
    return 0;
}
