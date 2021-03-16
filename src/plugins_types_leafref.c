/**
 * @file plugins_types_leafref.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief Built-in leafref type plugin.
 *
 * Copyright (c) 2019-2021 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#define _GNU_SOURCE

#include "plugins_types.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libyang.h"

/* additional internal headers for some useful simple macros */
#include "common.h"
#include "compat.h"

/**
 * @brief Store and canonize value of the YANG built-in leafref type.
 *
 * Implementation of the ly_type_store_clb.
 */
LY_ERR
ly_type_store_leafref(const struct ly_ctx *ctx, const struct lysc_type *type, const char *value, size_t value_len,
        uint32_t options, LY_PREFIX_FORMAT format, void *prefix_data, uint32_t hints, const struct lysc_node *ctx_node,
        struct lyd_value *storage, struct lys_glob_unres *unres, struct ly_err_item **err)
{
    LY_ERR ret = LY_SUCCESS;
    struct lysc_type_leafref *type_lr = (struct lysc_type_leafref *)type;

    assert(type_lr->realtype);

    /* store the value as the real type of the leafref target */
    ret = type_lr->realtype->plugin->store(ctx, type_lr->realtype, value, value_len, options, format, prefix_data,
            hints, ctx_node, storage, unres, err);
    if (ret == LY_EINCOMPLETE) {
        /* it is irrelevant whether the target type needs some resolving */
        ret = LY_SUCCESS;
    }
    LY_CHECK_RET(ret);

    if (type_lr->require_instance) {
        /* needs to be resolved */
        return LY_EINCOMPLETE;
    } else {
        return LY_SUCCESS;
    }
}

/**
 * @brief Validate value of the YANG built-in leafref type.
 *
 * Implementation of the ly_type_validate_clb.
 */
LY_ERR
ly_type_validate_leafref(const struct ly_ctx *UNUSED(ctx), const struct lysc_type *type, const struct lyd_node *ctx_node,
        const struct lyd_node *tree, struct lyd_value *storage, struct ly_err_item **err)
{
    LY_ERR ret = LY_SUCCESS;
    struct lysc_type_leafref *type_lr = (struct lysc_type_leafref *)type;
    char *errmsg = NULL;

    *err = NULL;

    if (!type_lr->require_instance) {
        /* redundant to resolve */
        return LY_SUCCESS;
    }

    /* check leafref target existence */
    if (ly_type_find_leafref(type_lr, ctx_node, storage, tree, NULL, &errmsg)) {
        ret = ly_err_new(err, LY_EVALID, LYVE_DATA, NULL, NULL, errmsg);
        if (errmsg != NULL) {
            free(errmsg);
        }
    }

    return ret;
}

/**
 * @brief Comparison callback checking the leafref value.
 *
 * Implementation of the ly_type_compare_clb.
 */
LY_ERR
ly_type_compare_leafref(const struct lyd_value *val1, const struct lyd_value *val2)
{
    return val1->realtype->plugin->compare(val1, val2);
}

/**
 * @brief Printer callback printing the leafref value.
 *
 * Implementation of the ly_type_print_clb.
 */
const char *
ly_type_print_leafref(const struct lyd_value *value, LY_PREFIX_FORMAT format, void *prefix_data, ly_bool *dynamic)
{
    return value->realtype->plugin->print(value, format, prefix_data, dynamic);
}

/**
 * @brief Duplication callback of the leafref values.
 *
 * Implementation of the ly_type_dup_clb.
 */
LY_ERR
ly_type_dup_leafref(const struct ly_ctx *ctx, const struct lyd_value *original, struct lyd_value *dup)
{
    return original->realtype->plugin->duplicate(ctx, original, dup);
}

/**
 * @brief Free value of the YANG built-in leafref type.
 *
 * Implementation of the ly_type_free_clb.
 */
void
ly_type_free_leafref(const struct ly_ctx *ctx, struct lyd_value *value)
{
    if (value->realtype->plugin != &ly_builtin_type_plugins[LY_TYPE_LEAFREF]) {
        /* leafref's realtype is again leafref only in case of incomplete store */
        value->realtype->plugin->free(ctx, value);
    }
}