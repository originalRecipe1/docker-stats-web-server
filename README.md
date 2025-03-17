# Lightweight C++ Web Server for Docker Stats 🚀  
A minimal C++ web server (using `cpp-httplib` & `nlohmann/json`) that queries the **Docker daemon** to:

- **List running containers** (`GET /containers`)
- **Fetch stats for specific containers** (`GET /container_stats?ids=container_id1,container_id2`)

Runs in an **Alpine Linux Docker container**, supports **Unix sockets & exposed daemons on port 2375**, and reads the **port from `$PORT`** (default: `8080`).


## Instructions

```shell
    docker build -t docker_monitor:latest .
```

## On a Linux Host (Not Docker Desktop)

```shell
    docker run -it --rm \
      -p 8080:8080 \
      -v /var/run/docker.sock:/var/run/docker.sock \
      docker_monitor:latest
```

---

## Docker Desktop (Windows, Mac, Linux)

1. Expose Docker Daemon
2. ```shell
    docker run -it --rm -p 8080:8080 `
    -e DOCKER_ENDPOINT="host.docker.internal:2375" `
    docker_monitor:latest
    ```

## HTTP API

Once the container is running:

1. List containers:
    ```shell
    curl http://localhost:8080/containers
    ```

2. Container stats (provide comma-separated IDs):
    ```shell
    curl http://localhost:8080/container_stats?ids=<id1>,<id2>
    ```

It should return JSON results in the same structure as the official docker api.