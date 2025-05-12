import os

BASE_DIR = os.path.abspath(os.path.dirname(__file__))

class Config:
    """Base config."""
    VIEWER_IPADDR = os.environ.get("VIEWER_IPADDR", "192.168.4.1")
    VIEWER_PORT = os.environ.get("VIEWER_PORT", 5000)

    CLICKHOUSE_HOST = os.environ.get("CLICKHOUSE_HOST", "clickhouse")
    CLICKHOUSE_USER = os.environ.get("CLICKHOUSE_USER", "admin")
    CLICKHOUSE_PASSWORD = os.environ.get("CLICKHOUSE_PASSWORD", "admin")
    CLICKHOUSE_PORT = os.environ.get("CLICKHOUSE_PORT", 8123)
    CLICKHOUSE_TIMEOUT_S = 60

class DevelopmentConfig(Config):
    DEBUG = True

class TestingConfig(Config):
    TESTING = True

class ProductionConfig(Config):
    pass
