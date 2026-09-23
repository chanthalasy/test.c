#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEFAULT_SIZE 1000
#define MAX_LINE 256
#define MAX_TOKENS 8

/* ---------------------- 이진트리 ADT ---------------------- */

typedef struct {
    char data;
    int  left;     /* 왼쪽 자식의 배열 인덱스, 없으면 -1 */
    int  right;    /* 오른쪽 자식의 배열 인덱스, 없으면 -1 */
    int  parent;   /* 부모의 배열 인덱스, 없으면 -1 */
    int  used;     /* 슬롯 사용 여부 */
} TreeNode;

typedef struct {
    TreeNode *nodes;
    int size;   /* 최대 저장 가능 노드 수 */
    int count;  /* 현재 노드 수 */
    int root;   /* 루트 노드의 배열 인덱스, 트리가 비어 있으면 -1 */
} BTree;

/* 반환 코드 (연산 성공/실패를 호출부에서 구분하기 위해 사용) */
enum {
    OK = 0,
    ERR_TREE_FULL = -1,
    ERR_ROOT_EXISTS = -2,
    ERR_NOT_EMPTY = -3,       /* insert_root: 트리가 비어있지 않음 */
    ERR_NODE_NOT_FOUND = -4,
    ERR_CHILD_TAKEN = -5,     /* 지정한 위치(L/R)에 이미 자식이 있음 */
    ERR_DUP_SIBLING = -6,     /* 형제 노드와 데이터가 같음 */
    ERR_NOT_LEAF = -7,        /* 삭제 대상이 단말 노드가 아님 */
    ERR_EMPTY_TREE = -8
};

BTree *create_btree(int size) {
    BTree *t = (BTree *)malloc(sizeof(BTree));
    t->nodes = (TreeNode *)malloc(sizeof(TreeNode) * size);
    for (int i = 0; i < size; i++) t->nodes[i].used = 0;
    t->size = size;
    t->count = 0;
    t->root = -1;
    return t;
}

static int alloc_slot(BTree *t) {
    for (int i = 0; i < t->size; i++)
        if (!t->nodes[i].used) return i;
    return -1;
}

int insert_root(BTree *t, char value) {
    if (t->root != -1) return ERR_ROOT_EXISTS;
    int idx = alloc_slot(t);
    if (idx == -1) return ERR_TREE_FULL;
    t->nodes[idx].data = value;
    t->nodes[idx].left = -1;
    t->nodes[idx].right = -1;
    t->nodes[idx].parent = -1;
    t->nodes[idx].used = 1;
    t->root = idx;
    t->count++;
    return idx;
}

/* child: 'L' 또는 'R' */
int insert_child(BTree *t, int parent, char child, char value) {
    if (parent < 0 || parent >= t->size || !t->nodes[parent].used)
        return ERR_NODE_NOT_FOUND;

    int *slot = (child == 'L') ? &t->nodes[parent].left : &t->nodes[parent].right;
    int *sibling = (child == 'L') ? &t->nodes[parent].right : &t->nodes[parent].left;

    if (*slot != -1) return ERR_CHILD_TAKEN;
    if (*sibling != -1 && t->nodes[*sibling].data == value) return ERR_DUP_SIBLING;

    int idx = alloc_slot(t);
    if (idx == -1) return ERR_TREE_FULL;

    t->nodes[idx].data = value;
    t->nodes[idx].left = -1;
    t->nodes[idx].right = -1;
    t->nodes[idx].parent = parent;
    t->nodes[idx].used = 1;
    *slot = idx;
    t->count++;
    return idx;
}

int delete_node(BTree *t, int leaf) {
    if (leaf < 0 || leaf >= t->size || !t->nodes[leaf].used)
        return ERR_NODE_NOT_FOUND;
    if (t->nodes[leaf].left != -1 || t->nodes[leaf].right != -1)
        return ERR_NOT_LEAF;

    int parent = t->nodes[leaf].parent;
    if (parent == -1) {
        t->root = -1;
    } else {
        if (t->nodes[parent].left == leaf) t->nodes[parent].left = -1;
        else t->nodes[parent].right = -1;
    }
    t->nodes[leaf].used = 0;
    t->count--;
    return OK;
}

int update_value(BTree *t, int node, char value) {
    if (node < 0 || node >= t->size || !t->nodes[node].used)
        return ERR_NODE_NOT_FOUND;

    int parent = t->nodes[node].parent;
    if (parent != -1) {
        int sibling = (t->nodes[parent].left == node) ? t->nodes[parent].right
                                                        : t->nodes[parent].left;
        if (sibling != -1 && t->nodes[sibling].data == value)
            return ERR_DUP_SIBLING;
    }
    t->nodes[node].data = value;
    return OK;
}

/* out_left/out_right 에 자식의 데이터를 채워 넣고, 존재 여부를 return value 로 인코딩:
   비트0 = 왼쪽 존재, 비트1 = 오른쪽 존재 */
