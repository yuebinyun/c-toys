//
// Created by ban on 2019/11/21.
//
#include "staticTest.h"

static void staticFunc(void) {
    printf("call staticFunc\n");
}

void noStaticFunc(void) {
    printf("call noStaticFunc\n");
    staticFunc();
}



