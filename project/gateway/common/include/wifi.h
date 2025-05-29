#ifndef WIFI_H_
#define WIFI_H_

#define WIFI_RETRY_COUNT 10

void wifi_connect(void);

void wifi_init(void);

void wifi_thread(void);

#endif // WIFI_H_