# create_wl_security_context

(was run_with_wayland_security_context_v1, thanks to https://github.com/FelixPehla/run_with_wayland_security_context_v1 )

Create a new wayland socket with security protocols restricted, replaces $WAYLAND_SOCKET with it (the original socket is left at "$WAYLAND_SOCKET-priv"), and forks in the background.

Usage (example): `./create_wl_security_context`

Dependencies:
 - `make` (build)
 - `pkgconf` (build)
 - `wayland-protocols` (build)
 - `wayland-scanner` (build)


Example wrapper to run programs that require privileged protocols:
```
$ cd .bin # or whatever is in your PATH
$ cat > wayland-run-priv <<'EOF'
#!/bin/bash

prog=${0##*/}
dir=${0%/*}
# avoid loop
export PATH=${PATH//$dir/\/enoent}

[ -e "$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY-priv" ] && export WAYLAND_DISPLAY="$WAYLAND_DISPLAY-priv"
exec "$prog" "$@"
EOF
$ chmod +x wayland-run-priv
$ ln -s wayland-run-priv swaylock
$ ln -s wayland-run-priv mako
$ ln -s wayland-run-priv waybar
$ ln -s wayland-run-priv fuzzel
$ ln -s wayland-run-priv fcitx5
```

Caveats:
 - The wayland socket bound to a security context replaced `$WAYLAND_DISPLAY` and will be used by default, but the original wayland socket is still available at (e.g. `${XDG_RUNTIME_DIR}/wayland-[0-9]-priv`) - in a sandbox you would want to prevent its use through namespaces or other means.
- If using systemd user services for e.g. mako or waybar you will need to ajust the systemd service (e.g. replace ExecStart with `systemctl --user edit ...`
