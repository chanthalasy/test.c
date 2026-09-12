#include <stdio.h>
#include <string.h>

#define MAX 1000

/* =========================
   Node Stack
   ใช้หา Parent ของ C
   ========================= */
typedef struct {
    char data[MAX];
    int top;
} NodeStack;

void initNodeStack(NodeStack *s)
{
    s->top = -1;
}

void pushNode(NodeStack *s, char data)
{
    s->data[++s->top] = data;
}

char popNode(NodeStack *s)
{
    return s->data[s->top--];
}

char peekNode(NodeStack *s)
{
    return s->data[s->top];
}

int isNodeStackEmpty(NodeStack *s)
{
    return s->top == -1;
}


/* =========================
   Degree Stack
   ใช้คำนวณ Degree ของแต่ละ Node
   ========================= */
typedef struct {
    int data[MAX];
    int top;
} DegreeStack;

void initDegreeStack(DegreeStack *s)
{
    s->top = -1;
}

void pushDegree(DegreeStack *s, int data)
{
    s->data[++s->top] = data;
}

int popDegree(DegreeStack *s)
{
    return s->data[s->top--];
}

int isDegreeStackEmpty(DegreeStack *s)
{
    return s->top == -1;
}


/* =========================
   Counter Stack
   ใช้หา Children ของ C
   ========================= */
typedef struct {
    int data[MAX];
    int top;
} CounterStack;

void initCounterStack(CounterStack *s)
{
    s->top = -1;
}

void pushCounter(CounterStack *s, int data)
{
    s->data[++s->top] = data;
}

int popCounter(CounterStack *s)
{
    return s->data[s->top--];
}

int isCounterStackEmpty(CounterStack *s)
{
    return s->top == -1;
}


/* =========================
   Main
   ========================= */