int read_child(BTree *t, int parent, char *out_left, char *out_right) {
    if (parent < 0 || parent >= t->size || !t->nodes[parent].used)
        return ERR_NODE_NOT_FOUND;
    int flags = 0;
    if (t->nodes[parent].left != -1) {
        *out_left = t->nodes[t->nodes[parent].left].data;
        flags |= 1;
    }
    if (t->nodes[parent].right != -1) {
        *out_right = t->nodes[t->nodes[parent].right].data;
        flags |= 2;
    }
    return flags;
}

static void print_subtree(BTree *t, int node, int depth) {
    if (node == -1) return;
    if (depth == 0) {
        printf("%c\n", t->nodes[node].data);
    } else {
        for (int i = 0; i < (depth - 1) * 4; i++) putchar(' ');
        printf("+---%c\n", t->nodes[node].data);
    }
    print_subtree(t, t->nodes[node].left, depth + 1);
    print_subtree(t, t->nodes[node].right, depth + 1);
}

int print_btree(BTree *t) {
    if (t->root == -1) return ERR_EMPTY_TREE;
    print_subtree(t, t->root, 0);
    return OK;
}

void destroy_btree(BTree *t) {
    free(t->nodes);
    free(t);
}

/* ---------------------- 경로(path) 처리 ---------------------- */

/* "/A/B/C" 형태의 경로를 노드 인덱스로 변환. 없으면 -1. */
int find_by_path(BTree *t, const char *path) {
    if (path[0] != '/') return -1;

    char buf[MAX_LINE];
    strncpy(buf, path, MAX_LINE - 1);
    buf[MAX_LINE - 1] = '\0';

    char *tok = strtok(buf, "/");
    if (tok == NULL) return -1; /* "/" 만 있는 경우: 노드를 가리키지 않음 */

    if (t->root == -1) return -1;
    if (strlen(tok) != 1 || t->nodes[t->root].data != tok[0]) return -1;

    int cur = t->root;
    tok = strtok(NULL, "/");
    while (tok != NULL) {
        if (strlen(tok) != 1) return -1;
        char c = tok[0];
        int next = -1;
        if (t->nodes[cur].left != -1 && t->nodes[t->nodes[cur].left].data == c)
            next = t->nodes[cur].left;
        else if (t->nodes[cur].right != -1 && t->nodes[t->nodes[cur].right].data == c)
            next = t->nodes[cur].right;
        if (next == -1) return -1;
        cur = next;
        tok = strtok(NULL, "/");
    }
    return cur;
}

/* ---------------------- 명령 처리 ---------------------- */

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static int is_upper_letter(const char *s) {
    return strlen(s) == 1 && isupper((unsigned char)s[0]);
}

static void cmd_insert(BTree *t, char **arg, int nargs) {
    if (nargs == 2) {
        /* Insert / A : 루트 생성 */
        if (strcmp(arg[0], "/") != 0) {
            printf("오류: 루트 생성은 \"Insert / <데이터>\" 형식이어야 합니다.\n");
            return;
        }
        if (!is_upper_letter(arg[1])) {
            printf("오류: 데이터는 영문 대문자 한 글자여야 합니다.\n");
            return;
        }
        int r = insert_root(t, arg[1][0]);
        if (r == ERR_ROOT_EXISTS) {
            printf("오류: 이미 루트 노드가 존재합니다.\n");
        } else if (r == ERR_TREE_FULL) {
            printf("오류: 트리가 가득 찼습니다.\n");
        } else {
            printf("루트 노드 %c 가 생성되었습니다.\n", arg[1][0]);
        }
        return;
    }

    if (nargs == 3) {
        /* Insert parent-node child new-data */
        char child = toupper((unsigned char)arg[1][0]);
        if (strlen(arg[1]) != 1 || (child != 'L' && child != 'R')) {
            printf("오류: child는 L 또는 R 이어야 합니다.\n");
            return;
        }
        if (!is_upper_letter(arg[2])) {
            printf("오류: 데이터는 영문 대문자 한 글자여야 합니다.\n");
            return;
        }
        int parent = find_by_path(t, arg[0]);
        if (parent == -1) {
            printf("오류: 부모 노드 %s 를 찾을 수 없습니다.\n", arg[0]);
            return;
        }
        int r = insert_child(t, parent, child, arg[2][0]);
        if (r == ERR_CHILD_TAKEN) {
            printf("오류: 지정한 위치에 이미 자식 노드가 존재합니다.\n");
        } else if (r == ERR_DUP_SIBLING) {
            printf("오류: 동일한 부모의 형제 노드와 같은 데이터를 가질 수 없습니다.\n");
        } else if (r == ERR_TREE_FULL) {
            printf("오류: 트리가 가득 찼습니다.\n");
        } else if (r == ERR_NODE_NOT_FOUND) {
            printf("오류: 부모 노드를 찾을 수 없습니다.\n");
        } else {
            printf("%s 의 %s 자식으로 %c 노드가 추가되었습니다.\n",
                   arg[0], child == 'L' ? "왼쪽" : "오른쪽", arg[2][0]);
        }
        return;
    }

    printf("오류: Insert 명령의 인자 개수가 올바르지 않습니다.\n");
}

