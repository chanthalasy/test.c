#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 50
#define SPARSE_E 100
#define DENSE_E 600
#define MAX_W 100
#define REPS 20000 

static size_t g_alloc = 0;
static void *tmalloc(size_t sz) { g_alloc += sz; return malloc(sz); }

typedef struct { int u, v, w; } Edge;

typedef struct LNode { int v, w; struct LNode *next; } LNode;
typedef struct { int n, e; LNode *head[N]; } AdjList;

typedef struct { int n, e; int w[N][N]; } AdjMatrix;   /* w == 0 : 간선 없음 */


static int tmp[N][N];

static int is_connected(const Edge *ed, int cnt) {
    int comp[N];
    for (int i = 0; i < N; i++) comp[i] = i;
    for (int i = 0; i < cnt; i++) {                 /* 간단한 union-find */
        int a = ed[i].u, b = ed[i].v;
        while (comp[a] != a) a = comp[a];
        while (comp[b] != b) b = comp[b];
        if (a != b) comp[a] = b;
    }
    int root = 0;
    while (comp[root] != root) root = comp[root];
    for (int i = 1; i < N; i++) {
        int r = i;
        while (comp[r] != r) r = comp[r];
        if (r != root) return 0;
    }
    return 1;
}

/* 연결 그래프 보장: 먼저 임의의 신장 트리(N-1개)를 만들고 나머지 간선을 추가 */
static void gen_graph(Edge *ed, int E) {
    do {
        memset(tmp, 0, sizeof(tmp));
        int perm[N];
        for (int i = 0; i < N; i++) perm[i] = i;
        for (int i = N - 1; i > 0; i--) {            /* Fisher-Yates */
            int j = rand() % (i + 1);
            int t = perm[i]; perm[i] = perm[j]; perm[j] = t;
        }
        int k = 0;
        for (int i = 1; i < N; i++) {
            int u = perm[i], v = perm[rand() % i];
            tmp[u][v] = tmp[v][u] = 1;
            ed[k].u = u; ed[k].v = v; ed[k].w = rand() % MAX_W + 1; k++;
        }
        while (k < E) {
            int u = rand() % N, v = rand() % N;
            if (u == v || tmp[u][v]) continue;
            tmp[u][v] = tmp[v][u] = 1;
            ed[k].u = u; ed[k].v = v; ed[k].w = rand() % MAX_W + 1; k++;
        }
    } while (!is_connected(ed, E));                  /* 연결 여부 확인 */
}
static AdjList *build_list(const Edge *ed, int E) {
    AdjList *g = tmalloc(sizeof(AdjList));
    memset(g, 0, sizeof(*g));
    g->n = N;
    for (int i = 0; i < E; i++) {
        LNode *a = tmalloc(sizeof(LNode)), *b = tmalloc(sizeof(LNode));
        a->v = ed[i].v; a->w = ed[i].w; a->next = g->head[ed[i].u]; g->head[ed[i].u] = a;
        b->v = ed[i].u; b->w = ed[i].w; b->next = g->head[ed[i].v]; g->head[ed[i].v] = b;
        g->e++;
    }
    return g;
}
static void free_list(AdjList *g) {
    for (int i = 0; i < N; i++)
        for (LNode *p = g->head[i]; p;) { LNode *nx = p->next; free(p); p = nx; }
    free(g);
}
static AdjMatrix *build_matrix(const Edge *ed, int E) {
    AdjMatrix *g = tmalloc(sizeof(AdjMatrix));
    memset(g, 0, sizeof(*g));
    g->n = N;
    for (int i = 0; i < E; i++) {
        g->w[ed[i].u][ed[i].v] = g->w[ed[i].v][ed[i].u] = ed[i].w;
        g->e++;
    }
    return g;
}

static int cmp_edge(const void *a, const void *b) {
    return ((const Edge *)a)->w - ((const Edge *)b)->w;
}
static int uf_find(int *p, int x) {
    while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; } 
    return x;
}

static long kruskal(const AdjList *g, Edge *mst, int *mst_cnt) {
    Edge *all = tmalloc(sizeof(Edge) * g->e);
    int *parent = tmalloc(sizeof(int) * N);
    int *rnk = tmalloc(sizeof(int) * N);
    int m = 0;
    for (int u = 0; u < N; u++)                        
        for (LNode *p = g->head[u]; p; p = p->next)
            if (u < p->v) { all[m].u = u; all[m].v = p->v; all[m].w = p->w; m++; }
    qsort(all, m, sizeof(Edge), cmp_edge);  
    for (int i = 0; i < N; i++) { parent[i] = i; rnk[i] = 0; }
    long cost = 0; int cnt = 0;
    for (int i = 0; i < m && cnt < N - 1; i++) {
        int a = uf_find(parent, all[i].u), b = uf_find(parent, all[i].v);
        if (a == b) continue;                           /* 사이클 */
        if (rnk[a] < rnk[b]) { int t = a; a = b; b = t; }
        parent[b] = a;
        if (rnk[a] == rnk[b]) rnk[a]++;
        mst[cnt++] = all[i];
        cost += all[i].w;
    }
    free(all); free(parent); free(rnk);
    *mst_cnt = cnt;
    return cost;
}

