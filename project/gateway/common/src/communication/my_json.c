#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/uuid.h> 

#include <my_json.h>
#include <string.h>  // required for memcpy
#include <stdio.h>   // for snprintf
#include <ctype.h>
#include <time.h>

LOG_MODULE_REGISTER(json_module);

void generate_uuid(char *uuid_str, size_t len) {
    struct uuid uuid;
    int ret;

    ret = uuid_generate_v4(&uuid);
    if (ret != 0) {
        // Handle error (ret < 0)
        uuid_str[0] = '\0';
        return;
    }

    ret = uuid_to_string(&uuid, uuid_str);
    if (ret != 0) {
        // Handle error (ret < 0)
        uuid_str[0] = '\0';
        return;
    }
}

// void print_json_full_packet(const struct json_full_packet *packet)
extern void print_json_full_packet(const struct json_full_packet *packet) {
    printk("Header:\n");
    printk("  messageId: %s\n", packet->header.messageId);
    printk("  gatewayId: %s\n", packet->header.gatewayId);
    printk("  schemaVersion: %s\n", packet->header.schemaVersion);
    printk("  messageType: %s\n", packet->header.messageType);

    printk("Payload:\n");
    printk("  deviceId: %s\n", packet->payload.deviceId);
    printk("  time: %s\n", packet->payload.timestamp);
    printk("  uptime: %d\n", packet->payload.uptime);

    printk("  Location:\n");
    printk("    latitude: %s\n", packet->payload.location.latitude);
    printk("    ns: %s\n", packet->payload.location.ns);
    printk("    longitude: %s\n", packet->payload.location.longitude);
    printk("    ew: %s\n", packet->payload.location.ew);
    printk("    altitude_m: %s\n", packet->payload.location.altitude_m);

    printk("  Environment:\n");
    printk("    temperature_c: %s\n", packet->payload.environment.temperature_c);
    printk("    humidity_percent: %s\n", packet->payload.environment.humidity_percent);
    printk("    pressure_hpa: %s\n", packet->payload.environment.pressure_hpa);
    printk("    gas_ppm: %s\n", packet->payload.environment.gas_ppm);

    printk("  Acceleration:\n");
    printk("    x_mps2: %s\n", packet->payload.acceleration.x_mps2);
    printk("    y_mps2: %s\n", packet->payload.acceleration.y_mps2);
    printk("    z_mps2: %s\n", packet->payload.acceleration.z_mps2);

    printk("Signature:\n");
    printk("  alg: %s\n", packet->signature.alg);
    printk("  keyId: %s\n", packet->signature.keyId);
    printk("  value: %s\n", packet->signature.value);
}