static void cmd_delete(BTree *t, char **arg, int nargs) {
    if (nargs != 1) {
        printf("오류: Delete 명령의 인자 개수가 올바르지 않습니다.\n");
        return;
    }
    int idx = find_by_path(t, arg[0]);
    if (idx == -1) {
        printf("오류: 노드 %s 를 찾을 수 없습니다.\n", arg[0]);
        return;
    }
    int r = delete_node(t, idx);
    if (r == ERR_NOT_LEAF) {
        printf("오류: 단말 노드만 삭제할 수 있습니다.\n");
    } else {
        printf("%s 노드가 삭제되었습니다.\n", arg[0]);
    }
}

static void cmd_update(BTree *t, char **arg, int nargs) {
    if (nargs != 2) {
        printf("오류: Update 명령의 인자 개수가 올바르지 않습니다.\n");
        return;
    }
    if (!is_upper_letter(arg[1])) {
        printf("오류: 데이터는 영문 대문자 한 글자여야 합니다.\n");
        return;
    }
    int idx = find_by_path(t, arg[0]);
    if (idx == -1) {
        printf("오류: 노드 %s 를 찾을 수 없습니다.\n", arg[0]);
        return;
    }
    int r = update_value(t, idx, arg[1][0]);
    if (r == ERR_DUP_SIBLING) {
        printf("오류: 동일한 부모의 형제 노드와 같은 데이터를 가질 수 없습니다.\n");
    } else {
        printf("%s 노드의 데이터가 %c 로 변경되었습니다.\n", arg[0], arg[1][0]);
    }
}

static void cmd_read(BTree *t, char **arg, int nargs) {
    if (nargs != 1) {
        printf("오류: Read 명령의 인자 개수가 올바르지 않습니다.\n");
        return;
    }
    int idx = find_by_path(t, arg[0]);
    if (idx == -1) {
        printf("오류: 노드 %s 를 찾을 수 없습니다.\n", arg[0]);
        return;
    }
    char lv = 0, rv = 0;
    int flags = read_child(t, idx, &lv, &rv);
    if (flags == 0) {
        printf("단말 노드입니다. 자식이 없습니다.\n");
    } else if (flags == 1) {
        printf("%c(L)\n", lv);
    } else if (flags == 2) {
        printf("%c(R)\n", rv);
    } else {
        printf("%c(L), %c(R)\n", lv, rv);
    }
}

static void cmd_print(BTree *t, int nargs) {
    if (nargs != 0) {
        printf("오류: Print 명령은 인자를 받지 않습니다.\n");
        return;
    }
    int r = print_btree(t);
    if (r == ERR_EMPTY_TREE) {
        printf("트리가 비어 있습니다.\n");
    }
}

/* ---------------------- 메인 루프 ---------------------- */

static int match_cmd(const char *word, const char *full) {
    if (strlen(word) == 1)
        return toupper((unsigned char)word[0]) == full[0];
    if (strcasecmp(word, full) == 0) return 1;
    /* full 이름 접두어로 시작하는 경우도 허용하려면 여기서 처리 가능하지만
       과제 명세는 "전체 이름 또는 첫 글자"만 요구하므로 완전 일치만 허용 */
    return 0;
}

int main(void) {
    BTree *tree = create_btree(DEFAULT_SIZE);
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        trim_newline(line);
        if (line[0] == '\0') continue; /* 빈 줄은 무시 */

        char *tokens[MAX_TOKENS];
        int ntok = 0;
        char *p = strtok(line, " \t");
        while (p != NULL && ntok < MAX_TOKENS) {
            tokens[ntok++] = p;
            p = strtok(NULL, " \t");
        }
        if (ntok == 0) continue;

        char *cmd = tokens[0];
        char **args = &tokens[1];
        int nargs = ntok - 1;

        if (match_cmd(cmd, "Insert")) {
            cmd_insert(tree, args, nargs);
        } else if (match_cmd(cmd, "Delete")) {
            cmd_delete(tree, args, nargs);
        } else if (match_cmd(cmd, "Update")) {
            cmd_update(tree, args, nargs);
        } else if (match_cmd(cmd, "Read")) {
            cmd_read(tree, args, nargs);
        } else if (match_cmd(cmd, "Print")) {
            cmd_print(tree, nargs);
        } else {
            printf("오류: 알 수 없는 명령입니다: %s\n", cmd);
        }
    }

    destroy_btree(tree);
    return 0;
}
