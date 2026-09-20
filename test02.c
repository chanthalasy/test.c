
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct Node {
    char data;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
} Node;

static Node *newNode(char data) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->data = data;
    n->left = n->right = n->parent = NULL;
    return n;
}

static void freeTree(Node *root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}
static const char *g_input;
static int g_pos;

static void skipSpace(void) {
    while (g_input[g_pos] == ' ' || g_input[g_pos] == '\t') g_pos++;
}

static Node *parseTree(void) {
    skipSpace();
    char c = g_input[g_pos];

    if (c == '\0' || c == ',' || c == ')') {
        return NULL;
    }
    char data = c;
    g_pos++;
    Node *node = newNode(data);
    skipSpace();
    if (g_input[g_pos] == '(') {
        g_pos++;                       /* '(' 소비 */
        Node *left = parseTree();
        skipSpace();
        if (g_input[g_pos] == ',') g_pos++;   /* ',' 소비 */
        Node *right = parseTree();
        skipSpace();
        if (g_input[g_pos] == ')') g_pos++;   /* ')' 소비 */
        node->left = left;
        node->right = right;
        if (left)  left->parent  = node;
        if (right) right->parent = node;
    }
    return node;
}

static Node *buildTreeFromString(const char *str) {
    g_input = str;
    g_pos = 0;
    return parseTree();
}

typedef struct {
    char data;
    int  occupied;
} ArrCell;

typedef struct {
    ArrCell *cell;
    int capacity;
} ArrTree;

static void arrEnsureCapacity(ArrTree *t, int idx) {
    if (idx < t->capacity) return;
    int newCap = (t->capacity == 0) ? 2 : t->capacity;
    while (newCap <= idx) newCap *= 2;
    t->cell = (ArrCell *)realloc(t->cell, sizeof(ArrCell) * newCap);
    for (int i = t->capacity; i < newCap; i++) t->cell[i].occupied = 0;
    t->capacity = newCap;
}

static void arrInsert(ArrTree *t, Node *node, int idx) {
    if (node == NULL) return;
    arrEnsureCapacity(t, idx);
    t->cell[idx].data = node->data;
    t->cell[idx].occupied = 1;
    arrInsert(t, node->left,  2 * idx);
    arrInsert(t, node->right, 2 * idx + 1);
}

static ArrTree buildArrayFromLinked(Node *root) {
    ArrTree t;
    t.cell = NULL;
    t.capacity = 0;
    if (root != NULL) arrInsert(&t, root, 1);
    return t;
}

static void freeArrTree(ArrTree *t) {
    free(t->cell);
    t->cell = NULL;
    t->capacity = 0;
}

static int arrOccupied(ArrTree *t, int idx) {
    return (idx >= 1 && idx < t->capacity && t->cell[idx].occupied);
}

static void printTreeLinked(Node *node, int depth) {
    if (node == NULL) return;
    printTreeLinked(node->right, depth + 1);
    for (int i = 0; i < depth; i++) printf("\t");
    printf("%c\n", node->data);
    printTreeLinked(node->left, depth + 1);
}

static void printTreeArray(ArrTree *t, int idx, int depth) {
    if (!arrOccupied(t, idx)) return;
    printTreeArray(t, 2 * idx + 1, depth + 1);
    for (int i = 0; i < depth; i++) printf("\t");
    printf("%c\n", t->cell[idx].data);
    printTreeArray(t, 2 * idx, depth + 1);
}

static int countNodesL(Node *n) {
    if (n == NULL) return 0;
    return 1 + countNodesL(n->left) + countNodesL(n->right);
}
static int countLeavesL(Node *n) {
    if (n == NULL) return 0;
    if (n->left == NULL && n->right == NULL) return 1;
    return countLeavesL(n->left) + countLeavesL(n->right);
}

static int heightL(Node *n) {
    if (n == NULL) return -1;
    int l = heightL(n->left);
    int r = heightL(n->right);
    return 1 + (l > r ? l : r);
}
static int degreeL(Node *n) {
    if (n == NULL) return 0;
    int d = 0;
    if (n->left)  d++;
    if (n->right) d++;
    int ld = degreeL(n->left);
    int rd = degreeL(n->right);
    if (ld > d) d = ld;
    if (rd > d) d = rd;
    return d;
}

static void printTreeInfoLinked(Node *root) {
    int total = countNodesL(root);
    int leaf  = countLeavesL(root);
    int nonleaf = total - leaf;
    int h = heightL(root);
    int d = degreeL(root);
    printf("  1) 전체 노드 수     : %d\n", total);
    printf("  2) 단말 노드 수     : %d\n", leaf);
    printf("  3) 비단말 노드 수   : %d\n", nonleaf);
    printf("  4) 트리의 높이      : %d\n", h);
    printf("  5) 트리의 차수      : %d\n", d);
}


