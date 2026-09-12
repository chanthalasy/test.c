#include <stdio.h>
#include <string.h>

#define MAX 1000
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

int main(void)
{
    char input[MAX];

    NodeStack nodeStack;
    DegreeStack degreeStack;
    CounterStack counterStack;

    initNodeStack(&nodeStack);
    initDegreeStack(&degreeStack);
    initCounterStack(&counterStack);

    printf("트리를 괄호 표기법으로 입력하세요: ");
    scanf("%999s", input);

    int totalNodes = 0;
    int leafNodes = 0;
    int nonLeafNodes = 0;

    int height = 0;
    int currentDepth = 0;

    int treeDegree = 0;

    int valid = 1;

    int used[26] = {0};

    char parentOfC = '-';

    char childrenOfC[26];
    int cChildCount = 0;

    int expectNode = 1;
    int afterClose = 0;


    for (int i = 0; input[i] != '\0' && valid; i++)
    {
        char ch = input[i];

        if (expectNode)
        {
            
            if (ch < 'A' || ch > 'Z')
            {
                valid = 0;
                break;
            }


            if (i == 0 && ch != 'A')
            {
                valid = 0;
                break;
            }

            if (used[ch - 'A'])
            {
                valid = 0;
                break;
            }

            used[ch - 'A'] = 1;

          
            totalNodes++;

            if (ch == 'C')
            {
                if (!isNodeStackEmpty(&nodeStack))
                {
                    parentOfC = peekNode(&nodeStack);
                }
            }


            if (!isNodeStackEmpty(&nodeStack))
            {
                if (peekNode(&nodeStack) == 'C')
                {
                    childrenOfC[cChildCount] = ch;
                    cChildCount++;
                }
            }


            if (!isDegreeStackEmpty(&degreeStack))
            {
                degreeStack.data[degreeStack.top]++;
            }

            
            if (!isCounterStackEmpty(&counterStack))
            {
                counterStack.data[counterStack.top]++;
            }

            currentDepth++;

            if (currentDepth > height)
            {
                height = currentDepth;
            }


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


     
        else if (!afterClose)
        {
         
            if (ch == '(')
            {
               
                pushNode(&nodeStack, input[i - 1]);

                pushDegree(&degreeStack, 0);

                pushCounter(&counterStack, 0);

                expectNode = 1;
            }

            else if (ch == ',')
            {
            
                if (isNodeStackEmpty(&nodeStack))
                {
                    valid = 0;
                }
                else
                {
                    expectNode = 1;
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


                    int count = popCounter(&counterStack);

                  
                    (void)count;

                    popNode(&nodeStack);

                    currentDepth--;

                    afterClose = 1;
                }
            }

            else
            {
                valid = 0;
            }
        }

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

    if (!valid)
    {
        printf("\n잘못된 트리 표기법입니다.\n");
        return 0;
    }


    printf("\n Tree Information :\n");

    printf("전체 노드의 수: %d\n", totalNodes);

    printf("단말 노드의 수: %d\n", leafNodes);

    printf("비단말 노드의 수: %d\n", nonLeafNodes);

    printf("트리의 높이: %d\n", height);

    printf("트리의 차수: %d\n", treeDegree);

    printf("노드 C의 부모 노드: ");

    if (parentOfC == '-')
    {
        printf("없음\n");
    }
    else
    {
        printf("%c\n", parentOfC);
    }


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
