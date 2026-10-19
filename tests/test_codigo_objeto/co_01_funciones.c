#include <stdio.h>
extern long cs(), ops(long,long), cmp(long,long), suma(long), fact(long), fib(long), llama(), param(long);
static int fallos = 0;
#define CHK(e, esp) do { long v = (e); if (v != (esp)) { printf("FALLO %s = %ld (esperado %ld)\n", #e, v, (long)(esp)); fallos++; } } while (0)
int main(void) {
    CHK(cs(), 2); /* g > 5 es falso: tocar() no se evalua en el if */
    CHK(ops(17,5), 17*5 + 17/5 - 17%5 + -17);
    CHK(ops(-17,5), -17*5 + -17/5 - -17%5 + 17);
    CHK(cmp(1,2), 1001); CHK(cmp(2,1), 1010); CHK(cmp(3,3), 105);
    CHK(suma(10), 55); CHK(suma(0), 0);
    CHK(fact(5), 120); CHK(fib(10), 55);
    CHK(llama(), 204 + 28*6 + 85);
    CHK(param(4), 5);
    printf(fallos ? "%d fallos\n" : "todo OK\n", fallos);
    return fallos != 0;
}
