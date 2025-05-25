
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <drivers/nfc/t2t/tag.h>
#include <nfc/ndef/msg.h>
#include <m
#include <nfc/ndef/uri_rec.h>

/* 1) The actual URI string you want to encode (no URI prefix!) */
static u8_t payload[] = "example.com/my/path";

/* 2) Build the URI record descriptor: */
static const struct nfc_ndef_uri_record_desc uri_rec =
    NFC_NDEF_URI_RECORD_DESC(UTF_8,
                             NFC_URI_HTTP_WWW, /* will prepend "http://www." */
                             payload);

/* 3) Wrap into an NDEF message: */
static struct nfc_ndef_msg_desc nfc_msg =
    NFC_NDEF_MSG_DESC(uri_rec);

int main(void)
{
    int err;
    size_t len;

    printk("Starting NFC URI emulation...\n");

    /* 4) Encode the message into the T2T memory buffer */
    err = nfc_t2t_tag_mem_init();
    if (err) {
        printk("  tag_mem_init failed: %d\n", err);
        return err;
    }

    err = nfc_ndef_msg_enc(&nfc_msg, NULL, &len);
    if (err) {
        printk("  NDEF encode failed: %d\n", err);
        return err;
    }

    err = nfc_t2t_tag_mem_write(nfc_t2t_tag_mem_get(), len);
    if (err) {
        printk("  tag_mem_write failed: %d\n", err);
        return err;
    }

    /* 5) Start the emulation: */
    err = nfc_t2t_emulation_start();
    if (err) {
        printk("  emulation_start failed: %d\n", err);
        return err;
    }

    printk("  NFC tag active—tap a phone to read URI!\n");

    return 0;
}

