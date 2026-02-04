if(CONFIG_APP_MQTT_CLIENT_ENABLED)
target_sources(app PRIVATE app_mqtt_client.c)
endif()
