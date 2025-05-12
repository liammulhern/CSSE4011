target_sources(app PRIVATE main.c)

target_sources(app PRIVATE cli/cli.c)

target_sources(app PRIVATE communication/bluetooth.c)
target_sources(app PRIVATE communication/nfg.c)

target_sources(app PRIVATE sensors/gnss.c)
