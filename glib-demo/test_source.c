#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

/* The bulk of the code. */
GSource *
message_queue_source_new (GAsyncQueue    *queue,
                          GDestroyNotify  destroy_message,
                          GCancellable   *cancellable);

#include "source.h"

/* Utility functions. */
static gboolean
source_cb (gpointer message,
           gpointer user_data)
{
    guint *seen_i = user_data;

    g_assert_cmpuint (GPOINTER_TO_UINT (message), ==, *seen_i);
    *seen_i = *seen_i + 1;

    return TRUE;
}

static guint global_destroy_calls = 0;

static void
destroy_cb (gpointer user_data)
{
    global_destroy_calls++;
}

/* Tests. */
static void
test_construction (void)
{
    GSource *source = NULL;
    GAsyncQueue *queue = NULL;

    queue = g_async_queue_new ();
    source = message_queue_source_new (queue, NULL, NULL);

    g_source_unref (source);
    g_async_queue_unref (queue);
}

static void
test_messages_with_callback (void)
{
    GSource *source;
    GAsyncQueue *queue;
    guint i, seen_i;
    GMainContext *main_context;

    queue = g_async_queue_new_full (destroy_cb);
    source = message_queue_source_new (queue, destroy_cb, NULL);

    /* Enqueue some messages. */
    for (i = 1; i <= 100; i++)
    {
        g_async_queue_push (queue, GUINT_TO_POINTER (i));
    }

    /* Create and run a main loop. Expect to see the messages in order. */
    seen_i = 1;
    global_destroy_calls = 0;

    main_context = g_main_context_new ();
    g_source_set_callback (source, (GSourceFunc) source_cb, &seen_i, NULL);
    g_source_attach (source, main_context);

    while (g_main_context_iteration (main_context, FALSE));

    g_assert_cmpuint (global_destroy_calls, ==, 0);
    g_assert_cmpuint (seen_i, ==, 101);
    g_assert_cmpuint (g_async_queue_length (queue), ==, 0);

    g_main_context_unref (main_context);
    g_async_queue_unref (queue);
    g_source_unref (source);
}

static void
test_messages_without_callback (void)
{
    GSource *source;
    GAsyncQueue *queue;
    guint i;
    GMainContext *main_context;

    queue = g_async_queue_new_full (destroy_cb);
    source = message_queue_source_new (queue, destroy_cb, NULL);

    /* Enqueue some messages. */
    for (i = 1; i <= 100; i++)
    {
        g_async_queue_push (queue, GUINT_TO_POINTER (i));
    }

    /* Create and run a main loop. */
    main_context = g_main_context_new ();
    g_source_attach (source, main_context);

    while (g_main_context_iteration (main_context, FALSE));

    g_assert_cmpuint (global_destroy_calls, ==, 100);
    g_assert_cmpuint (g_async_queue_length (queue), ==, 0);

    g_main_context_unref (main_context);
    g_async_queue_unref (queue);
    g_source_unref (source);
}

int
main (int    argc,
      char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/custom-gsource/construction", test_construction);
    g_test_add_func ("/custom-gsource/messages-with-callback",
                     test_messages_with_callback);
    g_test_add_func ("/custom-gsource/messages-without-callback",
                     test_messages_without_callback);

    return g_test_run ();
}
