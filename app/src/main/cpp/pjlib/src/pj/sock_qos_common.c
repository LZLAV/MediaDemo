/**
 * 已看完
 */
#include <pj/sock_qos.h>
#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/log.h>
#include <pj/string.h>

#define THIS_FILE   "sock_qos_common.c"
#define ALL_FLAGS   (PJ_QOS_PARAM_HAS_DSCP | PJ_QOS_PARAM_HAS_SO_PRIO | \
                     PJ_QOS_PARAM_HAS_WMM)

/* "Standard" mapping between traffic type and QoS params */
static const pj_qos_params qos_map[] =
        {
                /* flags	dscp  prio wmm_prio */
                {ALL_FLAGS, 0x00, 0, PJ_QOS_WMM_PRIO_BULK_EFFORT},   /* BE */
                {ALL_FLAGS, 0x08, 2, PJ_QOS_WMM_PRIO_BULK},        /* BK */
                {ALL_FLAGS, 0x28, 5, PJ_QOS_WMM_PRIO_VIDEO},        /* VI */
                {ALL_FLAGS, 0x30, 6, PJ_QOS_WMM_PRIO_VOICE},        /* VO */
                {ALL_FLAGS, 0x38, 7, PJ_QOS_WMM_PRIO_VOICE},        /* CO */
                {ALL_FLAGS, 0x28, 5, PJ_QOS_WMM_PRIO_VIDEO}        /* SIG */
        };


/* 获取指定流量类型的参数 */
PJ_DEF(pj_status_t) pj_qos_get_params(pj_qos_type type,
                                      pj_qos_params *p_param) {
    PJ_ASSERT_RETURN(type <= PJ_QOS_TYPE_CONTROL && p_param, PJ_EINVAL);
    pj_memcpy(p_param, &qos_map[type], sizeof(*p_param));
    return PJ_SUCCESS;
}

/* 获取匹配的流量类型 */
PJ_DEF(pj_status_t) pj_qos_get_type(const pj_qos_params *param,
                                    pj_qos_type *p_type) {
    unsigned dscp_type = PJ_QOS_TYPE_BEST_EFFORT,
            prio_type = PJ_QOS_TYPE_BEST_EFFORT,
            wmm_type = PJ_QOS_TYPE_BEST_EFFORT;
    unsigned i, count = 0;

    PJ_ASSERT_RETURN(param && p_type, PJ_EINVAL);

    if (param->flags & PJ_QOS_PARAM_HAS_DSCP) {
        for (i = 0; i <= PJ_QOS_TYPE_CONTROL; ++i) {
            if (param->dscp_val >= qos_map[i].dscp_val)
                dscp_type = (pj_qos_type) i;
        }
        ++count;
    }

    if (param->flags & PJ_QOS_PARAM_HAS_SO_PRIO) {
        for (i = 0; i <= PJ_QOS_TYPE_CONTROL; ++i) {
            if (param->so_prio >= qos_map[i].so_prio)
                prio_type = (pj_qos_type) i;
        }
        ++count;
    }

    if (param->flags & PJ_QOS_PARAM_HAS_WMM) {
        for (i = 0; i <= PJ_QOS_TYPE_CONTROL; ++i) {
            if (param->wmm_prio >= qos_map[i].wmm_prio)
                wmm_type = (pj_qos_type) i;
        }
        ++count;
    }

    if (count)
        *p_type = (pj_qos_type) ((dscp_type + prio_type + wmm_type) / count);
    else
        *p_type = PJ_QOS_TYPE_BEST_EFFORT;

    return PJ_SUCCESS;
}

/* 应用 Qos */
PJ_DEF(pj_status_t) pj_sock_apply_qos(pj_sock_t sock,
                                      pj_qos_type qos_type,
                                      pj_qos_params *qos_params,
                                      unsigned log_level,
                                      const char *log_sender,
                                      const char *sock_name) {
    pj_status_t qos_type_rc = PJ_SUCCESS,
            qos_params_rc = PJ_SUCCESS;

    if (!log_sender)
        log_sender = THIS_FILE;
    if (!sock_name)
        sock_name = "socket";

    if (qos_type != PJ_QOS_TYPE_BEST_EFFORT) {
        qos_type_rc = pj_sock_set_qos_type(sock, qos_type);

        if (qos_type_rc != PJ_SUCCESS) {
            pj_perror(log_level, log_sender, qos_type_rc,
                      "Error setting QoS type %d to %s",
                      qos_type, sock_name);
        }
    }

    if (qos_params && qos_params->flags) {
        qos_params_rc = pj_sock_set_qos_params(sock, qos_params);
        if (qos_params_rc != PJ_SUCCESS) {
            pj_perror(log_level, log_sender, qos_params_rc,
                      "Error setting QoS params (flags=%d) to %s",
                      qos_params->flags, sock_name);
            if (qos_type_rc != PJ_SUCCESS)
                return qos_params_rc;
        }
    } else if (qos_type_rc != PJ_SUCCESS)
        return qos_type_rc;

    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pj_sock_apply_qos2(pj_sock_t sock,
                                       pj_qos_type qos_type,
                                       const pj_qos_params *qos_params,
                                       unsigned log_level,
                                       const char *log_sender,
                                       const char *sock_name) {
    pj_qos_params qos_params_buf, *qos_params_copy = NULL;

    if (qos_params) {
        pj_memcpy(&qos_params_buf, qos_params, sizeof(*qos_params));
        qos_params_copy = &qos_params_buf;
    }

    return pj_sock_apply_qos(sock, qos_type, qos_params_copy,
                             log_level, log_sender, sock_name);
}
