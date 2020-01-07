#include <nice.h>
#include "../debug.h"
#include <stdio.h>
#include <gio/gnetworking.h>

void local_interface();

static const gchar *candidate_type_name[] = {"host", "srflx",
                                             "prflx", "relay"};
static const gchar *state_name[] = {"disconnected", "gathering",
                                    "connecting", "connected",
                                    "ready", "failed"};

static GMainLoop *gloop;
static GIOChannel *io_stdin;
static guint stream_id;

static gboolean print_local_data(NiceAgent *agent, guint _stream_id) {
    gboolean ret = FALSE;
    gchar *local_ufrag = NULL;
    gchar *local_password = NULL;
    gchar ip_addr[INET6_ADDRSTRLEN];
    GSList *candidates = NULL, *it = NULL;

    if (!nice_agent_get_local_credentials(agent, _stream_id,
                                          &local_ufrag, &local_password))
        goto end;

    g_print("%s %s", local_ufrag, local_password);

    candidates = nice_agent_get_local_candidates(agent, _stream_id, 1);
    if (candidates == NULL) goto end;

    for (it = candidates; it; it = it->next) {
        NiceCandidate *c = it->data;
        nice_address_to_string(&c->addr, ip_addr);

        printf(" %s,%u,%s,%u,%s",
               c->foundation,
               c->priority,
               ip_addr,
               nice_address_get_port(&c->addr),
               candidate_type_name[c->type]);
    }
    g_print("\n");
    ret = TRUE;

    end:
    if (local_ufrag) g_free(local_ufrag);
    if (local_password) g_free(local_password);
    if (candidates) g_slist_free_full(candidates, (GDestroyNotify) &nice_candidate_free);
    return ret;
}

static NiceCandidate *parse_candidate(char *candidate, guint _stream_id) {
    NiceCandidate *c = NULL;
    NiceCandidateType type = NICE_CANDIDATE_TYPE_HOST;
    gchar **tokens = NULL;
    gulong i;

    tokens = g_strsplit(candidate, ",", 5);
    for (i = 0; tokens[i]; ++i);
    if (i != 5) goto end;

    for (i = 0; i < G_N_ELEMENTS(candidate_type_name); ++i) {
        if (strcmp(tokens[4], candidate_type_name[i]) == 0) {
            type = i;
            break;
        }
    }

    if (i == G_N_ELEMENTS(candidate_type_name)) goto end;

    c = nice_candidate_new(type);
    c->component_id = 1;
    c->stream_id = _stream_id;
    c->transport = NICE_CANDIDATE_TRANSPORT_UDP;
    strncpy(c->foundation, tokens[0], NICE_CANDIDATE_MAX_FOUNDATION - 1);
    c->foundation[NICE_CANDIDATE_MAX_FOUNDATION - 1] = 0;
    c->priority = atoi(tokens[1]);

    if (!nice_address_set_from_string(&c->addr, tokens[2])) {
        JANUS_LOG(LOG_ERR, "failed to parse address: %s", tokens[2]);
        nice_candidate_free(c);
        goto end;
    }

    nice_address_set_port(&c->addr, atoi(tokens[3]));

    end:
    g_strfreev(tokens);
    return c;
}

static gboolean parse_remote_data(NiceAgent *agent, guint _stream_id,
                                  guint _component_id, char *line) {
    GSList *remote_candidates = NULL;
    gchar **line_argv = NULL;
    const gchar *ufrag = NULL;
    const gchar *password = NULL;
    int result = FALSE;
    int i;

    line_argv = g_strsplit_set(line, " \t\n", 0);
    for (i = 0; line_argv && line_argv[i]; ++i) {
        if (strlen(line_argv[i]) == 0)
            continue;

        if (!ufrag) ufrag = line_argv[i];
        else if (!password) password = line_argv[i];
        else {
            NiceCandidate *c = parse_candidate(line_argv[i], _stream_id);
            if (c == NULL) {
                JANUS_LOG(LOG_ERR, "failed to parse candidate : %s\n", line_argv[i]);
                goto end;
            }
            remote_candidates = g_slist_append(remote_candidates, c);
        }
    }

    if (ufrag == NULL || password == NULL || remote_candidates == NULL) {
        JANUS_LOG(LOG_ERR, "lost message ...\n");
        goto end;
    }

    if (!nice_agent_set_remote_credentials(agent, _stream_id, ufrag, password)) {
        JANUS_LOG(LOG_ERR, "failed to set remote credentials...\n");
        goto end;
    }

    if (nice_agent_set_remote_candidates(agent, _stream_id, _component_id, remote_candidates) < 1) {
//    if (nice_agent_set_remote_candidates(agent, _stream_id, _component_id, NULL) < 0) {
        JANUS_LOG(LOG_ERR, "failed to set remote candidates...\n");
        goto end;
    } else {
        JANUS_LOG(LOG_INFO, "set remote candidates... \n");
    }

    result = TRUE;

    end:
    if (!line_argv) g_strfreev(line_argv);
    if (!remote_candidates)
        g_slist_free_full(remote_candidates, (GDestroyNotify) &nice_candidate_free);

    return result;
}