static int countNodesA(ArrTree *t) {
    int cnt = 0;
    for (int i = 1; i < t->capacity; i++)
        if (t->cell[i].occupied) cnt++;
    return cnt;
}
static int countLeavesA(ArrTree *t) {
    int cnt = 0;
    for (int i = 1; i < t->capacity; i++) {
        if (!t->cell[i].occupied) continue;
        if (!arrOccupied(t, 2 * i) && !arrOccupied(t, 2 * i + 1)) cnt++;
    }
    return cnt;
}
static int heightArrayRec(ArrTree *t, int idx) {
    if (!arrOccupied(t, idx)) return -1;
    int l = heightArrayRec(t, 2 * idx);
    int r = heightArrayRec(t, 2 * idx + 1);
    return 1 + (l > r ? l : r);
}
static int degreeArrayRec(ArrTree *t, int idx) {
    if (!arrOccupied(t, idx)) return 0;
    int d = 0;
    if (arrOccupied(t, 2 * idx))     d++;
    if (arrOccupied(t, 2 * idx + 1)) d++;
    int ld = degreeArrayRec(t, 2 * idx);
    int rd = degreeArrayRec(t, 2 * idx + 1);
    if (ld > d) d = ld;
    if (rd > d) d = rd;
    return d;
}

static void printTreeInfoArray(ArrTree *t) {
    int total = countNodesA(t);
    int leaf  = countLeavesA(t);
    int nonleaf = total - leaf;
    int h = (total == 0) ? -1 : heightArrayRec(t, 1);
    int d = (total == 0) ? 0  : degreeArrayRec(t, 1);
    printf("  1) 전체 노드 수     : %d\n", total);
    printf("  2) 단말 노드 수     : %d\n", leaf);
    printf("  3) 비단말 노드 수   : %d\n", nonleaf);
    printf("  4) 트리의 높이      : %d\n", h);
    printf("  5) 트리의 차수      : %d\n", d);
}

static int isCompleteLinked(Node *root) {
    if (root == NULL) return 1;
    Node **queue = (Node **)malloc(sizeof(Node *) * 100000);
    int front = 0, rear = 0;
    int seenNull = 0;
    queue[rear++] = root;
    int ok = 1;
    while (front < rear) {
        Node *cur = queue[front++];
        if (cur == NULL) {
            seenNull = 1;
        } else {
            if (seenNull) { ok = 0; break; }
            queue[rear++] = cur->left;
            queue[rear++] = cur->right;
        }
    }
    free(queue);
    return ok;
}

static int isCompleteArray(ArrTree *t, int total) {

    if (total == 0) return 1;
    for (int i = 1; i <= total; i++) {
        if (!arrOccupied(t, i)) return 0;
    }
    return 1;
}

static int isFullLinked(Node *root) {
    int total = countNodesL(root);
    int h = heightL(root);
    if (total == 0) return 1;
    return total == (int)(pow(2.0, h + 1) - 1 + 0.5);
}
static int isFullArray(ArrTree *t, int total) {
    int h = (total == 0) ? -1 : heightArrayRec(t, 1);
    if (total == 0) return 1;
    return total == (int)(pow(2.0, h + 1) - 1 + 0.5);
}

static int isLeftSkewedLinked(Node *n) {
    if (n == NULL) return 1;
    if (n->right != NULL) return 0;
    return isLeftSkewedLinked(n->left);
}
static int isRightSkewedLinked(Node *n) {
    if (n == NULL) return 1;
    if (n->left != NULL) return 0;
    return isRightSkewedLinked(n->right);
}

static const char *skewedKindLinked(Node *root) {
    int total = countNodesL(root);
    if (total < 2) return "해당없음(노드 1개 이하)";
    if (isLeftSkewedLinked(root))  return "예 (좌편향)";
    if (isRightSkewedLinked(root)) return "예 (우편향)";
    return "아니오";
}

static int isLeftSkewedArray(ArrTree *t, int idx) {
    if (!arrOccupied(t, idx)) return 1;
    if (arrOccupied(t, 2 * idx + 1)) return 0;
    return isLeftSkewedArray(t, 2 * idx);
}
static int isRightSkewedArray(ArrTree *t, int idx) {
    if (!arrOccupied(t, idx)) return 1;
    if (arrOccupied(t, 2 * idx)) return 0;
    return isRightSkewedArray(t, 2 * idx + 1);
}
static const char *skewedKindArray(ArrTree *t, int total) {
    if (total < 2) return "해당없음(노드 1개 이하)";
    if (isLeftSkewedArray(t, 1))  return "예 (좌편향)";
    if (isRightSkewedArray(t, 1)) return "예 (우편향)";
    return "아니오";
}

