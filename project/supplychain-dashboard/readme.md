# Digital Product Passport Dashboard

This repository contains the Django-based dashboard for the Digital Product Passport PoC.
You can run it **locally** using the [`uv`](https://pypi.org/project/uv/) Python package manager, or spin up a **development environment** via Docker Compose.

---

## Table of Contents

- [Prerequisites](#prerequisites)  
- [Environment Variables](#environment-variables)  
- [Local Setup (uv)](#local-setup-uv)  
- [Database Setup](#database-setup)  
- [Running Locally](#running-locally)  
- [Docker Compose Setup](#docker-compose-setup)  
- [Running in Docker](#running-in-docker)  
- [Running Tests](#running-tests)  
- [License](#license)  

---

## Prerequisites

- **Python 3.11** installed on your host  
- **uv** Python package manager (`pip install --user uv`)  
- **Docker** (Engine ≥ 20.10) & **Docker Compose** (v2)  

---

## Environment Variables

Create a file named `.env` in the project root (you can copy `.env.example`):

```dotenv
# .env
DEBUG=True
SECRET_KEY=changeme-in-production
DATABASE_URL=postgres://django:django@db:5432/dpp_db
ALLOWED_HOSTS=localhost,127.0.0.1
```

| Name            | Description                                         |
|-----------------|-----------------------------------------------------|
| `DEBUG`         | `True` or `False`                                   |
| `SECRET_KEY`    | Django secret key                                   |
| `DATABASE_URL`  | Postgres URL (see [12-factor](https://12factor.net/)) |
| `ALLOWED_HOSTS` | Comma-separated list of allowed hosts               |

---

## Local Setup (uv)

1. **Install uv** (if you haven’t already):

   ```bash
   python3.11 -m pip install --user uv
   ```

2. **Create a virtual environment** in `.venv`:

   ```bash
   uv venv --python 3.11 .venv
   ```

3. **Activate** the venv:

   ```bash
   # macOS / Linux
   source .venv/bin/activate

   # Windows (PowerShell)
   .venv\Scripts\Activate.ps1
   ```

4. **Install dependencies**:

   ```bash
   uv pip install -r requirements.txt
   ```

---

## Database Setup

> **Note:** For local dev, you can run Postgres on your host or use Docker.

### Option A: Host-installed Postgres

1. Create database & user:

   ```bash
   psql postgres
   CREATE USER django WITH PASSWORD 'django';
   CREATE DATABASE dpp_db OWNER django;
   \q
   ```

2. Ensure your `DATABASE_URL` in `.env` matches.

### Option B: Dockerized Postgres

Skip to [Docker Compose Setup](#docker-compose-setup) and use the `db` service.

---

## Running Locally

1. **Apply migrations**:

   ```bash
   uv run manage.py migrate
   ```

2. **Create a superuser**:

   ```bash
   uv run manage.py createsuperuser
   ```

3. **Start the development server**:

   ```bash
   uv run manage.py runserver 0.0.0.0:8000
   ```

4. Open http://localhost:8000 in your browser.

---

## Docker Compose Setup

A `docker-compose.yml` is provided for an isolated dev environment:

Make sure your `.env`’s `DATABASE_URL` is:

```text
postgres://django:django@db:5432/dpp_db
```

---

## Running in Docker

1. **Build & start** all services:

   ```bash
   docker-compose up -d --build
   ```

2. **Apply migrations** inside the `web` container:

   ```bash
   docker-compose exec web uv run manage.py migrate
   ```

3. **Create a superuser**:

   ```bash
   docker-compose exec web uv run manage.py createsuperuser
   ```

4. **Visit** http://localhost:8000 to see the running app.

---

## Running Tests

- **Locally (uv)**:

  ```bash
  uv run manage.py test
  ```

- **In Docker**:

  ```bash
  docker-compose exec web uv run manage.py test
  ```

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
