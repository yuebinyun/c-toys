#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>
//#include <gio/gtask.h>
#include "debug.h"

void testGList();

gboolean cb(gpointer user_data) {
    JANUS_LOG(LOG_INFO, "hello [%d] thread id %ld\n", (int *)user_data, pthread_self());
    return G_SOURCE_CONTINUE;
}

static void lsdir_cb(GTask *task, GValue *result, gpointer user_date) {
    const gchar *path = user_date;
    const gchar *name;
    GDir *dir;
    dir = g_dir_open(path, 0, NULL);
    while ((name = g_dir_read_name(dir)) != NULL) {
        g_print("%s\n", name);
    }
}

static void *fun(void *user_data) {
    JANUS_LOG(LOG_INFO, "run in thread %ld\n", pthread_self());

    int cb3 = 10;
    GMainLoop *l = g_main_loop_new(NULL, FALSE);
    g_timeout_add_seconds(1, cb, &cb3);
    g_main_loop_run(l);

    return NULL;
}

int main() {

    GThread *thread = g_thread_try_new("new-thread", fun, "data", NULL);

//    JANUS_LOG(LOG_WARN, "debug : %s thread id : %ld\n", "hello", pthread_self());
//
//    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
//    int cb1 = 9;
//    int cb2 = 9;
//
//    g_timeout_add_seconds(2, cb, &cb1);
//    g_timeout_add_seconds(3, cb, &cb2);
//
//    g_main_loop_run(loop);
    g_thread_join(thread);

    return 0;
}

void testGList() {
    GList *list = NULL;

    list = g_list_append(list, "Hello, World!");
    printf("The first item is %s\n", (char *) g_list_first(list)->data);
    printf("The length of list is %d\n", g_list_length(list));

    list = g_list_append(list, "Good Morning!");
    printf("The length of list is %d\n", g_list_length(list));
    printf("The last item is %s\n", (char *) g_list_last(list)->data);

    list = g_list_insert(list, "你好！", 0);
    printf("The length of list is %d\n", g_list_length(list));
    printf("The first item is %s\n", (char *) g_list_first(list)->data);

    g_list_free(list);
}


