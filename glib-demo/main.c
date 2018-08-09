#include <stdio.h>
#include <glib.h>


int main() {
    GList *list = NULL;
    list = g_list_append(list, "Hello, World!\n");
    printf("The first item is %s", (char *) g_list_first(list)->data);
    g_list_free(list);
    return 0;
}