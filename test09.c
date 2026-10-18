#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define V 10
#define E 20
#define MAXW 100
#define INF 1000000000

typedef struct { int u, v, w; } Edge;

static Edge edges[E];
static int adj[V][V];            /* 0 = 간선 없음, 그 외 = 가중치 */
static int dist[V][V];           /* Floyd-Warshall 최단거리 */
static int nxt[V][V];            /* nxt[i][j] = i에서 j로 갈 때 다음 정점 */

static int is_connected(void) {
    int seen[V] = {0}, stack[V], sp = 0, cnt = 0;
    stack[sp++] = 0; seen[0] = 1;
    while (sp) {
        int x = stack[--sp]; cnt++;
        for (int y = 0; y < V; y++)
            if (adj[x][y] && !seen[y]) { seen[y] = 1; stack[sp++] = y; }
    }
    return cnt == V;
}

static int cmp_edge(const void *a, const void *b) {
    const Edge *x = a, *y = b;
    return x->u != y->u ? x->u - y->u : x->v - y->v;
}

static void gen_graph(void) {
    do {
        memset(adj, 0, sizeof(adj));
        int perm[V], k = 0;
        for (int i = 0; i < V; i++) perm[i] = i;
        for (int i = V - 1; i > 0; i--) {            /* 임의 순열 */
            int j = rand() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t;
        }
        for (int i = 1; i < V; i++) {                /* 신장 트리로 연결성 확보 */
            int a = perm[i], b = perm[rand() % i], w = rand() % MAXW + 1;
            adj[a][b] = adj[b][a] = w;
            edges[k].u = a < b ? a : b; edges[k].v = a < b ? b : a; edges[k].w = w; k++;
        }
        while (k < E) {                              /* 나머지 간선: 자기 루프/중복 방지 */
            int a = rand() % V, b = rand() % V;
            if (a == b || adj[a][b]) continue;
            int w = rand() % MAXW + 1;
            adj[a][b] = adj[b][a] = w;
            edges[k].u = a < b ? a : b; edges[k].v = a < b ? b : a; edges[k].w = w; k++;
        }
    } while (!is_connected());
    qsort(edges, E, sizeof(Edge), cmp_edge);
}

static void floyd(void) {
    for (int i = 0; i < V; i++)
        for (int j = 0; j < V; j++) {
            if (i == j) { dist[i][j] = 0; nxt[i][j] = i; }
            else if (adj[i][j]) { dist[i][j] = adj[i][j]; nxt[i][j] = j; }
            else { dist[i][j] = INF; nxt[i][j] = -1; }
        }
    for (int k = 0; k < V; k++)
        for (int i = 0; i < V; i++)
            for (int j = 0; j < V; j++)
                if (dist[i][k] < INF && dist[k][j] < INF &&
                    dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    nxt[i][j] = nxt[i][k];
                }
}


static int get_path(int s, int t, int *path) {
    int n = 0, cur = s;
    if (nxt[s][t] < 0) return 0;
    path[n++] = cur;
    while (cur != t && n <= V) { cur = nxt[cur][t]; path[n++] = cur; }
    return n;
}

static void dijkstra(int s, int *d) {
    int done[V] = {0};
    for (int i = 0; i < V; i++) d[i] = INF;
    d[s] = 0;
    for (int it = 0; it < V; it++) {
        int u = -1;
        for (int i = 0; i < V; i++) if (!done[i] && (u < 0 || d[i] < d[u])) u = i;
        if (u < 0 || d[u] >= INF) break;
        done[u] = 1;
        for (int v = 0; v < V; v++)
            if (adj[u][v] && d[u] + adj[u][v] < d[v]) d[v] = d[u] + adj[u][v];
    }
}

int main(int argc, char **argv) {
    unsigned seed = (argc > 1) ? (unsigned)atoi(argv[1]) : 2024;
    srand(seed);
    gen_graph();
    floyd();

    printf("(seed = %u)\n\n", seed);
    printf("Graph\nVertices : %d\nEdges    : %d\n\n", V, E);
    printf("Edges\n");
    for (int i = 0; i < E; i++)
        printf("(%d, %d) Weight: %d\n", edges[i].u, edges[i].v, edges[i].w);

    printf("\nShortest Paths\n");
    int pairs = 0, path_ok = 0;
    for (int u = 0; u < V; u++)
        for (int v = u + 1; v < V; v++) {
            int path[V + 1];
            int n = get_path(u, v, path);
            printf("\n%d -> %d\nPath     : ", u, v);
            int sum = 0, valid = (n >= 2 && path[0] == u && path[n - 1] == v);
            for (int i = 0; i < n; i++) {
                printf(i ? " -> %d" : "%d", path[i]);
                if (i > 0) {
                    int w = adj[path[i - 1]][path[i]];
                    if (!w) valid = 0;               /* 존재하지 않는 간선을 지나면 오류 */
                    sum += w;
                }
            }
            printf("\nDistance : %d\n", dist[u][v]);
            pairs++;
            if (valid && sum == dist[u][v]) path_ok++;
        }
    printf("\nTotal vertex pairs : %d\n", pairs);

 
    int self_loop = 0, dup = 0, range_bad = 0, vertex_bad = 0;
    for (int i = 0; i < E; i++) {
        if (edges[i].u == edges[i].v) self_loop++;
        if (edges[i].w < 1 || edges[i].w > MAXW) range_bad++;
        if (edges[i].u < 0 || edges[i].v >= V) vertex_bad++;
        for (int j = i + 1; j < E; j++)
            if (edges[i].u == edges[j].u && edges[i].v == edges[j].v) dup++;
    }
    int dj[V], mismatch = 0;
    for (int s = 0; s < V; s++) {
        dijkstra(s, dj);
        for (int t = 0; t < V; t++) if (dj[t] != dist[s][t]) mismatch++;
    }
    printf("\n[Verification]\n");
    printf("Vertices = %d, Edges = %d\n", V, E);
    printf("Self-loops = %d, Duplicate edges = %d, Weight out of 1..%d = %d, Bad vertex id = %d\n",
           self_loop, dup, MAXW, range_bad, vertex_bad);
    printf("Connected graph            : %s\n", is_connected() ? "YES" : "NO");
    printf("Pairs printed              : %d / 45\n", pairs);
    printf("Path weight sum == Distance: %d / %d pairs\n", path_ok, pairs);
    printf("Floyd-Warshall vs Dijkstra : %s\n", mismatch ? "MISMATCH" : "all distances match");
    return 0;
}
