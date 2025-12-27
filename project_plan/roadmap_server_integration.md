# NAS Integration Roadmap (Plant Journal)

This document outlines a safe, simple path to store and sync the Plant Journal database on a home NAS and access it from the home network or remotely.

## 0) Goals and Constraints
- Local-first app should keep working offline.
- Data should be centralized on NAS or reliably synced to it.
- Remote access must be secure (no public SQLite/SMB exposure).
- Avoid data corruption and conflicts.

## 1) Choose a Storage/Sync Model (Decision Point)

### Option A (Simplest & safest for SQLite): Local DB + NAS Sync
- Each device keeps a local SQLite file.
- Sync the DB to NAS with a file sync tool.
- Rules: only one device edits at a time, or use a sync tool with file locking.
- Pros: no server work, offline-friendly.
- Cons: conflict risk if multiple writers.

### Option B (Central DB on NAS via Network Share)
- Store SQLite on SMB/NFS share and open DB over the network.
- Pros: one central DB.
- Cons: SQLite is not ideal for multi-client access over network shares; file locking can be unreliable and may corrupt the DB.
- Not recommended for multiple writers.

### Option C (Recommended for multi-device writes): NAS hosts a DB service
- Run a small service on NAS or a home server (Postgres or a local API) and have the app talk to it over VPN.
- Pros: safe multi-device access, centralized logic.
- Cons: more setup and maintenance.

## 2) Home NAS Setup (Baseline)
- OS: Synology DSM, TrueNAS SCALE/CORE, or a small Linux box.
- Storage: RAID or ZFS; enable snapshots.
- UPS: if possible to avoid corruption on power loss.
- Users: create a dedicated user for the app data, no admin rights.
- Shared folder: restricted permissions (read/write only for the app user).

## 3) Network & Remote Access (Security First)
- Preferred: VPN-only access (WireGuard or Tailscale).
- Avoid exposing SMB, NFS, or database ports to the internet.
- Router:
  - If WireGuard: forward only the WireGuard UDP port.
  - If Tailscale: no port forwarding required (NAT traversal).
- DNS: use split DNS or MagicDNS (Tailscale) for easy hostnames.
- Enforce MFA on NAS and VPN accounts.

## 4) Sync Strategy (if Option A)
- Tool choices: Syncthing, rsync over SSH, or NAS native sync tools.
- Rules:
  - Do not run the app on two devices at the same time against the same DB file.
  - Sync on app start/exit or on a schedule.
- Backup:
  - Keep versioned copies or snapshots in case of conflicts or corruption.

## 5) Centralized Service (if Option C)
- Run a small server process on NAS or a home server.
- Use Postgres or a lightweight API that mediates access to SQLite.
- App connects over VPN only (no public exposure).
- Add authentication tokens for the app.
- Backups: database dumps + NAS snapshots.

## 6) Security Checklist
- NAS and router firmware up to date.
- Disable default admin account; use unique strong passwords.
- Enable firewall on NAS; restrict to VPN subnet.
- SSH: key-based auth only; disable password login if possible.
- No database ports exposed to WAN.
- Encrypt backups; keep an offsite backup if possible.

## 7) Suggested Step-by-Step Implementation

### Phase 1: Safe MVP Sync (Option A)
1) Create NAS shared folder for Plant Journal data.
2) Set up VPN (WireGuard or Tailscale).
3) Enable Syncthing (or rsync) between device and NAS.
4) Add a usage rule: only one device writes at a time.
5) Test recovery from a snapshot.

### Phase 2: Reliable Multi-Device Writes (Option C)
1) Spin up a small server on NAS (Docker or system service).
2) Move DB to Postgres or keep SQLite behind a single-writer API.
3) Update app to use the API over VPN.
4) Add auth tokens, rotate secrets.
5) Monitor logs and backups.

## 8) What the App Needs (Implementation Tasks)
- Add a configurable DB path or server endpoint.
- If using sync: add “sync on exit/launch” hooks.
- If using API: add auth and a simple client layer.

## 9) Notes
- SQLite on NAS via SMB/NFS is risky for concurrent access.
- VPN-only access keeps the attack surface minimal.
- For simplicity and safety, start with Option A, then move to Option C if needed.
