# Redis SWR (Stale-While-Revalidate)

A Redis module that adds native stale-while-revalidate caching support.

## Motivation

Redis provides TTL-based expiration, but many production systems need:

- Soft expiry
- Hard expiry
- Serve stale data during refresh
- Prevent cache stampede

Redis SWR adds these capabilities.

## Current Features

- [x] SETSWR command
- [x] Soft expiry metadata
- [x] Hard expiry metadata
- [x] GETSWR command
- [ ] Background refresh
- [ ] Stampede prevention
- [ ] Refresh events
- [ ] Redis Cluster support

## Commands

### SETSWR

