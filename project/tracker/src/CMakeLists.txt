target_sources(app PRIVATE main.c)

target_sources(app PRIVATE cli/cli.c)

target_sources(app PRIVATE communication/bluetooth.c)
target_sources(app PRIVATE communication/nfc.c)

target_sources(app PRIVATE sensors/gnss.c)
