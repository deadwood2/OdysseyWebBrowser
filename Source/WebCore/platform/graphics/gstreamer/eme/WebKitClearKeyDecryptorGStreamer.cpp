/* GStreamer ClearKey common encryption decryptor
 *
 * Copyright (C) 2016 Metrological
 * Copyright (C) 2016 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "config.h"
#include "WebKitClearKeyDecryptorGStreamer.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include <gcrypt.h>
#include <gst/base/gstbytereader.h>
#include <wtf/RunLoop.h>

#define CLEARKEY_SIZE 16

struct Key {
    GRefPtr<GstBuffer> keyID;
    GRefPtr<GstBuffer> keyValue;
};

#define WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_CK_DECRYPT, WebKitMediaClearKeyDecryptPrivate))
struct _WebKitMediaClearKeyDecryptPrivate {
    Vector<Key> keys;
    gcry_cipher_hd_t handle;
};

static void webKitMediaClearKeyDecryptorFinalize(GObject*);
static gboolean webKitMediaClearKeyDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent*);
static gboolean webKitMediaClearKeyDecryptorSetupCipher(WebKitMediaCommonEncryptionDecrypt*, GstBuffer*);
static gboolean webKitMediaClearKeyDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer* iv, GstBuffer* sample, unsigned subSamplesCount, GstBuffer* subSamples);
static void webKitMediaClearKeyDecryptorReleaseCipher(WebKitMediaCommonEncryptionDecrypt*);

GST_DEBUG_CATEGORY_STATIC(webkit_media_clear_key_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_clear_key_decrypt_debug_category

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" CLEAR_KEY_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" CLEAR_KEY_PROTECTION_SYSTEM_UUID));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-h264; audio/mpeg"));

#define webkit_media_clear_key_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaClearKeyDecrypt, webkit_media_clear_key_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void webkit_media_clear_key_decrypt_class_init(WebKitMediaClearKeyDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaClearKeyDecryptorFinalize;

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using ISOBMFF ClearKey Common Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using ISOBMFF ClearKey Common Encryption.",
        "Philippe Normand <philn@igalia.com>");

    GST_DEBUG_CATEGORY_INIT(webkit_media_clear_key_decrypt_debug_category,
        "webkitclearkey", 0, "ClearKey decryptor");

    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    cencClass->protectionSystemId = CLEAR_KEY_PROTECTION_SYSTEM_UUID;
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorHandleKeyResponse);
    cencClass->setupCipher = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorSetupCipher);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorDecrypt);
    cencClass->releaseCipher = GST_DEBUG_FUNCPTR(webKitMediaClearKeyDecryptorReleaseCipher);

    g_type_class_add_private(klass, sizeof(WebKitMediaClearKeyDecryptPrivate));
}

static void webkit_media_clear_key_decrypt_init(WebKitMediaClearKeyDecrypt* self)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(self);

    self->priv = priv;
    new (priv) WebKitMediaClearKeyDecryptPrivate();
}

static void webKitMediaClearKeyDecryptorFinalize(GObject* object)
{
    WebKitMediaClearKeyDecrypt* self = WEBKIT_MEDIA_CK_DECRYPT(object);
    WebKitMediaClearKeyDecryptPrivate* priv = self->priv;

    priv->~WebKitMediaClearKeyDecryptPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static gboolean webKitMediaClearKeyDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    const GstStructure* structure = gst_event_get_structure(event);

    // Demand the `drm-cipher-clearkey` GstStructure.
    if (!gst_structure_has_name(structure, "drm-cipher-clearkey"))
        return FALSE;

    // Retrieve the `key-ids` GStreamer value list.
    const GValue* keyIDsList = gst_structure_get_value(structure, "key-ids");
    ASSERT(keyIDsList && GST_VALUE_HOLDS_LIST(keyIDsList));
    unsigned keyIDsListSize = gst_value_list_get_size(keyIDsList);

    // Retrieve the `key-values` GStreamer value list.
    const GValue* keyValuesList = gst_structure_get_value(structure, "key-values");
    ASSERT(keyValuesList && GST_VALUE_HOLDS_LIST(keyValuesList));
    unsigned keyValuesListSize = gst_value_list_get_size(keyValuesList);

    // Bail if somehow the two lists don't match in size.
    if (keyIDsListSize != keyValuesListSize)
        return FALSE;

    // Clear out the previous list of keys.
    priv->keys.clear();

    // Append the retrieved GstBuffer objects containing each key's ID and value to the list of Key objects.
    for (unsigned i = 0; i < keyIDsListSize; ++i) {
        GRefPtr<GstBuffer> keyIDBuffer(gst_value_get_buffer(gst_value_list_get_value(keyIDsList, i)));
        GRefPtr<GstBuffer> keyValueBuffer(gst_value_get_buffer(gst_value_list_get_value(keyValuesList, i)));
        priv->keys.append(Key { WTFMove(keyIDBuffer), WTFMove(keyValueBuffer) });
    }

    return TRUE;
}

static gboolean webKitMediaClearKeyDecryptorSetupCipher(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* keyIDBuffer)
{
    if (!keyIDBuffer) {
        GST_ERROR_OBJECT(self, "got no key id buffer");
        return false;
    }

    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    gcry_error_t error;

    GRefPtr<GstBuffer> keyBuffer;
    {
        GstMapInfo keyIDBufferMap;
        if (!gst_buffer_map(keyIDBuffer, &keyIDBufferMap, GST_MAP_READ)) {
            GST_ERROR_OBJECT(self, "Failed to map key ID buffer");
            return false;
        }

        for (auto& key : priv->keys) {
            if (!gst_buffer_memcmp(key.keyID.get(), 0, keyIDBufferMap.data, keyIDBufferMap.size)) {
                keyBuffer = key.keyValue;
                break;
            }
        }

        gst_buffer_unmap(keyIDBuffer, &keyIDBufferMap);
    }

    if (!keyBuffer) {
        GST_ERROR_OBJECT(self, "Failed to find an appropriate key buffer");
        return false;
    }

    error = gcry_cipher_open(&(priv->handle), GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CTR, GCRY_CIPHER_SECURE);
    if (error) {
        GST_ERROR_OBJECT(self, "Failed to create AES 128 CTR cipher handle: %s", gpg_strerror(error));
        return false;
    }

    GstMapInfo keyMap;
    if (!gst_buffer_map(keyBuffer.get(), &keyMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map decryption key");
        return false;
    }

    ASSERT(keyMap.size == CLEARKEY_SIZE);
    error = gcry_cipher_setkey(priv->handle, keyMap.data, keyMap.size);
    gst_buffer_unmap(keyBuffer.get(), &keyMap);
    if (error) {
        GST_ERROR_OBJECT(self, "gcry_cipher_setkey failed: %s", gpg_strerror(error));
        return false;
    }

    return true;
}

static gboolean webKitMediaClearKeyDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    GstMapInfo ivMap;
    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    uint8_t ctr[CLEARKEY_SIZE];
    if (ivMap.size == 8) {
        memset(ctr + 8, 0, 8);
        memcpy(ctr, ivMap.data, 8);
    } else {
        ASSERT(ivMap.size == CLEARKEY_SIZE);
        memcpy(ctr, ivMap.data, CLEARKEY_SIZE);
    }
    gst_buffer_unmap(ivBuffer, &ivMap);

    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    gcry_error_t error = gcry_cipher_setctr(priv->handle, ctr, CLEARKEY_SIZE);
    if (error) {
        GST_ERROR_OBJECT(self, "gcry_cipher_setctr failed: %s", gpg_strerror(error));
        return false;
    }

    GstMapInfo map;
    gboolean bufferMapped = gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE));
    if (!bufferMapped) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    GstMapInfo subSamplesMap;
    gboolean subsamplesBufferMapped = gst_buffer_map(subSamplesBuffer, &subSamplesMap, GST_MAP_READ);
    if (!subsamplesBufferMapped) {
        GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
        gst_buffer_unmap(buffer, &map);
        return false;
    }

    GstByteReader* reader = gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size);
    unsigned position = 0;
    unsigned sampleIndex = 0;

    GST_DEBUG_OBJECT(self, "position: %d, size: %zu", position, map.size);

    while (position < map.size) {
        guint16 nBytesClear = 0;
        guint32 nBytesEncrypted = 0;

        if (sampleIndex < subSampleCount) {
            if (!gst_byte_reader_get_uint16_be(reader, &nBytesClear)
                || !gst_byte_reader_get_uint32_be(reader, &nBytesEncrypted)) {
                GST_DEBUG_OBJECT(self, "unsupported");
                gst_byte_reader_free(reader);
                gst_buffer_unmap(buffer, &map);
                gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
                return false;
            }

            sampleIndex++;
        } else {
            nBytesClear = 0;
            nBytesEncrypted = map.size - position;
        }

        GST_TRACE_OBJECT(self, "%d bytes clear (todo=%zu)", nBytesClear, map.size - position);
        position += nBytesClear;
        if (nBytesEncrypted) {
            GST_TRACE_OBJECT(self, "%d bytes encrypted (todo=%zu)", nBytesEncrypted, map.size - position);
            error = gcry_cipher_decrypt(priv->handle, map.data + position, nBytesEncrypted, 0, 0);
            if (error) {
                GST_ERROR_OBJECT(self, "decryption failed: %s", gpg_strerror(error));
                gst_byte_reader_free(reader);
                gst_buffer_unmap(buffer, &map);
                gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
                return false;
            }
            position += nBytesEncrypted;
        }
    }

    gst_byte_reader_free(reader);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
    return true;
}

static void webKitMediaClearKeyDecryptorReleaseCipher(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaClearKeyDecryptPrivate* priv = WEBKIT_MEDIA_CK_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_CK_DECRYPT(self));
    gcry_cipher_close(priv->handle);
}

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)
