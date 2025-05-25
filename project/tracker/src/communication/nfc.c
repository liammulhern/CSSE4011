#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <drivers/nfc/t2t/tag.h>
#include <nfc/ndef/msg.h>
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