int main(void)
{
    char input[MAX];

    NodeStack nodeStack;
    DegreeStack degreeStack;
    CounterStack counterStack;

    initNodeStack(&nodeStack);
    initDegreeStack(&degreeStack);
    initCounterStack(&counterStack);

    /* =========================
       รับ Input
       ========================= */
    printf("트리를 괄호 표기법으로 입력하세요: ");
    scanf("%999s", input);


    /* =========================
       ตัวแปรสำหรับคำนวณ
       ========================= */
    int totalNodes = 0;
    int leafNodes = 0;
    int nonLeafNodes = 0;

    int height = 0;
    int currentDepth = 0;

    int treeDegree = 0;

    int valid = 1;

    /* ตรวจสอบ Node ซ้ำ */
    int used[26] = {0};

    /* Parent ของ C */
    char parentOfC = '-';

    /* Children ของ C */
    char childrenOfC[26];
    int cChildCount = 0;

    /* ใช้ควบคุมรูปแบบ */
    int expectNode = 1;
    int afterClose = 0;


    /* =========================
       ตรวจสอบ + คำนวณ
       Scan String
       ========================= */

    for (int i = 0; input[i] != '\0' && valid; i++)
    {
        char ch = input[i];


        /* =====================
           ต้องการ Node
           ===================== */
        if (expectNode)
        {
            /* ต้องเป็น A-Z */
            if (ch < 'A' || ch > 'Z')
            {
                valid = 0;
                break;
            }

            /* Root ต้องเป็น A */
            if (i == 0 && ch != 'A')
            {
                valid = 0;
                break;
            }

            /* ห้าม Node ซ้ำ */
            if (used[ch - 'A'])
            {
                valid = 0;
                break;
            }

            used[ch - 'A'] = 1;

            /* =================
               Node Count
               ================= */
            totalNodes++;


            /* =================
               หา Parent ของ C
               ================= */
            if (ch == 'C')
            {
                if (!isNodeStackEmpty(&nodeStack))
                {
                    parentOfC = peekNode(&nodeStack);
                }
            }


            /* =================
               หา Children ของ C
               ================= */

            if (!isNodeStackEmpty(&nodeStack))
            {
                if (peekNode(&nodeStack) == 'C')
                {
                    childrenOfC[cChildCount] = ch;
                    cChildCount++;
                }
            }


            /* =================
               เพิ่ม Degree ของ Parent
               ================= */

            if (!isDegreeStackEmpty(&degreeStack))
            {
                degreeStack.data[degreeStack.top]++;
            }

            /* Counter Stack */
            if (!isCounterStackEmpty(&counterStack))
            {
                counterStack.data[counterStack.top]++;
            }


            /* =================
               Depth / Height
               ================= */

            currentDepth++;

            if (currentDepth > height)
            {
                height = currentDepth;
            }


            /* =================
               Leaf / Non-leaf
               ================= */

            if (input[i + 1] == '(')
            {
                nonLeafNodes++;
            }
            else
            {
                leafNodes++;
                currentDepth--;
            }

            expectNode = 0;
            afterClose = 0;
        }


        /* =====================
           หลังจากอ่าน Node
           ===================== */
        else if (!afterClose)
        {
            /* Node มี Children */
            if (ch == '(')
            {
                /*
                 * Node ตัวก่อนหน้า
                 * ถูก Push เข้า Node Stack
                 */
                pushNode(&nodeStack, input[i - 1]);

                /*
                 * เริ่มนับ Degree ของ Node
                 */
                pushDegree(&degreeStack, 0);

                /*
                 * เริ่ม Counter
                 */
                pushCounter(&counterStack, 0);

                expectNode = 1;
            }


            /* เจอ comma */
            else if (ch == ',')
            {
                /*
                 * comma ต้องอยู่ภายใน ()
                 */
                if (isNodeStackEmpty(&nodeStack))
                {
                    valid = 0;
                }
                else
                {
                    expectNode = 1;
                }
            }


            /* เจอ ) */
            else if (ch == ')')
            {
                if (isNodeStackEmpty(&nodeStack))
                {
                    valid = 0;
                }
                else
                {
                    /* =================
                       Degree ของ Node
                       ================= */

                    int degree = popDegree(&degreeStack);

                    if (degree > treeDegree)
                    {
                        treeDegree = degree;
                    }


                    /* =================
                       Counter Stack
                       ================= */

                    int count = popCounter(&counterStack);

                    /*
                     * ถ้า Node ที่ปิดคือ C
                     * count = จำนวนลูกของ C
                     */
                    (void)count;


                    /* =================
                       Node Stack
                       ================= */

                    popNode(&nodeStack);

                    currentDepth--;

                    afterClose = 1;
                }
            }


            /* ตัวอักษรอื่น = ผิด */
            else
            {
                valid = 0;
            }
        }


        /* =====================
           หลังจากเจอ )
           ===================== */
        else
        {
            if (ch == ',')
            {
                if (isNodeStackEmpty(&nodeStack))
                {
                    valid = 0;
                }
                else
                {
                    expectNode = 1;
                    afterClose = 0;
                }
            }

            else if (ch == ')')
            {
                if (isNodeStackEmpty(&nodeStack))
                {
                    valid = 0;
                }
                else
                {
                    int degree = popDegree(&degreeStack);

                    if (degree > treeDegree)
                    {
                        treeDegree = degree;
                    }

                    popCounter(&counterStack);
                    popNode(&nodeStack);

                    currentDepth--;
                }
            }

            else
            {
                valid = 0;
            }
        }
    }


    /* =========================
       ตรวจสอบตอนจบ
       ========================= */

    if (expectNode)
    {
        valid = 0;
    }

    if (!isNodeStackEmpty(&nodeStack))
    {
        valid = 0;
    }

    if (!isDegreeStackEmpty(&degreeStack))
    {
        valid = 0;
    }

    if (!isCounterStackEmpty(&counterStack))
    {
        valid = 0;
    }


    /* =========================
       ถ้า Input ผิด
       ========================= */

    if (!valid)
    {
        printf("\n잘못된 트리 표기법입니다.\n");
        return 0;
    }


    /* =========================
       Output
       ========================= */

    printf("\n===== Tree Information =====\n");

    printf("전체 노드의 수: %d\n", totalNodes);

    printf("단말 노드의 수: %d\n", leafNodes);

    printf("비단말 노드의 수: %d\n", nonLeafNodes);

    printf("트리의 높이: %d\n", height);

    printf("트리의 차수: %d\n", treeDegree);


    /* =========================
       Parent of C
       ========================= */

    printf("노드 C의 부모 노드: ");

    if (parentOfC == '-')
    {
        printf("없음\n");
    }
    else
    {
        printf("%c\n", parentOfC);
    }


    /* =========================
       Children of C
       ========================= */

    printf("노드 C의 자식 노드: ");

    if (cChildCount == 0)
    {
        printf("없음\n");
    }
    else
    {
        for (int i = 0; i < cChildCount; i++)
        {
            printf("%c", childrenOfC[i]);

            if (i < cChildCount - 1)
            {
                printf(", ");
            }
        }

        printf("\n");
    }


    /* =========================
       Tree แบบนอนซ้าย
       ========================= */

    printf("\n===== Tree =====\n");

    int depth = 0;

    for (int i = 0; input[i] != '\0'; i++)
    {
        char ch = input[i];

        if (ch >= 'A' && ch <= 'Z')
        {
            if (depth == 0)
            {
                printf("%c\n", ch);
            }
            else
            {
                for (int j = 1; j < depth; j++)
                {
                    printf("    ");
                }

                printf("+---%c\n", ch);
            }

            /* ถ้า Node มีลูก */
            if (input[i + 1] == '(')
            {
                depth++;
            }
        }

        else if (ch == ')')
        {
            depth--;
        }
    }


    return 0;
}
