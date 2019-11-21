//
// Created by ban on 2019/11/21.
//


#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

/**
 * MessageQueueSource:
 *
 * This is a #Gsource which wraps a #GAsyncQueue and is dispatched whenever a
 * message can be pulled off the queue. Messages can be enqueued from any thread.
 *
 * The callbacks dispatched by a #MessageQueueSource have type
 * #MessageQueueSourceFunc.
 *
 * #MessageQueueSource supports adding a #GCancellable child source which will
 * additionally dispatch if a #GCancellable is cancelled.
 */
typedef struct{
    GSource parent;
    GAsyncQueue *queue;
    GDestroyNotify destroy_message;
} MessageQueueSource;

/**
 * MessageQueueSourceFunc:
 * @message: (transfer full) (nullable): message pulled off the queue
 * @user_data: user data provided to g_source_set_callback()
 */
typedef gboolean (*MessageQueueSourceFunc)(gpointer message);

/**
 * handle message pop from the queue
 * @param message
 * @param user_data
 * @return
 */
static gboolean handle_message(gpointer message){
    g_print("handle message %s\n", message);
    if(g_strcmp0(message, "bye") == 0){
        g_print("catch bye message! good day!\n");
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static gboolean message_queue_source_prepare(GSource *source, gint *timeout_){
    g_print("prepare...\n");
    MessageQueueSource *message_queue_source = (MessageQueueSource *) source;
    if(g_async_queue_length(message_queue_source->queue) > 0){
        g_print("length message queue > 0\n");
        return TRUE;
    }else{
        g_print("length message queue <= 0\n");
        return FALSE;
    }
}

static gboolean message_queue_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data){

    g_print("dispatch ... \n");

    MessageQueueSource *message_queue_source = (MessageQueueSource *) source;
    gpointer message;
    MessageQueueSourceFunc func = (MessageQueueSourceFunc) callback;

    /* Pop a message off the queue. */
    message = g_async_queue_try_pop(message_queue_source->queue);

    /* If there was no message, bail. */
    if(message == NULL){
        /* Keep the source around to handle the next message. */
        return TRUE;
    }

    /* @func may be NULL if no callback was specified. */
    /* If so, drop the message. */
    if(func == NULL){
        if(message_queue_source->destroy_message != NULL){
            message_queue_source->destroy_message(message);
        }
        return TRUE;
    }

    return func(message);
}

static void  message_queue_source_finalize(GSource *source){
    MessageQueueSource *message_queue_source = (MessageQueueSource *) source;
    g_async_queue_unref(message_queue_source->queue);
}

static gboolean message_queue_source_closure_callback(gpointer message, gpointer user_data){
    GClosure *closure = user_data;
    GValue param_value = G_VALUE_INIT;
    GValue result_value = G_VALUE_INIT;
    gboolean retval;

    /* The invoked function is responsible for freeing @message. */
    g_value_init(&result_value, G_TYPE_BOOLEAN);
    g_value_init(&param_value, G_TYPE_POINTER);
    g_value_set_pointer(&param_value, message);

    g_closure_invoke(closure, &result_value, 1, &param_value, NULL);
    retval = g_value_get_boolean(&result_value);

    g_value_unset(&param_value);
    g_value_unset(&result_value);

    return retval;
}

static GSourceFuncs message_queue_source_funcs ={
        message_queue_source_prepare,
        NULL,
        message_queue_source_dispatch,
        message_queue_source_finalize,
        (GSourceFunc)message_queue_source_closure_callback,
        NULL,
};

GSource *message_queue_source_new(GAsyncQueue *queue,
        GDestroyNotify destroy_message,
        GCancellable *cancellable){

    GSource *source;
    MessageQueueSource *message_queue_source;
    g_return_val_if_fail(queue != NULL, NULL);
    g_return_val_if_fail(cancellable == NULL || G_IS_CANCELLABLE(cancellable), NULL);

    source = g_source_new(&message_queue_source_funcs, sizeof(MessageQueueSource));
    message_queue_source = (MessageQueueSource *) source;

    g_source_set_name(source, "MessageQueueSource");

    message_queue_source->queue = g_async_queue_ref(queue);
    message_queue_source->destroy_message = destroy_message;

    if(cancellable != NULL){
        GSource *cancellable_source;

        cancellable_source = g_cancellable_source_new(cancellable);
        g_source_set_dummy_callback(cancellable_source);
        g_source_add_child_source(source, cancellable_source);
        g_source_unref(cancellable_source);
    }

    return source;
}


static GMainContext *mainContext = NULL;

static gpointer consumer(gpointer data){
    GAsyncQueue *queue = data;
    GSource * source = message_queue_source_new(queue, g_free, NULL);
    g_source_set_callback(source, handle_message, "你好!", NULL);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    mainContext = g_main_context_new();
    GMainLoop *loop = g_main_loop_new(mainContext, FALSE);
    g_source_attach(source, mainContext);
    g_source_unref(source);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_main_context_unref(mainContext);
    g_print("out of thread\n");
    return NULL;
}

int main(){

    GAsyncQueue *queue = g_async_queue_new();
    GThread *thread = g_thread_try_new("consumer", consumer, queue, NULL);

    // product message
    int count = 10;
    for(; count>0; --count){
        g_usleep(G_USEC_PER_SEC);
        if(mainContext == NULL){
            exit(1);
        }
        g_print("push message hello\n");
        g_async_queue_push(queue, "hello");
//        g_main_context_wakeup(mainContext);
    }
    g_print("push message bye\n");
    g_async_queue_push(queue, "bye");

    g_thread_join(thread);

    return 0;
}



