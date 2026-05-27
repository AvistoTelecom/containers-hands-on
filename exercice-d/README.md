# Exercice D

## Introduction

Containers are ephemeral: when a container is removed, its filesystem is gone with it. They also run in isolated networks by default, with no way to reach each other by name. This exercise introduces **named volumes** for data persistence and **user-defined bridge networks** for service-to-service communication.

---

## Part 1 — Volumes

### 1. Observe data loss

Write a file inside a container, remove it, then start a fresh container from the same image.

```bash
docker run --name writer alpine sh -c "echo 'important data' > /data/message.txt && cat /data/message.txt"
docker rm writer
docker run --name writer alpine sh -c "cat /data/message.txt"
```

The last command fails — the file does not exist. Each container starts from a clean slate; anything written to its own filesystem is lost when it is removed.

```bash
docker rm writer
```

### 2. Persist data with a named volume

Run the same container with a **named volume** mounted at `/data`. Docker creates the volume automatically if it does not exist yet.

```bash
docker run --name writer -v mydata:/data alpine sh -c "echo 'important data' > /data/message.txt"
docker rm writer
```

Now run a completely new container mounting the same volume and read the file.

```bash
docker run --name reader -v mydata:/data alpine sh -c "cat /data/message.txt"
docker rm reader
```

The data survived. It lives in the volume, which is independent of any container's lifetime.

### 3. Inspect the volume

List all volumes on the host, then look at the details of the one you just created.

```bash
docker volume ls
docker volume inspect mydata
```

The `Mountpoint` field shows where Docker stores the volume data on the host filesystem.

### 4. Delete the volume and verify data is gone

```bash
docker volume rm mydata
docker run --rm -v mydata:/data alpine sh -c "cat /data/message.txt"
```

Docker recreates the volume empty — the file is gone. Deleting a volume permanently destroys its contents.

---

## Part 2 — Networking

### 5. Observe the default network limitation

Start a long-running container named `server`, then try to reach it by name from a second container.

```bash
docker run -d --name server alpine sleep 3600
docker run --rm alpine ping -c 2 server
```

The ping fails. On the **default bridge network**, Docker does not provide DNS resolution by container name — containers can only reach each other by IP address.

### 6. Create a user-defined network

```bash
docker network create mynet
docker network ls
```

On a user-defined bridge network, Docker's embedded DNS automatically resolves container names to their IP addresses, making service discovery trivial.

### 7. Connect containers on the same network

Remove the previous server and start a new one attached to `mynet`, then ping it by name from a second container on the same network.

```bash
docker rm -f server
docker run -d --name server --network mynet alpine sleep 3600
docker run --rm --network mynet alpine ping -c 2 server
```

The ping resolves and succeeds. The container name `server` is used directly as a hostname.

### 8. Inspect the network

```bash
docker network inspect mynet
```

The `Containers` section lists every container currently attached to the network along with their assigned IP addresses.

### 9. Clean up

```bash
docker rm -f server
docker network rm mynet
```

---

## Part 3 — Port forwarding

By default, a container's ports are not reachable from the host. This section shows how to expose them.

### 10. Start nginx and reach it from inside the container

Start an nginx container without any port mapping.

```bash
docker run -d --name webserver nginx
```

A container can always reach its own services. Use `docker exec` to run `curl` inside the container itself:

```bash
docker exec webserver curl -s http://localhost
```

You should see the nginx welcome page HTML. The service is running, but it is only visible from inside the container.

### 11. Try reaching it from the host

Now try the same request from the host machine:

```bash
curl -s http://localhost
```

The connection is refused — nginx is listening inside the container's network namespace, which the host cannot access without an explicit mapping.

```bash
docker rm -f webserver
```

### 12. Expose the port with `-p`

Start a new nginx container, this time mapping port `8080` on the host to port `80` inside the container.

```bash
docker run -d --name webserver -p 8080:80 nginx
```

The format is `host_port:container_port`. Docker configures a `iptables` rule to forward traffic arriving on the host's port `8080` into the container.

Run `curl` from the host:

```bash
curl -s http://localhost:8080
```

The nginx welcome page is now reachable from outside the container.

### 13. Clean up

```bash
docker rm -f webserver
```

---

## Part 4 — Debugging

### 14. Inspect logs

Start nginx again and inspect its output.

```bash
docker run -d --name webserver -p 8080:80 nginx
docker logs webserver
```

Make a request to generate a log entry, then follow the logs in real time:

```bash
curl -s http://localhost:8080 > /dev/null
docker logs -f webserver
```

Press `Ctrl+C` to stop following. Containers that write to stdout/stderr are the ones you can observe this way.

### 15. Enter a running container

Open a shell inside the running container and explore its filesystem.

```bash
docker exec -it webserver sh
```

From inside the container, run a few commands:

```sh
cat /etc/os-release
ls /usr/share/nginx/html
exit
```

You are inside the container's filesystem and process namespace. Changes made here are lost when the container is removed.

### 16. Inspect container metadata

`docker inspect` dumps everything Docker knows about a container as JSON.

```bash
docker inspect webserver
```

Extract specific fields using `--format`:

```bash
docker inspect --format='{{.NetworkSettings.IPAddress}}' webserver
docker inspect --format='{{.State.Status}}' webserver
```

### 17. Monitor resource usage

```bash
docker stats --no-stream
```

This prints a one-shot snapshot of CPU, memory, and network I/O for all running containers. Omit `--no-stream` for a live updating view.

### 18. Clean up

```bash
docker rm -f webserver
```

---

## Cleanup

Remove any remaining containers, the volume, and the user-defined network. Steps 9 and 13 already handle teardown during the exercise — run this section to ensure everything is gone.

```bash
docker rm -f server webserver
docker volume rm mydata
docker network rm mynet
```

---

> **Key takeaway**: named volumes outlive containers — data lives in the volume, not in the container layer. User-defined bridge networks give containers automatic DNS resolution by name; the default bridge network does not. A container's ports are isolated by default — use `-p host:container` to forward traffic from the host into the container. Use `docker logs`, `docker exec`, `docker inspect`, and `docker stats` to observe and debug running containers.
