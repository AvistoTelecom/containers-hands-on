# Exercice E

## Introduction

This exercise introduces **Docker Compose** by deploying a real distributed application: the [example-voting-app](https://github.com/dockersamples/example-voting-app). You will write a `compose.yaml` from scratch, adding services incrementally and using `docker compose logs` to understand and fix problems at each step.

No builds in this exercise — all images are pulled from Docker Hub.

---

## Architecture

![Architecture](https://raw.githubusercontent.com/dockersamples/example-voting-app/main/architecture.excalidraw.png)

| Service | Image | Role |
|---|---|---|
| `vote` | `dockersamples/examplevotingapp_vote` | Python web app — cast a vote between two options |
| `redis` | `redis:alpine` | Collects incoming votes (message queue) |
| `worker` | `dockersamples/examplevotingapp_worker` | .NET service — reads votes from Redis and stores them in the database |
| `db` | `postgres:15-alpine` | PostgreSQL — persistent vote storage |
| `result` | `dockersamples/examplevotingapp_result` | Node.js web app — displays live results |

### Networks

| Network | Connected services | Purpose |
|---|---|---|
| `vote-tier` | `vote`, `redis` | Vote app → Redis message queue |
| `back-tier` | `redis`, `worker`, `db` | Worker bridges Redis and PostgreSQL |
| `result-tier` | `db`, `result` | Result app reads PostgreSQL |

Each service is on only the network(s) it actually needs: `vote` cannot reach `db`, `result` cannot reach `redis`, and `redis` cannot talk directly to `db`.

---

## Objectives

Write a `compose.yaml` that deploys the full voting application in three incremental steps. At each step you will start the stack, check what works, inspect the logs, and add what is missing.

## Project structure

```
exercice-e/
├── compose.yaml              ← you will create this
└── compose.yaml-correction   ← reference solution
```

---

## Reference

- [Compose file reference](https://docs.docker.com/reference/compose-file/)

---

## Useful commands

```bash
docker compose up -d                  # start all services in the background
docker compose down                   # stop and remove containers and networks
docker compose logs <service>         # print logs for a service
docker compose logs -f <service>      # follow logs in real time
docker compose ps                     # list running services
```

---

## Instructions

### Step 1 — Vote and Redis

Create a `compose.yaml` with two services: `redis` and `vote`.

**`redis`**
- Image: `redis:alpine`
- Networks: `vote-tier` and `back-tier`

**`vote`**
- Image: `dockersamples/examplevotingapp_vote`
- Port: `8080:80`
- Network: `vote-tier`
- Depends on: `redis`

> The vote app resolves Redis by the service name. Name the Redis service exactly `redis`.

Start the stack and open http://localhost:8080. You should see the voting interface and be able to cast a vote.

Run the following and observe the output:

```bash
docker compose logs vote
docker compose logs redis
```

The vote is accepted but nothing processes it yet — there is no worker or database. That comes in the next step.

---

### Step 2 — Database and Worker

Add two more services to your `compose.yaml`: `db` and `worker`.

**`db`**
- Image: `postgres:15-alpine`
- Networks: `back-tier` and `result-tier`
- Volume: persist `/var/lib/postgresql/data` using a named volume `db-data`
- The PostgreSQL image requires two environment variables to initialise — look at what is missing in the logs

**`worker`**
- Image: `dockersamples/examplevotingapp_worker`
- Network: `back-tier`
- Depends on: `redis` and `db`

Declare the `db-data` volume at the top level under `volumes:`.

Start the updated stack:

```bash
docker compose up -d
docker compose logs -f worker
```

Watch the worker output. If the database is not configured correctly the worker will log connection errors — read them carefully, they tell you exactly what environment variable is missing.

> The worker expects PostgreSQL at hostname `db` with user `postgres` and password `postgres`. The `db` service must expose those credentials via environment variables.

Once the worker connects successfully, cast a few votes and confirm they are being processed in the logs.

---

### Step 2b — Add health checks and readiness conditions

`depends_on` controls **startup order** but not readiness: the worker may start before Redis or PostgreSQL are actually ready to accept connections, causing transient errors. Fix this by adding a `healthcheck` to each infrastructure service and upgrading `depends_on` to wait for `service_healthy`.

Add a health check to **`redis`**:

```yaml
healthcheck:
  test: ["CMD", "redis-cli", "ping"]
  interval: 5s
  timeout: 3s
  retries: 5
```

Add a health check to **`db`**:

```yaml
healthcheck:
  test: ["CMD-SHELL", "pg_isready -U postgres"]
  interval: 5s
  timeout: 3s
  retries: 5
```

Update every `depends_on` that references `redis` or `db` to use the long-form syntax:

```yaml
depends_on:
  redis:
    condition: service_healthy
  db:
    condition: service_healthy
```

Apply this to `vote` (depends on `redis`), `worker` (depends on `redis` and `db`), and `result` (depends on `db`).

Restart the stack and watch the logs: services now wait for their dependencies to pass the health check before starting.

```bash
docker compose up -d
docker compose ps   # HEALTH column shows starting → healthy
```

---

### Step 3 — Result app

Add the final service: `result`.

**`result`**
- Image: `dockersamples/examplevotingapp_result`
- Port: `8081:80`
- Network: `result-tier`
- Depends on: `db` with `condition: service_healthy`

Restart the stack and open http://localhost:8081. You should see the live vote results update as you vote at http://localhost:8080.

```bash
docker compose logs -f result
```

> If the result app cannot reach the database, the page will stay empty. The service name `db` must match exactly.

---

### Step 4 — Inspect the full stack

Once everything is running, explore the state of your deployment:

```bash
docker compose ps
docker network ls
docker volume ls
```

Stop the stack, restart it, and verify the vote counts are preserved — the named volume keeps the database alive across restarts.

```bash
docker compose down
docker compose up -d
```

Open http://localhost:8081 and confirm your previous votes are still there.

---

## Cleanup

`docker compose down` removes containers and networks but leaves the named volume intact. Pass `-v` to also delete it.

```bash
docker compose down -v
```

To also remove the pulled images:

```bash
docker rmi dockersamples/examplevotingapp_vote \
           dockersamples/examplevotingapp_result \
           dockersamples/examplevotingapp_worker \
           postgres:15-alpine \
           redis:alpine
```

---

> **Key takeaway**: Compose turns a multi-container architecture into a single declarative file. Service names become DNS hostnames on shared networks, `depends_on` controls startup order, and named volumes outlive the containers that use them. Multiple networks enforce the principle of least privilege: each service can only reach the services it genuinely needs.
