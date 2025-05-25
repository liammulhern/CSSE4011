
#ifndef BASE_JSON_H
#define BASE_JSON_H

#include <zephyr/kernel.h>
#include <zephyr/data/json.h>

#define JSON_BUFFER_SIZE 1024

typedef struct {
    uint32_t timestamp;
    uint32_t uptime;

    int32_t lat;
    char ns;

    int32_t lon;
    char ew;

    int16_t alt;

    int16_t temp;
    int16_t humid;

    int16_t press;
    int16_t gas;

    int16_t x;
    int16_t y;
    int16_t z;

    uint8_t hash[32];

    int16_t dev_id;

} tracker_payload_t;


// Location block   
struct json_payload_location {
    char latitude[16];
    char ns[2];         
    char longitude[16];
    char ew[2];        
    char altitude_m[16];
};

// Environment block
struct json_payload_environment {
    char temperature_c[8];
    char humidity_percent[8];
    char pressure_hpa[8];
    char gas_ppm[8];
};

// Acceleration block
struct json_payload_acceleration {
    char x_mps2[8];
    char y_mps2[8];
    char z_mps2[8];
};

// Payload block
struct json_payload {
    char deviceId[20];
    char timestamp[72];
    uint32_t uptime;
    struct json_payload_location location;
    struct json_payload_environment environment;
    struct json_payload_acceleration acceleration;
};

// Header block
struct json_header {
    char messageId[40];
    char gatewayId[12];
    char schemaVersion[8];
    char messageType[12];
};

// Signature block
struct json_signature {
    char alg[16];
    char keyId[24];
    char value[65];
};

// Final full packet
struct json_full_packet {
    struct json_header header;
    struct json_payload payload;
    struct json_signature signature;
};

// Location
static const struct json_obj_descr json_location_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_payload_location, latitude, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_location, ns, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_location, longitude, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_location, ew, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_location, altitude_m, JSON_TOK_STRING),
};

// Environment
static const struct json_obj_descr json_env_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_payload_environment, temperature_c, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_environment, humidity_percent, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_environment, pressure_hpa, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_environment, gas_ppm, JSON_TOK_STRING),
};

// Acceleration
static const struct json_obj_descr json_accel_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_payload_acceleration, x_mps2, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_acceleration, y_mps2, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload_acceleration, z_mps2, JSON_TOK_STRING),
};

// Payload
static const struct json_obj_descr json_payload_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_payload, deviceId, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload, timestamp, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_payload, uptime, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_OBJECT(struct json_payload, location, json_location_descr),
    JSON_OBJ_DESCR_OBJECT(struct json_payload, environment, json_env_descr),
    JSON_OBJ_DESCR_OBJECT(struct json_payload, acceleration, json_accel_descr),
};

// Header
static const struct json_obj_descr json_header_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_header, messageId, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_header, gatewayId, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_header, schemaVersion, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_header, messageType, JSON_TOK_STRING),
};

// Signature
static const struct json_obj_descr json_signature_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct json_signature, alg, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_signature, keyId, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct json_signature, value, JSON_TOK_STRING),
};

// Full packet
static const struct json_obj_descr json_full_descr[] = {
    JSON_OBJ_DESCR_OBJECT(struct json_full_packet, header, json_header_descr),
    JSON_OBJ_DESCR_OBJECT(struct json_full_packet, payload, json_payload_descr),
    JSON_OBJ_DESCR_OBJECT(struct json_full_packet, signature, json_signature_descr),
};

static const char JSON_FORMAT[] =
    "{"
    "\"header\":{"
        "\"messageId\":\"%s\","
        "\"gatewayId\":\"%s\","
        "\"schemaVersion\":\"%s\","
        "\"messageType\":\"%s\""
    "},"
    "\"payload\":{"
        "\"deviceId\":\"%s\","
        "\"time\":\"%s\","
        "\"uptime\":\"%d\","
        "\"location\":{"
            "\"latitude\":\"%s\","
            "\"ns\":\"%s\","
            "\"longitude\":\"%s\","
            "\"ew\":\"%s\","
            "\"altitude_m\":\"%s\","
        "},"
        "\"environment\":{"
            "\"temperature_c\":\"%s\","
            "\"humidity_percent\":\"%s\","
            "\"pressure_hpa\":\"%s\","
            "\"gas_ppm\":\"%s\""
        "},"
        "\"acceleration\":{"
            "\"x_mps2\":\"%s\","
            "\"y_mps2\":\"%s\","
            "\"z_mps2\":\"%s\""
        "}"
    "},"
    "\"signature\":{"
        "\"alg\":\"%s\","
        "\"keyId\":\"%s\","
        "\"value\":\"%s\""
    "}"
    "}";

// void print_json_full_packet(const struct json_full_packet *packet)
extern void print_json_full_packet(const struct json_full_packet *packet);

extern void fill_json_packet_from_tracker_payload(const tracker_payload_t *payload, struct json_full_packet *packet);

extern void encode_and_print_json(const struct json_full_packet *packet);

#endif // BASE_JSON_H