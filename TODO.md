# TODO

## LAN Game Hosting

- Add a LAN server browser screen that sends Quake 2-style broadcast `info`
  probes, parses server replies, and shows host name, map, player count, speed,
  and address.
- Wire selecting a discovered LAN server to `CL_Connect(host, game_port)`,
  including refresh/rescan and stale-server expiry.
- Consume the remaining lobby cvars server-side:
  - `sv_slot*_color`
  - `sv_slot*_name`
  - `sv_map_name`
- Decide how open slots map to remote clients after the host starts the server:
  current code applies host-selected map player type/race/team before spawn,
  but remote clients still need deterministic slot assignment.
- Push game speed beyond LAN advertisement into simulation/JASS-visible game
  state when the game-speed model is implemented.
- Add an end-to-end LAN discovery test with a real broadcast path if CI can
  support it; current coverage verifies direct UDP `info` query/reply.
