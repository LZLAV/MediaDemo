/**
 * 已看完
 *      bsd qos:
 *          设置：
 *              IPv4：
 *                  pj_sock_setsockopt(sock, pj_SOL_IP(), pj_IP_TOS(),&val, sizeof(val))
 *              IPv6:
 *                  pj_sock_setsockopt(sock, pj_SOL_IPV6(),pj_IPV6_TCLASS(),&val, sizeof(val))
 *
 *              pj_sock_setsockopt(sock, pj_SOL_SOCKET(), pj_SO_PRIORITY(),&val, sizeof(val))
 *          获取：
 *              IPv4:
 *                  pj_sock_getsockopt(sock, pj_SOL_IP(), pj_IP_TOS(),&val, &optlen)
 *              IPv6:
 *                  pj_sock_getsockopt(sock, pj_SOL_IPV6(),pj_IPV6_TCLASS(),&val, &optlen)
 *
 *              pj_sock_getsockopt(sock, pj_SOL_SOCKET(), pj_SO_PRIORITY(),&val, &optlen)
 *
 *
 */
#include <pj/sock_qos.h>
#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/string.h>

/**
 * Socket Qos
 * 		设置 type 和 qos_params
 */

/* This is the implementation of QoS with BSD socket's setsockopt(),
 * using IP_TOS/IPV6_TCLASS and SO_PRIORITY
 */
#if !defined(PJ_QOS_IMPLEMENTATION) || PJ_QOS_IMPLEMENTATION == PJ_QOS_BSD

PJ_DEF(pj_status_t) pj_sock_set_qos_params(pj_sock_t sock,
                                           pj_qos_params *param) {
    pj_status_t last_err = PJ_ENOTSUP;
    pj_status_t status;

    /* No op? */
    if (!param->flags)
        return PJ_SUCCESS;

    /* Clear WMM field since we don't support it */
    param->flags &= ~(PJ_QOS_PARAM_HAS_WMM);

    /* Set TOS/DSCP */
    if (param->flags & PJ_QOS_PARAM_HAS_DSCP) {
        /* We need to know if the socket is IPv4 or IPv6 */
        pj_sockaddr sa;
        int salen = sizeof(salen);

        /* Value is dscp_val << 2 */
        int val = (param->dscp_val << 2);

        status = pj_sock_getsockname(sock, &sa, &salen);
        if (status != PJ_SUCCESS)
            return status;

        if (sa.addr.sa_family == pj_AF_INET()) {
            /* In IPv4, the DS field goes in the TOS field */
            status = pj_sock_setsockopt(sock, pj_SOL_IP(), pj_IP_TOS(),
                                        &val, sizeof(val));
        } else if (sa.addr.sa_family == pj_AF_INET6()) {
            /* In IPv6, the DS field goes in the Traffic Class field */
            status = pj_sock_setsockopt(sock, pj_SOL_IPV6(),
                                        pj_IPV6_TCLASS(),
                                        &val, sizeof(val));
        } else {
            status = PJ_EINVAL;
        }

        if (status != PJ_SUCCESS) {
            param->flags &= ~(PJ_QOS_PARAM_HAS_DSCP);
            last_err = status;
        }
    }

    /* Set SO_PRIORITY */
    if (param->flags & PJ_QOS_PARAM_HAS_SO_PRIO) {
        int val = param->so_prio;
        status = pj_sock_setsockopt(sock, pj_SOL_SOCKET(), pj_SO_PRIORITY(),
                                    &val, sizeof(val));
        if (status != PJ_SUCCESS) {
            param->flags &= ~(PJ_QOS_PARAM_HAS_SO_PRIO);
            last_err = status;
        }
    }

    return param->flags ? PJ_SUCCESS : last_err;
}

PJ_DEF(pj_status_t) pj_sock_set_qos_type(pj_sock_t sock,
                                         pj_qos_type type) {
    pj_qos_params param;
    pj_status_t status;

    status = pj_qos_get_params(type, &param);
    if (status != PJ_SUCCESS)
        return status;

    return pj_sock_set_qos_params(sock, &param);
}


PJ_DEF(pj_status_t) pj_sock_get_qos_params(pj_sock_t sock,
                                           pj_qos_params *p_param) {
    pj_status_t last_err = PJ_ENOTSUP;
    int val = 0, optlen;
    pj_sockaddr sa;
    int salen = sizeof(salen);
    pj_status_t status;

    pj_bzero(p_param, sizeof(*p_param));

    /* Get DSCP/TOS value */
    status = pj_sock_getsockname(sock, &sa, &salen);
    if (status == PJ_SUCCESS) {
        optlen = sizeof(val);
        if (sa.addr.sa_family == pj_AF_INET()) {
            status = pj_sock_getsockopt(sock, pj_SOL_IP(), pj_IP_TOS(),
                                        &val, &optlen);
        } else if (sa.addr.sa_family == pj_AF_INET6()) {
            status = pj_sock_getsockopt(sock, pj_SOL_IPV6(),
                                        pj_IPV6_TCLASS(),
                                        &val, &optlen);
        } else {
            status = PJ_EINVAL;
        }

        if (status == PJ_SUCCESS) {
            p_param->flags |= PJ_QOS_PARAM_HAS_DSCP;
            p_param->dscp_val = (pj_uint8_t) (val >> 2);
        } else {
            last_err = status;
        }
    } else {
        last_err = status;
    }

    /* Get SO_PRIORITY */
    optlen = sizeof(val);
    status = pj_sock_getsockopt(sock, pj_SOL_SOCKET(), pj_SO_PRIORITY(),
                                &val, &optlen);
    if (status == PJ_SUCCESS) {
        p_param->flags |= PJ_QOS_PARAM_HAS_SO_PRIO;
        p_param->so_prio = (pj_uint8_t) val;
    } else {
        last_err = status;
    }

    /* WMM is not supported */

    return p_param->flags ? PJ_SUCCESS : last_err;
}

PJ_DEF(pj_status_t) pj_sock_get_qos_type(pj_sock_t sock,
                                         pj_qos_type *p_type) {
    pj_qos_params param;
    pj_status_t status;

    status = pj_sock_get_qos_params(sock, &param);
    if (status != PJ_SUCCESS)
        return status;

    return pj_qos_get_type(&param, p_type);
}

#endif    /* PJ_QOS_IMPLEMENTATION */

