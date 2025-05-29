#ifndef JSON_H_
#define JSON_H_

#define JSON_BUFFER_SIZE 1024

static const char JSON_FORMAT[] =
"\"payload\":{"
    "\"timestamp\":\"%04d-%02d-%02dT%02d:%02d:%02d\","
    "\"uptime\":\"%u\","
    "\"location\":{"
	"\"latitude\":\"%.7f\","
	"\"ns\":\"%c\","
	"\"longitude\":\"%.7f\","
	"\"ew\":\"%c\","
	"\"altitude_m\":\"%.1f\""
    "},"
    "\"environment\":{"
	"\"temperature_c\":\"%.2f\","
	"\"humidity_percent\":\"%.2f\","
	"\"pressure_hpa\":\"%.1f\","
	"\"gas_ppm\":\"%.2f\""
    "},"
    "\"acceleration\":{"
	"\"x_mps2\":\"%.3f\","
	"\"y_mps2\":\"%.3f\","
	"\"z_mps2\":\"%.3f\""
    "}"
"}";

#endif
