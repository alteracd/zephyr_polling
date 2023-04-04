/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <common/bt_settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>

#include "common/bt_str.h"

#include "hci_core.h"
#include "settings.h"

#define LOG_MODULE_NAME settings
#include "logging/bt_log.h"




/*
 * Append a single value to persisted config. Don't store duplicate value.
 */
int settings_save_one(const char *name, const void *value, size_t val_len)
{
	// int rc;
	// struct settings_store *cs;

	// cs = settings_save_dst;
	// if (!cs) {
	// 	return -ENOENT;
	// }

	// k_mutex_lock(&settings_lock, K_FOREVER);

	// rc = cs->cs_itf->csi_save(cs, name, (char *)value, val_len);

	// k_mutex_unlock(&settings_lock);

	// return rc;
    return 0;
}

int settings_delete(const char *name)
{
	return settings_save_one(name, NULL, 0);
}

int settings_save(void)
{
// 	struct settings_store *cs;
// 	int rc;
// 	int rc2;

// 	cs = settings_save_dst;
// 	if (!cs) {
// 		return -ENOENT;
// 	}

// 	if (cs->cs_itf->csi_save_start) {
// 		cs->cs_itf->csi_save_start(cs);
// 	}
// 	rc = 0;

// 	STRUCT_SECTION_FOREACH(settings_handler_static, ch) {
// 		if (ch->h_export) {
// 			rc2 = ch->h_export(settings_save_one);
// 			if (!rc) {
// 				rc = rc2;
// 			}
// 		}
// 	}

// #if defined(CONFIG_SETTINGS_DYNAMIC_HANDLERS)
// 	struct settings_handler *ch;
// 	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
// 		if (ch->h_export) {
// 			rc2 = ch->h_export(settings_save_one);
// 			if (!rc) {
// 				rc = rc2;
// 			}
// 		}
// 	}
// #endif /* CONFIG_SETTINGS_DYNAMIC_HANDLERS */

// 	if (cs->cs_itf->csi_save_end) {
// 		cs->cs_itf->csi_save_end(cs);
// 	}
// return rc;
    return 0;
}

int settings_storage_get(void **storage)
{
	// struct settings_store *cs = settings_save_dst;

	// if (!cs) {
	// 	return -ENOENT;
	// }

	// if (cs->cs_itf->csi_storage_get) {
	// 	*storage = cs->cs_itf->csi_storage_get(cs);
	// }

	return 0;
}

void settings_store_init(void)
{
	// sys_slist_init(&settings_load_srcs);
}













































#if defined(CONFIG_BT_SETTINGS_USE_PRINTK)
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
                            const bt_addr_le_t *addr, const char *key)
{
    if (key)
    {
        snprintk(path, path_size, "bt/%s/%02x%02x%02x%02x%02x%02x%u/%s", subsys, addr->a.val[5],
                 addr->a.val[4], addr->a.val[3], addr->a.val[2], addr->a.val[1], addr->a.val[0],
                 addr->type, key);
    }
    else
    {
        snprintk(path, path_size, "bt/%s/%02x%02x%02x%02x%02x%02x%u", subsys, addr->a.val[5],
                 addr->a.val[4], addr->a.val[3], addr->a.val[2], addr->a.val[1], addr->a.val[0],
                 addr->type);
    }

    LOG_DBG("Encoded path %s", path);
}
#else
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
                            const bt_addr_le_t *addr, const char *key)
{
    size_t len = 3;

    /* Skip if path_size is less than 3; strlen("bt/") */
    if (len < path_size)
    {
        /* Key format:
         *  "bt/<subsys>/<addr><type>/<key>", "/<key>" is optional
         */
        strcpy(path, "bt/");
        strncpy(&path[len], subsys, path_size - len);
        len = strlen(path);
        if (len < path_size)
        {
            path[len] = '/';
            len++;
        }

        for (int8_t i = 5; i >= 0 && len < path_size; i--)
        {
            len += bin2hex(&addr->a.val[i], 1, &path[len], path_size - len);
        }

        if (len < path_size)
        {
            /* Type can be either BT_ADDR_LE_PUBLIC or
             * BT_ADDR_LE_RANDOM (value 0 or 1)
             */
            path[len] = '0' + addr->type;
            len++;
        }

        if (key && len < path_size)
        {
            path[len] = '/';
            len++;
            strncpy(&path[len], key, path_size - len);
            len += strlen(&path[len]);
        }

        if (len >= path_size)
        {
            /* Truncate string */
            path[path_size - 1] = '\0';
        }
    }
    else if (path_size > 0)
    {
        *path = '\0';
    }

    LOG_DBG("Encoded path %s", path);
}
#endif

int bt_settings_decode_key(const char *key, bt_addr_le_t *addr)
{
    if (settings_name_next(key, NULL) != 13)
    {
        return -EINVAL;
    }

    if (key[12] == '0')
    {
        addr->type = BT_ADDR_LE_PUBLIC;
    }
    else if (key[12] == '1')
    {
        addr->type = BT_ADDR_LE_RANDOM;
    }
    else
    {
        return -EINVAL;
    }

    for (uint8_t i = 0; i < 6; i++)
    {
        hex2bin(&key[i * 2], 2, &addr->a.val[5 - i], 1);
    }

    LOG_DBG("Decoded %s as %s", key, bt_addr_le_str(addr));

    return 0;
}

