#include <stdio.h>
#include <glib.h>
#include "debug.h"

int main() {
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

    janus_log_level = 3;
    JANUS_LOG(LOG_WARN, "debug : %s", "hello");
    return 0;
}
