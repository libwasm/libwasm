#include "libwasm.h"

#include <stdio.h>

void** _externalRefs = NULL;

Table spectest__table;
Memory spectest__memory;
uint32_t spectest__global_i32 = 666;

void spectest__print(void)
{
    printf("spectest.print()\n");
}

void spectest__print_i32(uint32_t i)
{
    printf("spectest.print_i32(%d)\n", i);
}

void spectest__print_f32(float f)
{
    printf("spectest.print_f32(%g)\n", f);
}

void spectest__print_i32_f32(uint32_t i, float f)
{
    printf("spectest.print_i32_f32(%d %g)\n", i, f);
}

void spectest__print_f64(double d)
{
    printf("spectest.print_f64(%g)\n", d);
}

void spectest__print_f64_f64(double d1, double d2)
{
    printf("spectest.print_f64_f64(%g %g)\n", d1, d2);
}

void spectest__initialize()
{
    initializeMemory(&spectest__memory, 1, 2);
    initializeTable(&spectest__table, 10, 20);
}