static int set_setting(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
    ssize_t len;
    const char *next;

    if (!atomic_test_bit(bt_dev.flags, BT_DEV_ENABLE))
    {
        /* The Bluetooth settings loader needs to communicate with the Bluetooth
         * controller to setup identities. This will not work before
         * bt_enable(). The doc on @ref bt_enable requires the "bt/" settings
         * tree to be loaded after @ref bt_enable is completed, so this handler
         * will be called again later.
         */
        return 0;
    }

    if (!name)
    {
        LOG_ERR("Insufficient number of arguments");
        return -ENOENT;
    }

    len = settings_name_next(name, &next);

    if (!strncmp(name, "id", len))
    {
        /* Any previously provided identities supersede flash */
        if (atomic_test_bit(bt_dev.flags, BT_DEV_PRESET_ID))
        {
            LOG_WRN("Ignoring identities stored in flash");
            return 0;
        }

        len = read_cb(cb_arg, &bt_dev.id_addr, sizeof(bt_dev.id_addr));
        if (len < sizeof(bt_dev.id_addr[0]))
        {
            if (len < 0)
            {
                LOG_ERR("Failed to read ID address from storage"
                        " (err %zd)",
                        len);
            }
            else
            {
                LOG_ERR("Invalid length ID address in storage");
                LOG_HEXDUMP_DBG(&bt_dev.id_addr, len, "data read");
            }
            (void)memset(bt_dev.id_addr, 0, sizeof(bt_dev.id_addr));
            bt_dev.id_count = 0U;
        }
        else
        {
            int i;

            bt_dev.id_count = len / sizeof(bt_dev.id_addr[0]);
            for (i = 0; i < bt_dev.id_count; i++)
            {
                LOG_DBG("ID[%d] %s", i, bt_addr_le_str(&bt_dev.id_addr[i]));
            }
        }

        return 0;
    }

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
    if (!strncmp(name, "name", len))
    {
        len = read_cb(cb_arg, &bt_dev.name, sizeof(bt_dev.name) - 1);
        if (len < 0)
        {
            LOG_ERR("Failed to read device name from storage"
                    " (err %zd)",
                    len);
        }
        else
        {
            bt_dev.name[len] = '\0';

            LOG_DBG("Name set to %s", bt_dev.name);
        }
        return 0;
    }
#endif

#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
    if (!strncmp(name, "appearance", len))
    {
        if (len_rd != sizeof(bt_dev.appearance))
        {
            LOG_ERR("Ignoring settings entry 'bt/appearance'. Wrong length.");
            return -EINVAL;
        }

        len = read_cb(cb_arg, &bt_dev.appearance, sizeof(bt_dev.appearance));
        if (len < 0)
        {
            return len;
        }

        return 0;
    }
#endif

#if defined(CONFIG_BT_PRIVACY)
    if (!strncmp(name, "irk", len))
    {
        len = read_cb(cb_arg, bt_dev.irk, sizeof(bt_dev.irk));
        if (len < sizeof(bt_dev.irk[0]))
        {
            if (len < 0)
            {
                LOG_ERR("Failed to read IRK from storage"
                        " (err %zd)",
                        len);
            }
            else
            {
                LOG_ERR("Invalid length IRK in storage");
                (void)memset(bt_dev.irk, 0, sizeof(bt_dev.irk));
            }
        }
        else
        {
            int i, count;

            count = len / sizeof(bt_dev.irk[0]);
            for (i = 0; i < count; i++)
            {
                LOG_DBG("IRK[%d] %s", i, bt_hex(bt_dev.irk[i], 16));
            }
        }

        return 0;
    }
#endif /* CONFIG_BT_PRIVACY */

    return -ENOENT;
}

#define ID_DATA_LEN(array) (bt_dev.id_count * sizeof(array[0]))

static void save_id(struct k_work *work)
{
    int err;
    LOG_INF("Saving ID");
    err = settings_save_one("bt/id", &bt_dev.id_addr, ID_DATA_LEN(bt_dev.id_addr));
    if (err)
    {
        LOG_ERR("Failed to save ID (err %d)", err);
    }

#if defined(CONFIG_BT_PRIVACY)
    err = settings_save_one("bt/irk", bt_dev.irk, ID_DATA_LEN(bt_dev.irk));
    if (err)
    {
        LOG_ERR("Failed to save IRK (err %d)", err);
    }
#endif
}

K_WORK_DEFINE(save_id_work, save_id);

void bt_settings_save_id(void)
{
    k_work_submit(&save_id_work);
}

static int commit_settings(void)
{
    int err;

    LOG_DBG("");

    if (!atomic_test_bit(bt_dev.flags, BT_DEV_ENABLE))
    {
        /* The Bluetooth settings loader needs to communicate with the Bluetooth
         * controller to setup identities. This will not work before
         * bt_enable(). The doc on @ref bt_enable requires the "bt/" settings
         * tree to be loaded after @ref bt_enable is completed, so this handler
         * will be called again later.
         */
        return 0;
    }

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
    if (bt_dev.name[0] == '\0')
    {
        bt_set_name(CONFIG_BT_DEVICE_NAME);
    }
#endif
    if (!bt_dev.id_count)
    {
        err = bt_setup_public_id_addr();
        if (err)
        {
            LOG_ERR("Unable to setup an identity address");
            return err;
        }
    }

    if (!bt_dev.id_count)
    {
        err = bt_setup_random_id_addr();
        if (err)
        {
            LOG_ERR("Unable to setup an identity address");
            return err;
        }
    }

    if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY))
    {
        bt_finalize_init();
    }

    /* If any part of the Identity Information of the device has been
     * generated this Identity needs to be saved persistently.
     */
    if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_STORE_ID))
    {
        LOG_DBG("Storing Identity Information");
        bt_settings_save_id();
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt, "bt", NULL, set_setting, commit_settings, NULL);

int bt_settings_init(void)
{
    int err;

    LOG_DBG("");

    // err = settings_subsys_init();
    // if (err)
    // {
    //     LOG_ERR("settings_subsys_init failed (err %d)", err);
    //     return err;
    // }

    return 0;
}
