if(CONFIG_APP_HTTPS_CLIENT_ENABLED)
target_sources(app PRIVATE app_https_client.c)
endif()
