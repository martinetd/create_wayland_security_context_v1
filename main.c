#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "security-context-client-protocol.h"

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                                   const char *interface, uint32_t version)
{
	if (strcmp(wp_security_context_manager_v1_interface.name, interface) == 0) {
		*((struct wp_security_context_manager_v1 **) data) = wl_registry_bind(
			registry, name, &wp_security_context_manager_v1_interface, 1);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	// Don't bother
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

int create_wl_socket(char *socket_path)
{
	char *socket_dir = getenv("XDG_RUNTIME_DIR");
	char *socket_prefix = getenv("WAYLAND_DISPLAY");
	if (!socket_dir || !socket_prefix) {
		fprintf(stderr, "Failed to obtain runtime directory path or display\n");
		return -1;
	}

	// Check if a wayland socket could be created in the runtime directory
	snprintf(socket_path, 256, "%s/%s-priv", socket_dir, socket_prefix);

	struct sockaddr_un sockaddr = {
		.sun_family = AF_UNIX,
	};
	snprintf(sockaddr.sun_path, sizeof(sockaddr.sun_path), "%s", socket_path);

	int test_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connect(test_fd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) == 0) {
		fprintf(stderr, "Already another socket at %s\n", socket_path);
		close(test_fd);
		return -1;
	}
	unlink(socket_path);
	close(test_fd);

	int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		fprintf(stderr, "Failed to create socket\n");
		perror("");
		return -1;
	}

	int flags = fcntl(listen_fd, F_GETFD);
	fcntl(listen_fd, F_SETFD, flags | FD_CLOEXEC);

	if (bind(listen_fd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) != 0) {
		fprintf(stderr, "Failed to bind to socket at %s\n", socket_path);
		close(listen_fd);
		return -1;
	}

	if (listen(listen_fd, 0) != 0) {
		fprintf(stderr, "Failed to listen on socket at %s\n", socket_path);
		close(listen_fd);
		return -1;
	}
	return listen_fd;
}

int main(int argc, char *argv[])
{
	int exit_code = EXIT_FAILURE;

	struct wl_display *display;
	struct wl_registry *registry;
	struct wp_security_context_manager_v1 *security_context_manager = NULL;
	struct wp_security_context_v1 *security_context;

	int listen_fd = -1, ret;
	char socket_path[256];

	display = wl_display_connect(NULL);
	if (!display) {
		fprintf(stderr, "Failed to connect to wayland display\n");
		return EXIT_FAILURE;
	}

	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, &security_context_manager);

	ret = wl_display_roundtrip(display);
	wl_registry_destroy(registry);
	if (ret == -1) {
		fprintf(stderr, "Failed to obtain globals\n");
		goto exit;
	}
	if (!security_context_manager) {
		fprintf(stderr, "Failed to bind security_context_manager\n");
		goto exit;
	}

	listen_fd = create_wl_socket(socket_path);
	if (listen_fd == -1)
		goto exit;

	int fds[2];
	if (pipe2(fds, O_CLOEXEC) < 0)
		goto exit;

	security_context = wp_security_context_manager_v1_create_listener(security_context_manager,
	                                                                  listen_fd, fds[1]);
	// wp_security_context_v1_set_sandbox_engine(security_context, "");
	// wp_security_context_v1_set_app_id(security_context, "");
	// wp_security_context_v1_set_instance_id(security_context, "");
	wp_security_context_v1_commit(security_context);
	wp_security_context_v1_destroy(security_context);
	wl_display_roundtrip(display);

	printf("export WAYLAND_DISPLAY=%s\n", basename(socket_path));
	while (access(socket_path, F_OK) == 0)
		sleep(300);

	close(fds[0]);

	exit_code = EXIT_SUCCESS;
exit:
	if (listen_fd >= 0)
		close(listen_fd);
	if (security_context_manager)
		wp_security_context_manager_v1_destroy(security_context_manager);
	wl_display_disconnect(display);
	return exit_code;
}