static void printShapeLinked(Node *root) {
    printf("  - 완전 이진트리 여부 : %s\n", isCompleteLinked(root) ? "예" : "아니오");
    printf("  - 포화 이진트리 여부 : %s\n", isFullLinked(root) ? "예" : "아니오");
    printf("  - 편향 이진트리 여부 : %s\n", skewedKindLinked(root));
}
static void printShapeArray(ArrTree *t) {
    int total = countNodesA(t);
    printf("  - 완전 이진트리 여부 : %s\n", isCompleteArray(t, total) ? "예" : "아니오");
    printf("  - 포화 이진트리 여부 : %s\n", isFullArray(t, total) ? "예" : "아니오");
    printf("  - 편향 이진트리 여부 : %s\n", skewedKindArray(t, total));
}

static Node *findNodeLinked(Node *root, char target) {
    if (root == NULL) return NULL;
    if (root->data == target) return root;
    Node *l = findNodeLinked(root->left, target);
    if (l) return l;
    return findNodeLinked(root->right, target);
}

static void queryNodeLinked(Node *root, char target) {
    Node *node = findNodeLinked(root, target);
    if (node == NULL) {
        printf("  '%c' 노드를 찾을 수 없습니다.\n", target);
        return;
    }
    printf("  [연결 자료구조] '%c' 노드 검색 결과\n", target);
    printf("   - 왼쪽 자식 : %s\n", node->left  ? (char[]){node->left->data, 0}  : "없음");
    printf("   - 오른쪽 자식 : %s\n", node->right ? (char[]){node->right->data, 0} : "없음");
    if (node->parent == NULL) {
        printf("   - 부모 : 없음 (루트 노드)\n");
        printf("   - 형제 : 없음\n");
    } else {
        printf("   - 부모 : %c\n", node->parent->data);
        Node *sibling = (node->parent->left == node) ? node->parent->right : node->parent->left;
        printf("   - 형제 : %s\n", sibling ? (char[]){sibling->data, 0} : "없음");
    }
}

static int findIndexArray(ArrTree *t, char target) {
    for (int i = 1; i < t->capacity; i++) {
        if (t->cell[i].occupied && t->cell[i].data == target) return i;
    }
    return -1;
}

static void queryNodeArray(ArrTree *t, char target) {
    int idx = findIndexArray(t, target);
    if (idx == -1) {
        printf("  '%c' 노드를 찾을 수 없습니다.\n", target);
        return;
    }
    printf("  [배열 기반] '%c' 노드(index=%d) 검색 결과\n", target, idx);
    printf("   - 왼쪽 자식 : %s\n", arrOccupied(t, 2 * idx) ? (char[]){t->cell[2*idx].data, 0} : "없음");
    printf("   - 오른쪽 자식 : %s\n", arrOccupied(t, 2 * idx + 1) ? (char[]){t->cell[2*idx+1].data, 0} : "없음");
    if (idx == 1) {
        printf("   - 부모 : 없음 (루트 노드)\n");
        printf("   - 형제 : 없음\n");
    } else {
        int pIdx = idx / 2;
        printf("   - 부모 : %c\n", t->cell[pIdx].data);
        int sibIdx = (idx % 2 == 0) ? idx + 1 : idx - 1;
        printf("   - 형제 : %s\n", arrOccupied(t, sibIdx) ? (char[]){t->cell[sibIdx].data, 0} : "없음");
    }
}

static void memoryCompareOneCase(const char *label, const char *bracketStr) {
    Node *root = buildTreeFromString(bracketStr);
    ArrTree arrT = buildArrayFromLinked(root);

    int total = countNodesL(root);
    long linkedBytes = (long)total * (long)sizeof(Node);
    long arrayBytes  = (long)arrT.capacity * (long)sizeof(ArrCell);
    int wasted = arrT.capacity - 1 - total; /* index 0은 사용 안 함 */

    printf("----------------------------------------------------------------\n");
    printf(" [%s]  입력: %s\n", label, bracketStr);
    printf("   실제 노드 수                : %d\n", total);
    printf("   연결 자료구조 : node 1개 크기 %zu byte x %d개 = %ld byte\n",
           sizeof(Node), total, linkedBytes);
    printf("   배열 기반     : cell 1개 크기 %zu byte x 배열크기 %d칸 = %ld byte\n",
           sizeof(ArrCell), arrT.capacity, arrayBytes);
    printf("                    (그 중 실제 사용 %d칸, 낭비되는 빈 칸 %d칸)\n",
           total, wasted);
    printf("   메모리 비율(배열/연결)      : %.2f 배\n",
           (double)arrayBytes / (double)linkedBytes);

    freeTree(root);
    freeArrTree(&arrT);
}