static gboolean stdin_remote_info_cb(GIOChannel *source, GIOCondition cond,
                                     gpointer user_data) {
    NiceAgent *agent = user_data;
    gchar *line = NULL;
    int rval;
    gboolean ret = G_SOURCE_CONTINUE;

    if (g_io_channel_read_line(source, &line, NULL, NULL, NULL)
        == G_IO_STATUS_NORMAL) {
        rval = parse_remote_data(agent, stream_id, 1, line);
        if (rval) {
            ret = G_SOURCE_REMOVE;
            JANUS_LOG(LOG_INFO, "waiting for state READY or FAILED signal...\n");
        } else {
            JANUS_LOG(LOG_ERR, "failed to parse remote candidates\n");
            JANUS_LOG(LOG_WARN, "Enter remote data...\n");
            g_print("> ");
            fflush(stdout);
        }
        g_free(line);
    }
    return ret;
}

static void cb_candidate_gathering_done(NiceAgent *agent, guint _stream_id, gpointer user_data) {

    JANUS_LOG(LOG_INFO, "SIGNAL candidates gathering done.\n");
    JANUS_LOG(LOG_WARN, "Copy this line to remote client:\n");
    print_local_data(agent, _stream_id);
    g_print("\n");


    JANUS_LOG(LOG_WARN, "Enter remote data : \n");
    // Listen on stdin for the remote candidate list
    g_io_add_watch(io_stdin, G_IO_IN, stdin_remote_info_cb, agent);
    g_print("> ");
    fflush(stdout);
}

static void cb_new_selected_pair(NiceAgent *agent, guint _stream_id,
                                 guint component_id, gchar *lfoundation,
                                 gchar *rfoundation, gpointer user_data) {

}

static gboolean stdin_send_data_cb(GIOChannel *source, GIOCondition cond, gpointer user_data) {
    NiceAgent *agent = user_data;
    gchar *line = NULL;

    if (g_io_channel_read_line(source, &line, NULL, NULL, NULL)
        == G_IO_STATUS_NORMAL) {
        nice_agent_send(agent, stream_id, 1, strlen(line), line);
        g_free(line);
        line = NULL;
        printf("> ");
        fflush(stdout);
    } else {
        nice_agent_send(agent, stream_id, 1, 1, "\0");
        // Ctrl-D was pressed
        g_main_loop_quit(gloop);
    }
    return TRUE;
}

static void cb_component_state_changed(NiceAgent *agent, guint _stream_id,
                                       guint component_id, guint state,
                                       gpointer user_data) {
    JANUS_LOG(LOG_INFO, "SIGNAL: state changed %d %d %s[%d]\n",
              _stream_id, component_id, state_name[state], state);

    if (state == NICE_COMPONENT_STATE_CONNECTED) {
        NiceCandidate *local, *remote;

        // Get current selected candidate pair and print IP address used
        if (nice_agent_get_selected_pair(agent, _stream_id, component_id, &local, &remote)) {
            gchar ip_addr[INET6_ADDRSTRLEN];
            nice_address_to_string(&local->addr, ip_addr);
            printf("\n Negotiation complete:(%s:%d, ",
                   ip_addr, nice_address_get_port(&local->addr));

            nice_address_to_string(&remote->addr, ip_addr);
            printf("%s:%d)\n", ip_addr, nice_address_get_port(&remote->addr));
        }

        JANUS_LOG(LOG_INFO, "Send lines to remote (Ctrl-D to quit):\n");
        g_io_add_watch(io_stdin, G_IO_IN, stdin_send_data_cb, agent);
        g_print("> ");
        fflush(stdout);
    } else if (state == NICE_COMPONENT_STATE_FAILED) {
        g_main_loop_quit(gloop);
    }
}

