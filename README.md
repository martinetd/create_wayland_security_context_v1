# run_with_wayland_security_context_v1
Execute a program attached to a wayland security context

Usage (example): `./run_with_wl_security_context /usr/bin/wayland-info`

Dependencies:
 - `make` (build)
 - `pkgconf` (build)
 - `wayland-protocols` (build)
 - `wayland-scanner` (build)

Caveats:
 - The wayland socket bound to a security context is passed to children using `$WAYLAND_DISPLAY` but the original wayland socket (e.g. `${XDG_RUNTIME_DIR}/wayland-[0-9]`) can still be accessed - in a sandbox you would want to prevent its use through namespaces or other means.
 - `$WAYLAND_SOCKET` is unset for the child process.
