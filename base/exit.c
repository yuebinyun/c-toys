#include <stdio.h>
#include <glib.h>
#include <zconf.h>

void threadTest();

void signalTest();

void asyncQueueSafeTest();

void staticFuncTest();

void constIntTest();

void constCharTest();

void constFunTest();

gpointer func(gpointer user_data) {
    g_usleep(G_USEC_PER_SEC * 5);
//    exit(0);
    g_print("exit thread, bye!\n");
    return NULL;
}

static int isExit = 0;

static void handle(int signum) {
    g_print("\nsignal num %d\n", signum);
    isExit = 1;
}

static void omega(void) {
    g_print("do last exit...\n");
}


static guint MAX_COUNT = 1000;
static int a[3000] = {0};
static GAsyncQueue *queue = NULL;

static gpointer pushInt(gpointer base) {
    int bb = GPOINTER_TO_INT(base);
    g_print("base %d\n", bb);
    g_usleep(G_USEC_PER_SEC);

    for (guint i = 0; i < MAX_COUNT; ++i) {
        g_async_queue_push(queue, &a[bb++]);
    }

    g_usleep(G_USEC_PER_SEC);
    g_print("exit sub thread \n");
    return NULL;
}

static gpointer popInt(gpointer user_data) {
    for (int i = 0; i < 3000; ++i) {
        if (i % 30 == 0) g_print("\n");
        int *value = g_async_queue_pop(queue);
        g_print("%4d ", *value);
    }
    g_print("\n");
    g_print("exit consumer thread\n");
    return NULL;
}

#include "staticTest.h"

int main() {
    printf("hello... \n");
    // threadTest();
    // signalTest();
    // asyncQueueSafeTest();
    // staticFuncTest();

//    constIntTest();
//    constCharTest();
//    constFunTest();

    g_print("exit main thread, bye!\n");
    return 0;
}

void constFunTest() {
    const int A = 5;
    const int *const PA = &A;

    int z = 5;
    const int *const Pz = &z;
    printf("%d\n", *Pz);
//    *Pz = 4;
    z = 3;
    printf("%d\n", *Pz);
}

void constCharTest() {// abc 是指针，指针本身可以修改，指针指向地址的值不能修改。
    const char *aaa = "aaa";
    aaa = "2222";           // 修改指针
// *aaa = 'b';          // 指针指向地址不能修改
// *(aaa+1) = 'c'       // error
    g_print("message : %s\n", aaa);

//    char *temp = "abc";
//    *temp = '1';
//    g_print("message : %s", temp);

    // bbb 是指针，且地址不能改变，但是值可以改变
/*! 可以防止内存泄漏呀，毕竟指针不能修改 */
    char *const bbb = g_malloc0(4);
//    char arr[] = "abc";
//    char *const bbb = &arr;
//     bbb = "222";         // error
    *bbb = '1';             // right
    *(bbb + 1) = '1';       // right
    *(bbb + 2) = '1';       // right
    g_print("message : %s", bbb);
}

void constIntTest() {
    int i = 99;
    int ii = 88;
    const int *aa = &i;     // aa 指针指向的值不能修改，指针本身可改变
    printf("p &i = %p, v i = %d, "
           "p &ii = %p, v ii = %d, "
           "p aa = %p, v *aa = %d\n",
           &i, i, &ii, ii, aa, *aa);
    // *aa = 9;             // error
    aa = &ii;               // right
    printf("p &i = %p, v i = %d, "
           "p &ii = %p, v ii = %d, "
           "p aa = %p, v *aa = %d\n",
           &i, i, &ii, ii, aa, *aa);

    int const *bb = &i;     // bb 指针指向的值不能修改，指针本身可改变
    printf("p &i = %p, v i = %d, "
           "p &ii = %p, v ii = %d, "
           "p aa = %p, v *aa = %d\n",
           &i, i, &ii, ii, bb, *bb);
    // *bb = 9;             // error
    bb = &ii;               // right
    printf("p &i = %p, v i = %d, "
           "p &ii = %p, v ii = %d, "
           "p aa = %p, v *aa = %d\n",
           &i, i, &ii, ii, bb, *bb);

    int *const cc = &i;     // cc 指针指向的值能修改，指针本身不能修改。
    printf("p &i = %p, v i = %d, "
           "p &ii = %p, v ii = %d, "
           "p aa = %p, v *aa = %d\n",
           &i, i, &ii, ii, cc, *cc);
    *cc = 999;              // right
    // cc = &ii;            // error
    printf("p &i = %p, v i = %d, "
           "p &ii = %p, v ii = %d, "
           "p aa = %p, v *aa = %d\n",
           &i, i, &ii, ii, cc, *cc);
}

/**
 * static function : 表示这个方法只在定义所在的 .c 文件可用
 */
void staticFuncTest() {
    // staticFunc();
    noStaticFunc();
}

void asyncQueueSafeTest() {
    queue = g_async_queue_new();
    int arrayLen = sizeof(a) / sizeof(int);
    g_print("size of a %d\n", arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        a[i] = i;
    }
    GThread *t1 = g_thread_try_new("t1", pushInt, GINT_TO_POINTER(0), NULL);
    GThread *t2 = g_thread_try_new("t2", pushInt, GINT_TO_POINTER(1000), NULL);
    GThread *t3 = g_thread_try_new("t3", pushInt, GINT_TO_POINTER(2000), NULL);
    GThread *t4 = g_thread_try_new("t4", popInt, NULL, NULL);
    g_thread_join(t1);
    g_thread_unref(t1);
    t1 = NULL;
    g_thread_join(t2);
    g_thread_unref(t2);
    t2 = NULL;
    g_thread_join(t3);
    g_thread_unref(t3);
    t3 = NULL;
    g_thread_join(t4);
    g_thread_unref(t4);
    t4 = NULL;
    g_print("length of queue %d\n", g_async_queue_length(queue));
    g_async_queue_unref(queue);
    queue = NULL;
}


/**
 * [Ctrl+C] signal num = 2, signal name = SIGINT
 * usleep 会因为收到 signal 而结束休眠
 * g_usleep 不会因为收到 signal 而休眠
 */
void signalTest() {
    signal(SIGINT, handle);
    signal(SIGTERM, handle);
    atexit(omega);
    while (!isExit) {
        g_usleep(G_USEC_PER_SEC * 5);
//        usleep(G_USEC_PER_SEC * 5);
    }
}

/**
 * 主线程 exit(), 程序退出
 * 子线程 exit(), 程序退出
 */
void threadTest() {
    GThread *thread = g_thread_try_new("thread-name", func, GINT_TO_POINTER(10), NULL);

    g_usleep(G_USEC_PER_SEC * 10);
    g_thread_join(thread);
    g_thread_unref(thread);
}