extern void fill_json_packet_from_tracker_payload(const tracker_payload_t *payload, struct json_full_packet *packet) {
    // Format strings from payload
    snprintf(packet->payload.deviceId, sizeof(packet->payload.deviceId), "dev-%d", payload->dev_id);

    // //Decode unix timestamp
    // struct tm decoded;
    // timeutil_timegm(payload->timestamp, &decoded);

    // printk("Decoded time: %04d-%02d-%02dT%02d:%02d:%02d\n",
    //        decoded.tm_year + 1900, decoded.tm_mon + 1, decoded.tm_mday,
    //        decoded.tm_hour, decoded.tm_min, decoded.tm_sec);
    // }

    time_t raw_time = (time_t)payload->timestamp;
    struct tm timeinfo;

    // Convert Unix timestamp to broken-down UTC time (GMT)
    // Use gmtime_r for thread safety, or gmtime if not available
    if (gmtime_r(&raw_time, &timeinfo) == NULL) {
        printk("Failed to convert timestamp\n");
        return;
    }

    // // Print time in desired format: YYYY-MM-DDTHH:MM:SS
    // printk("Decoded time: %04d-%02d-%02dT%02d:%02d:%02d\n",
    //     timeinfo.tm_year + 1900,
    //     timeinfo.tm_mon + 1,
    //     timeinfo.tm_mday,
    //     timeinfo.tm_hour,
    //     timeinfo.tm_min,
    //     timeinfo.tm_sec);

    // Location
    snprintf(packet->payload.location.latitude, sizeof(packet->payload.location.latitude), "%.7f", payload->lat / 1e7);
    snprintf(packet->payload.location.longitude, sizeof(packet->payload.location.longitude), "%.7f", payload->lon / 1e7);
    snprintf(packet->payload.location.altitude_m, sizeof(packet->payload.location.altitude_m), "%.1f", payload->alt / 10.0);
    snprintf(packet->payload.location.ns, sizeof(packet->payload.location.ns), "%c", payload->ns);
    snprintf(packet->payload.location.ew, sizeof(packet->payload.location.ew), "%c", payload->ew);

    // Environment
    snprintf(packet->payload.environment.temperature_c, sizeof(packet->payload.environment.temperature_c), "%.2f", payload->temp / 100.0);
    snprintf(packet->payload.environment.humidity_percent, sizeof(packet->payload.environment.humidity_percent), "%.2f", payload->humid / 100.0);
    snprintf(packet->payload.environment.pressure_hpa, sizeof(packet->payload.environment.pressure_hpa), "%.1f", payload->press / 10.0);
    snprintf(packet->payload.environment.gas_ppm, sizeof(packet->payload.environment.gas_ppm), "%.2f", payload->gas / 100.0);

    // Acceleration
    snprintf(packet->payload.acceleration.x_mps2, sizeof(packet->payload.acceleration.x_mps2), "%.3f", payload->x / 1000.0);
    snprintf(packet->payload.acceleration.y_mps2, sizeof(packet->payload.acceleration.y_mps2), "%.3f", payload->y / 1000.0);
    snprintf(packet->payload.acceleration.z_mps2, sizeof(packet->payload.acceleration.z_mps2), "%.3f", payload->z / 1000.0);

    //time
    snprintf(packet->payload.timestamp, sizeof(packet->payload.timestamp), "%04d-%02d-%02dT%02d:%02d:%02d", timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec);
    packet->payload.uptime = payload->uptime;


    char uuid_str[UUID_STR_LEN];  // 37 bytes buffer including '\0'
    generate_uuid(uuid_str, sizeof(uuid_str));

    // Header (dummy values)
    strncpy(packet->header.messageId, uuid_str, sizeof(packet->header.messageId));
    strncpy(packet->header.gatewayId, "GW-01", sizeof(packet->header.gatewayId));
    strncpy(packet->header.schemaVersion, "1.0", sizeof(packet->header.schemaVersion));
    strncpy(packet->header.messageType, "telemetry", sizeof(packet->header.messageType));

    // Signature
    strncpy(packet->signature.alg, "HS256", sizeof(packet->signature.alg));
    strncpy(packet->signature.keyId, "key-001", sizeof(packet->signature.keyId));

    for (int i = 0; i < 32; i++) {
        snprintf(&packet->signature.value[i * 2], 3, "%02X", payload->hash[i]);
    }
    packet->signature.value[64] = '\0';
}

extern void encode_and_print_json(const struct json_full_packet *packet) {
    char json_output[JSON_BUFFER_SIZE];

    int ret = snprintf(json_output, sizeof(json_output),
        JSON_FORMAT,
        packet->header.messageId,
        packet->header.gatewayId,
        packet->header.schemaVersion,
        packet->header.messageType,
        packet->payload.deviceId,
        packet->payload.timestamp,
        packet->payload.uptime,
        packet->payload.location.latitude,
        packet->payload.location.ns,
        packet->payload.location.longitude,
        packet->payload.location.ew,
        packet->payload.location.altitude_m,
        packet->payload.environment.temperature_c,
        packet->payload.environment.humidity_percent,
        packet->payload.environment.pressure_hpa,
        packet->payload.environment.gas_ppm,
        packet->payload.acceleration.x_mps2,
        packet->payload.acceleration.y_mps2,
        packet->payload.acceleration.z_mps2,
        packet->signature.alg,
        packet->signature.keyId,
        packet->signature.value
    );

    if (ret > 0 && ret < sizeof(json_output)) {
        printk("%s\n", json_output);
    } else {
        printk("JSON encoding failed or buffer too small.\n");
    }
    // fflush(stdout);
    log_flush();  //flush each JSON to uart
    // k_msleep(20);
}