static void cb_nice_recv(NiceAgent *agent, guint _stream_id, guint _component_id,
                         guint len, gchar *buf, gpointer user_data) {
    if (len == 1 && buf[0] == '\0')
        g_main_loop_quit(gloop);

    printf("%.*s", len, buf);
    fflush(stdout);
}

int main(int argc, char **argv) {

    // local_interface();

    NiceAgent *agent;
    gboolean controlling;
    char app[64];

    // get app name
    gchar **s = g_strsplit(argv[0], "/", -1);
    int len = g_strv_length(s);
    g_strlcpy(app, s[len - 1], strlen(s[len - 1]) + 1);
    g_strfreev(s);

    if (argc < 2) {
        JANUS_LOG(LOG_ERR, "Usage : %s [0|1]\n", app);
        return 0;
    }

    controlling = argv[1][0] - '0';
    if (controlling != 0 && controlling != 1) {
        JANUS_LOG(LOG_ERR, "Usage : %s [0|1]\n", app);
        return 0;
    }

    g_networking_init();
    gloop = g_main_loop_new(NULL, FALSE);
    io_stdin = g_io_channel_unix_new(fileno(stdin));
//    agent = nice_agent_new(g_main_loop_get_context(gloop), NICE_COMPATIBILITY_RFC5245);
//    agent = nice_agent_new_full(g_main_loop_get_context(gloop), NICE_COMPATIBILITY_RFC5245,
//                                NICE_AGENT_OPTION_LITE_MODE);
    if (!agent) {
        JANUS_LOG(LOG_ERR, "Failed to create agent\n");
        exit(0);
    }

    g_object_set(agent, "stun-server", "118.178.236.183", NULL);
    g_object_set(agent, "stun-server-port", 3478, NULL);

    g_object_set(agent, "controlling-mode", controlling, NULL);
    g_signal_connect(agent, "candidate-gathering-done",
                     G_CALLBACK(cb_candidate_gathering_done), NULL);
    g_signal_connect(agent, "new-selected-pair",
                     G_CALLBACK(cb_new_selected_pair), NULL);
    g_signal_connect(agent, "component-state-changed",
                     G_CALLBACK(cb_component_state_changed), NULL);

    stream_id = nice_agent_add_stream(agent, 1);
    JANUS_LOG(LOG_INFO, "stream id : %d\n", stream_id);
    if (stream_id == 0) {
        JANUS_LOG(LOG_ERR, "failed to add stream\n");
        exit(0);
    }

    // attach to the component to receive the data
    // Without this call, candidates cannot be gatered.
    nice_agent_attach_recv(agent, stream_id, 1, g_main_loop_get_context(gloop),
                           cb_nice_recv, NULL);

    // start gatering local candidate
    if (!nice_agent_gather_candidates(agent, stream_id)) {
        JANUS_LOG(LOG_ERR, "failed to gather candidates\n");
    }

    // JANUS_LOG(LOG_INFO, "waiting for candidate-gathering-done signal...\n");

    // Run the mainloop. Everything else will happen asynchronously
    // when the candidates are done gathering
    g_main_loop_run(gloop);
    g_main_loop_unref(gloop);
    g_object_unref(agent);

    g_io_channel_unref(io_stdin);

    return EXIT_SUCCESS;

}

void local_interface() {
    GList *iterator;
    GList *interfaces = nice_interfaces_get_local_interfaces();
    g_print("size of interfaces : %d\n", g_list_length(interfaces));
    for (iterator = interfaces; iterator; iterator = iterator->next) {
        gchar *ip = nice_interfaces_get_ip_for_interface(iterator->data);
        g_print("interface name : %s, ip : %s\n", iterator->data, ip);
        g_free(ip);
    }
    g_list_free(interfaces);

//    GList *list = nice_interfaces_get_local_ips(TRUE);
    GList *list = nice_interfaces_get_local_ips(FALSE);
    for (iterator = list; iterator; iterator = iterator->next) {
        g_print("element : %s\n", iterator->data);
    }
    g_list_free(list);
}

