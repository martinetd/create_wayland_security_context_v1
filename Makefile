CFLAGS=-Wall -Wextra -Wno-unused-parameter -g
PKG_CONFIG ?= pkg-config

WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)

SECURITY_CONTEXT_PROTOCOL=$(WAYLAND_PROTOCOLS_DIR)/staging/security-context/security-context-v1.xml

HEADERS=security-context-client-protocol.h
SOURCES=main.c security-context-protocol.c

run_with_wl_security_context : $(HEADERS) $(SOURCES)
	clang $(CFLAGS) -o $@ $(SOURCES) $(WAYLAND_FLAGS)

security-context-client-protocol.h :
	wayland-scanner client-header $(SECURITY_CONTEXT_PROTOCOL) $@

security-context-protocol.c :
	wayland-scanner private-code $(SECURITY_CONTEXT_PROTOCOL) $@

.PHONY: clean
clean :
	rm -f run_with_wl_security_context security-context-protocol.c security-context-client-protocol.h