static long prim(const AdjMatrix *g, Edge *mst, int *mst_cnt) {
    int *key = tmalloc(sizeof(int) * N);
    int *par = tmalloc(sizeof(int) * N);
    char *in = tmalloc(sizeof(char) * N);
    const int INF = 1 << 30;
    for (int i = 0; i < N; i++) { key[i] = INF; par[i] = -1; in[i] = 0; }
    key[0] = 0;                                        
    long cost = 0; int cnt = 0;
    for (int it = 0; it < N; it++) {
        int u = -1, best = INF;
        for (int i = 0; i < N; i++)                    
            if (!in[i] && key[i] < best) { best = key[i]; u = i; }
        if (u < 0) break;
        in[u] = 1;
        if (par[u] >= 0) {
            mst[cnt].u = par[u]; mst[cnt].v = u; mst[cnt].w = key[u]; cnt++;
            cost += key[u];
        }
        for (int v = 0; v < N; v++)                     /* 행 한 줄로 key 갱신 */
            if (g->w[u][v] && !in[v] && g->w[u][v] < key[v]) { key[v] = g->w[u][v]; par[v] = u; }
    }
    free(key); free(par); free(in);
    *mst_cnt = cnt;
    return cost;
}
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

typedef struct { int E, cnt; long cost; size_t mem, graph_mem, aux_mem; double ms; Edge mst[N]; } Res;

static void print_case(int no, const char *gname, const char *rep, const char *alg, const Res *r) {
    printf("Case %d : %s Graph - %s - %s\n", no, gname, rep, alg);
    printf("Vertices      : %d\n", N);
    printf("Edges         : %d\n", r->E);
    printf("MST Edges     : %d\n", r->cnt);
    printf("MST Cost      : %ld\n", r->cost);
    printf("Memory        : %zu Bytes\n", r->mem);
    printf("Execution Time: %.6f ms (평균, %d회 반복)\n", r->ms, REPS);
    printf("MST edges (u-v:w):");
    for (int i = 0; i < r->cnt; i++) {
        if (i % 7 == 0) printf("\n  ");
        printf("%2d-%-2d:%-3d ", r->mst[i].u, r->mst[i].v, r->mst[i].w);
    }
    printf("\n\n");
}

static void run_graph(int E, Res *k, Res *p) {
    Edge *ed = malloc(sizeof(Edge) * E);
    gen_graph(ed, E);
    Edge tmpmst[N];
    int cnt;
    memset(k, 0, sizeof(*k)); memset(p, 0, sizeof(*p));
    k->E = p->E = E;
    g_alloc = 0;
    AdjList *lg = build_list(ed, E);
    k->graph_mem = g_alloc;
    g_alloc = 0;
    k->cost = kruskal(lg, k->mst, &k->cnt);               /* 1회 실행으로 보조 메모리 측정 */
    k->aux_mem = g_alloc + sizeof(Edge) * (N - 1);        /* + 결과 MST 배열 */
    k->mem = k->graph_mem + k->aux_mem;
    double t0 = now_ms();
    for (int r = 0; r < REPS; r++) kruskal(lg, tmpmst, &cnt);
    k->ms = (now_ms() - t0) / REPS;

    g_alloc = 0;
    AdjMatrix *mg = build_matrix(ed, E);
    p->graph_mem = g_alloc;
    g_alloc = 0;
    p->cost = prim(mg, p->mst, &p->cnt);
    p->aux_mem = g_alloc + sizeof(Edge) * (N - 1);
    p->mem = p->graph_mem + p->aux_mem;
    t0 = now_ms();
    for (int r = 0; r < REPS; r++) prim(mg, tmpmst, &cnt);
    p->ms = (now_ms() - t0) / REPS;

    free_list(lg); free(mg); free(ed);
}

int main(int argc, char **argv) {
    unsigned seed = (argc > 1) ? (unsigned)atoi(argv[1]) : 2024;
    srand(seed);
    printf("(seed = %u, sizeof(Edge)=%zu, sizeof(LNode)=%zu, sizeof(AdjList)=%zu, sizeof(AdjMatrix)=%zu)\n\n",
           seed, sizeof(Edge), sizeof(LNode), sizeof(AdjList), sizeof(AdjMatrix));

    Res sk, sp, dk, dp;
    run_graph(SPARSE_E, &sk, &sp);
    run_graph(DENSE_E, &dk, &dp);

    print_case(1, "Sparse", "Adjacency List",   "Kruskal", &sk);
    print_case(2, "Dense",  "Adjacency List",   "Kruskal", &dk);
    print_case(3, "Sparse", "Adjacency Matrix", "Prim",    &sp);
    print_case(4, "Dense",  "Adjacency Matrix", "Prim",    &dp);

    printf("[검증] Sparse: Kruskal %ld / Prim %ld -> %s | Dense: Kruskal %ld / Prim %ld -> %s\n",
           sk.cost, sp.cost, sk.cost == sp.cost ? "일치" : "불일치!!",
           dk.cost, dp.cost, dk.cost == dp.cost ? "일치" : "불일치!!");
    printf("[세부 메모리] (그래프 표현 + 보조 구조/MST 배열)\n");
    printf("  Sparse Kruskal/List  : %zu + %zu\n", sk.graph_mem, sk.aux_mem);
    printf("  Dense  Kruskal/List  : %zu + %zu\n", dk.graph_mem, dk.aux_mem);
    printf("  Sparse Prim/Matrix   : %zu + %zu\n", sp.graph_mem, sp.aux_mem);
    printf("  Dense  Prim/Matrix   : %zu + %zu\n", dp.graph_mem, dp.aux_mem);
    return 0;
}