static void memoryCompareAll(void) {
    printf("\n================ [3-1] 메모리 사용량 비교 ================\n");
    printf(" * 연결 자료구조 노드 크기 sizeof(Node) = %zu byte (data+left+right+parent)\n",
           sizeof(Node));
    printf(" * 배열 기반 셀 크기       sizeof(ArrCell) = %zu byte (data+occupied)\n",
           sizeof(ArrCell));

    memoryCompareOneCase("일반 이진트리", "A(B(D,E),C(,F))");

    memoryCompareOneCase("완전 이진트리", "A(B(D,E),C(F,))");

    memoryCompareOneCase("편향 이진트리(좌편향)", "A(B(C(D(E,),),),)");

    printf("----------------------------------------------------------------\n");
    printf(" 결론: 완전 이진트리에서는 배열의 빈 칸이 거의 없어 두 구현의\n");
    printf(" 메모리 사용량이 비슷하지만, 편향 이진트리에서는 배열의 인덱스가\n");
    printf(" 지수적으로 커져 대부분의 칸이 낭비된다. 자세한 설명은 report.md 참고.\n");
}

static void printMenu(void) {
    printf("\n================== 이진트리 프로그램 메뉴 ==================\n");
    printf(" [배열 기반]\n");
    printf("  1. 이진트리 출력\n");
    printf("  2. 트리 정보 출력\n");
    printf("  3. 이진트리 형태 판별\n");
    printf("  4. 노드 검색(자식/부모/형제)\n");
    printf(" [연결 자료구조]\n");
    printf("  5. 이진트리 출력\n");
    printf("  6. 트리 정보 출력\n");
    printf("  7. 이진트리 형태 판별\n");
    printf("  8. 노드 검색(자식/부모/형제)\n");
    printf(" [비교]\n");
    printf("  9. 메모리 사용량 비교 (일반/완전/편향 예시)\n");
    printf("  0. 새 트리 입력\n");
    printf(" -1. 종료\n");
    printf("--------------------------------------------------------------\n");
    printf(" 선택 > ");
}

static char *readLine(void) {
    static char buf[2048];
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        buf[0] = '\0';
        return buf;
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return buf;
}

int main(void) {
    Node *root = NULL;
    ArrTree arrT = {NULL, 0};
    int hasTree = 0;

    printf("이진트리를 괄호 표기법으로 입력하세요.\n");
    printf("예) A(B(,D),C)   /  leaf는 괄호 없이 문자 하나만 적습니다.\n");
    printf("입력 > ");
    char inputBuf[2048];
    strcpy(inputBuf, readLine());
    root = buildTreeFromString(inputBuf);
    arrT = buildArrayFromLinked(root);
    hasTree = 1;

    while (1) {
        printMenu();
        char *sel = readLine();
        int choice;
        if (sscanf(sel, "%d", &choice) != 1) continue;

        if (choice == -1) {
            break;
        } else if (choice == 0) {
            printf("새 이진트리를 괄호 표기법으로 입력하세요.\n입력 > ");
            strcpy(inputBuf, readLine());
            if (hasTree) { freeTree(root); freeArrTree(&arrT); }
            root = buildTreeFromString(inputBuf);
            arrT = buildArrayFromLinked(root);
            hasTree = 1;
            continue;
        }

        if (!hasTree) { printf("먼저 트리를 입력하세요.\n"); continue; }

        switch (choice) {
            case 1:
                printf("\n[배열 기반] 이진트리 출력 (왼쪽으로 90도 눕힌 형태)\n");
                printTreeArray(&arrT, 1, 0);
                break;
            case 2:
                printf("\n[배열 기반] 트리 정보\n");
                printTreeInfoArray(&arrT);
                break;
            case 3:
                printf("\n[배열 기반] 이진트리 형태 판별\n");
                printShapeArray(&arrT);
                break;
            case 4: {
                printf("검색할 노드 값(문자 1개) 입력 > ");
                char *s = readLine();
                if (strlen(s) >= 1) queryNodeArray(&arrT, s[0]);
                break;
            }
            case 5:
                printf("\n[연결 자료구조] 이진트리 출력 (왼쪽으로 90도 눕힌 형태)\n");
                printTreeLinked(root, 0);
                break;
            case 6:
                printf("\n[연결 자료구조] 트리 정보\n");
                printTreeInfoLinked(root);
                break;
            case 7:
                printf("\n[연결 자료구조] 이진트리 형태 판별\n");
                printShapeLinked(root);
                break;
            case 8: {
                printf("검색할 노드 값(문자 1개) 입력 > ");
                char *s = readLine();
                if (strlen(s) >= 1) queryNodeLinked(root, s[0]);
                break;
            }
            case 9:
                memoryCompareAll();
                break;
            default:
                printf("잘못된 선택입니다.\n");
        }
    }

    if (hasTree) { freeTree(root); freeArrTree(&arrT); }
    printf("프로그램을 종료합니다.\n");
    return 0;
}
