#include <stdio.h>
#include <glib.h>


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
int main() {
    GList *list = NULL;

    list = g_list_append(list, "Hello, World!");
    printf("The first item is %s\n", (char *) g_list_first(list)->data);
    printf("The length of list is %d\n", g_list_length(list));

    g_list_append(list, "Good Morning!");
    printf("The length of list is %d\n", g_list_length(list));
    printf("The last item is %s\n", (char *) g_list_last(list)->data);

    g_list_free(list);
    return 0;
}
#pragma clang diagnostic pop