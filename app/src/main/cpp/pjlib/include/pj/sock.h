/** 已完成
 * 		协议族：
 * 			AF_UNSPEC
 * 			AF_UNIX
 * 			AF_INET
 * 			AF_INET6
 * 			AF_PACKET
 * 			AF_IRDA
 *
 * 		socket 类型：
 * 			SOCK_STREAM
 * 			SOCK_DGRAM
 * 			SOCK_RAW
 * 			SOCK_RDM
 *
 * 		socket option:
 * 			级别：
 * 				SOL_SOCKET
 * 				SOL_IP
 * 				SOL_TCP
 * 				SOL_UDP
 * 				SOL_IPv6
 * 				IP_TOS
 *
 * 				IP_TOS 相关：
 * 					IPTOS_LOWDELAY
 * 					IPTOS_THROUGHPUT
 * 					IPTOS_RELIABILITY
 * 					IPTOS_MINCOST
 *
 * 				IPV6_TCLASS
 *
 * 				SO_PRIORITY
 *
 * 			options 名称：
 * 				SO_TYPE
 * 				SO_RCVBUF
 * 				SO_SNDBUF
 * 				TCP_NODELAY
 * 				SO_REUSEADDR
 * 				SO_NOSIGPIPE
 *
 *
 * 			IP 多播:
 * 				IP_MULTICAST_IF
 * 				IP_MULTICAST_TTL
 * 				IP_MULTICAST_LOOP
 * 				IP_ADD_MEMBERSHIP
 * 				IP_DROP_MEMBERSHIP
 *
 * 			sock_recv、sock_send 指定的标识：flags
 * 				MSG_OOB
 * 				MSG_PEEK
 * 				MSG_DONTROUTE
 *
 *
 * 			shutdown 标识：flags
 * 				SHUT_RD(1)		不再接收
 * 				SHUT_WR(2)		不再发送
 * 				SHUT_RDWR(3)	不再接收和发送
 *
 * 			IPv4
 * 				地址：
 * 					length : 16
 *
 *
 * 			IPv6:
 * 				地址：
 * 					length : 46
 *
 *
 *
 * 			socket地址操作：
 * 				1. 网络字节顺序 <--> 主机字节顺序（16位，32位）
 * 				2. 网络字节顺序 <--> 点分十进制字符串
 * 				3. 数字地址 <--> 文本字符串
 * 				4. 点分十进制 <--> IP地址
 * 				5. IPv4 -> IPv6		合成IP地址
 * 				6. parse addr:
 * 					IPv4:
 * 						10.0.0.1:80			10.0.0.1		80
 * 						localhost:80		127.0.0.1		80
 * 					IPv6:
 * 						[fec0::01]:80		[fec0::01]		80
 * 						"fec0::01:80":01	fec0::01:80		01
 *
 *
 * 			属性：
 * 				addr
 * 				port
 * 				length
 * 				hostname
 * 				hostaddr
 *
 *
 * 			socket api:
 * 				socket()
 * 				close()
 * 				bind()
 * 					addr
 * 					addr + port
 * 					addr + random addr
 * 				listen()
 * 				accept()
 * 				connect()
 * 				getpeername()
 * 				getsockname()
 * 				getsockopt()
 * 				setsockopt()
 * 				setsockopt_params()
 * 				setsockopt_sobuf()
 * 				recv()
 * 				recvfrom()
 * 				send()
 * 				sendto()
 * 				shutdown()
 *
 *
 *
 */
#ifndef __PJ_SOCK_H__
#define __PJ_SOCK_H__

/**
 * @file sock.h
 * @brief Socket 抽象
 */

#include <pj/types.h>

PJ_BEGIN_DECL 


/**
 * @defgroup PJ_SOCK Socket 抽象
 * @ingroup PJ_IO
 * @{
 *
 * PJLIB socket抽象层是 socketapi 的一个精简的、非常可移植的抽象层。它提供了类似于BSD套接字API的API。
 * 这种抽象是必要的，因为BSD套接字API并非总是在所有平台上都可用，因此除非我们提供这种抽象，否则不可能创
 * 建真正可移植的网络程序。
 *
 * 应用程序可以在其应用程序中直接使用此API，就像使用传统的BSD套接字API一样，前提是它们首先调用 pj_init()。
 *
 * \section 例子
 *
 * 有关如何使用socket API的一些示例，请参见：
 *
 *  - \ref page_pjlib_sock_test
 *  - \ref page_pjlib_select_test
 *  - \ref page_pjlib_sock_perf_test		perf  性能
 */


/**
 * 支持的协议族：
 *	应用须使用以下值，来取代 AF_*，库会将这些值转换为原生值（AF_*）
 */

/** 未指定协议族. */
extern const pj_uint16_t PJ_AF_UNSPEC;

/**
 * Unix 域 socket
 * unix本机进程间通信
 * */
extern const pj_uint16_t PJ_AF_UNIX;

/** POSIX name for AF_UNIX	*/
#define PJ_AF_LOCAL	 PJ_AF_UNIX;

/**IPv4
 * @see pj_AF_INET() */
extern const pj_uint16_t PJ_AF_INET;

/**IPv6
 * @see pj_AF_INET6() */
extern const pj_uint16_t PJ_AF_INET6;

/**Packet family.
 * @see pj_AF_PACKET() */
extern const pj_uint16_t PJ_AF_PACKET;

/** IRDA sockets. @see pj_AF_IRDA() */
extern const pj_uint16_t PJ_AF_IRDA;

/*
 * 各种地址族常量的访问器函数。提供这些函数是因为Symbian不允许从DLL导出全局变量。
 */

#if defined(PJ_DLL)
    /** Get #PJ_AF_UNSPEC value */
    PJ_DECL(pj_uint16_t) pj_AF_UNSPEC(void);
    /** Get #PJ_AF_UNIX value. */
    PJ_DECL(pj_uint16_t) pj_AF_UNIX(void);
    /** Get #PJ_AF_INET value. */
    PJ_DECL(pj_uint16_t) pj_AF_INET(void);
    /** Get #PJ_AF_INET6 value. */
    PJ_DECL(pj_uint16_t) pj_AF_INET6(void);
    /** Get #PJ_AF_PACKET value. */
    PJ_DECL(pj_uint16_t) pj_AF_PACKET(void);
    /** Get #PJ_AF_IRDA value. */
    PJ_DECL(pj_uint16_t) pj_AF_IRDA(void);
#else
    /**
     * 当pjlib不是作为DLL构建的时候，这些访问器函数只是一个宏来获取它们的常量
     */
    /** Get #PJ_AF_UNSPEC value */
#   define pj_AF_UNSPEC()   PJ_AF_UNSPEC
    /** Get #PJ_AF_UNIX value. */
#   define pj_AF_UNIX()	    PJ_AF_UNIX
    /** Get #PJ_AF_INET value. */
#   define pj_AF_INET()	    PJ_AF_INET
    /** Get #PJ_AF_INET6 value. */
#   define pj_AF_INET6()    PJ_AF_INET6
    /** Get #PJ_AF_PACKET value. */
#   define pj_AF_PACKET()   PJ_AF_PACKET
    /** Get #PJ_AF_IRDA value. */
#   define pj_AF_IRDA()	    PJ_AF_IRDA
#endif


/**
 * 支持的Sokcet类型
 * 	应用必须使用这些值来代替常规的 SOCK_*
 */

/** TCP
 *  @see pj_SOCK_STREAM() */
extern const pj_uint16_t PJ_SOCK_STREAM;

/** UDP
 *  @see pj_SOCK_DGRAM() */
extern const pj_uint16_t PJ_SOCK_DGRAM;

/** Raw
 *  @see pj_SOCK_RAW() */
extern const pj_uint16_t PJ_SOCK_RAW;

/** 一种可靠的UDP形式
 *  @see pj_SOCK_RDM() */
extern const pj_uint16_t PJ_SOCK_RDM;


/**
 * 各种常量的访问器函数。提供这些函数是因为Symbian不允许从DLL导出全局变量。
 */

#if defined(PJ_DLL)
    /** Get #PJ_SOCK_STREAM constant */
    PJ_DECL(int) pj_SOCK_STREAM(void);
    /** Get #PJ_SOCK_DGRAM constant */
    PJ_DECL(int) pj_SOCK_DGRAM(void);
    /** Get #PJ_SOCK_RAW constant */
    PJ_DECL(int) pj_SOCK_RAW(void);
    /** Get #PJ_SOCK_RDM constant */
    PJ_DECL(int) pj_SOCK_RDM(void);
#else
    /** 得到 TCP 常量值 */
#   define pj_SOCK_STREAM() PJ_SOCK_STREAM
    /** 得到 UDP 常量值 */
#   define pj_SOCK_DGRAM()  PJ_SOCK_DGRAM
    /** 得到 RAW 常量值 */
#   define pj_SOCK_RAW()    PJ_SOCK_RAW
    /** 得到 RDM 常量值 */
#   define pj_SOCK_RDM()    PJ_SOCK_RDM
#endif


/**
 * pj_sock_setsockopt() or #pj_sock_getsockopt() 指定 Socket 级别
 */
/** Socket 级别. @see pj_SOL_SOCKET() */
extern const pj_uint16_t PJ_SOL_SOCKET;
/** IP 级别. @see pj_SOL_IP() */
extern const pj_uint16_t PJ_SOL_IP;
/** TCP 级别. @see pj_SOL_TCP() */
extern const pj_uint16_t PJ_SOL_TCP;
/** UDP 级别. @see pj_SOL_UDP() */
extern const pj_uint16_t PJ_SOL_UDP;
/** IPv6. @see pj_SOL_IPV6() */
extern const pj_uint16_t PJ_SOL_IPV6;

/**
 * 各种常量的访问器函数。提供这些函数是因为Symbian不允许从DLL导出全局变量。
 */

#if defined(PJ_DLL)
    /** Get #PJ_SOL_SOCKET constant */
    PJ_DECL(pj_uint16_t) pj_SOL_SOCKET(void);
    /** Get #PJ_SOL_IP constant */
    PJ_DECL(pj_uint16_t) pj_SOL_IP(void);
    /** Get #PJ_SOL_TCP constant */
    PJ_DECL(pj_uint16_t) pj_SOL_TCP(void);
    /** Get #PJ_SOL_UDP constant */
    PJ_DECL(pj_uint16_t) pj_SOL_UDP(void);
    /** Get #PJ_SOL_IPV6 constant */
    PJ_DECL(pj_uint16_t) pj_SOL_IPV6(void);
#else
    /** Get #PJ_SOL_SOCKET constant */
#   define pj_SOL_SOCKET()  PJ_SOL_SOCKET
    /** Get #PJ_SOL_IP constant */
#   define pj_SOL_IP()	    PJ_SOL_IP
    /** Get #PJ_SOL_TCP constant */
#   define pj_SOL_TCP()	    PJ_SOL_TCP
    /** Get #PJ_SOL_UDP constant */
#   define pj_SOL_UDP()	    PJ_SOL_UDP
    /** Get #PJ_SOL_IPV6 constant */
#   define pj_SOL_IPV6()    PJ_SOL_IPV6
#endif


/* IP_TOS 
 *
 * Note:
 *  TOS 不工作在 Windows 2000 和更高版本
 *  See http://support.microsoft.com/kb/248611
 */
/** IP_TOS 选项名称 in setsockopt()
 *  @see pj_IP_TOS() */
extern const pj_uint16_t PJ_IP_TOS;

/*
 * IP TOS 相关常量
 *
 * Note:
 *  TOS 不工作在 Windows 2000 和更高版本
 *  See http://support.microsoft.com/kb/248611
 */
/** 最小延时. @see pj_IPTOS_LOWDELAY() */
extern const pj_uint16_t PJ_IPTOS_LOWDELAY;

/** 最优吞吐量 @see pj_IPTOS_THROUGHPUT() */
extern const pj_uint16_t PJ_IPTOS_THROUGHPUT;

/** 最优可达. @see pj_IPTOS_RELIABILITY() */
extern const pj_uint16_t PJ_IPTOS_RELIABILITY;

/** “填充数据”传输速度慢并不重要
 *  @see pj_IPTOS_MINCOST() */
extern const pj_uint16_t PJ_IPTOS_MINCOST;


#if defined(PJ_DLL)
    /** Get #PJ_IP_TOS constant */
    PJ_DECL(int) pj_IP_TOS(void);

    /** Get #PJ_IPTOS_LOWDELAY constant */
    PJ_DECL(int) pj_IPTOS_LOWDELAY(void);

    /** Get #PJ_IPTOS_THROUGHPUT constant */
    PJ_DECL(int) pj_IPTOS_THROUGHPUT(void);

    /** Get #PJ_IPTOS_RELIABILITY constant */
    PJ_DECL(int) pj_IPTOS_RELIABILITY(void);

    /** Get #PJ_IPTOS_MINCOST constant */
    PJ_DECL(int) pj_IPTOS_MINCOST(void);
#else
    /** 获取 PJ_IP_TOS 常量 */
#   define pj_IP_TOS()		PJ_IP_TOS

    /** 获取 PJ_IPTOS_LOWDELAY 常量 */
#   define pj_IPTOS_LOWDELAY()	PJ_IP_TOS_LOWDELAY

    /** 获取 PJ_IPTOS_THROUGHPUT 常量 */
#   define pj_IPTOS_THROUGHPUT() PJ_IP_TOS_THROUGHPUT

    /** 获取 PJ_IPTOS_RELIABILITY 常量 */
#   define pj_IPTOS_RELIABILITY() PJ_IP_TOS_RELIABILITY

    /** 获取 PJ_IPTOS_MINCOST 常量 */
#   define pj_IPTOS_MINCOST()	PJ_IP_TOS_MINCOST
#endif


/** IPV6_TCLASS optname in setsockopt(). @see pj_IPV6_TCLASS() */
extern const pj_uint16_t PJ_IPV6_TCLASS;


#if defined(PJ_DLL)
    /** Get #PJ_IPV6_TCLASS constant */
    PJ_DECL(int) pj_IPV6_TCLASS(void);
#else
    /** Get #PJ_IPV6_TCLASS constant */
#   define pj_IPV6_TCLASS()	PJ_IPV6_TCLASS
#endif


/**
 * pj_sock_setsockopt() pj_sock_getsockopt() 指定选项名称值
 */

/** Socket type. @see pj_SO_TYPE() */
extern const pj_uint16_t PJ_SO_TYPE;

/** 接收buf 大小
 * @see pj_SO_RCVBUF() */
extern const pj_uint16_t PJ_SO_RCVBUF;

/** 发送端buf 大小
 *  @see pj_SO_SNDBUF() */
extern const pj_uint16_t PJ_SO_SNDBUF;

/** 禁用发送聚结的Nagle算法
 * @see pj_TCP_NODELAY */
extern const pj_uint16_t PJ_TCP_NODELAY;

/** 允许socket绑定到已在使用的地址。
 *  @see pj_SO_REUSEADDR */
extern const pj_uint16_t PJ_SO_REUSEADDR;

/** 不要生成SIGPIPE
 *  @see pj_SO_NOSIGPIPE */
extern const pj_uint16_t PJ_SO_NOSIGPIPE;

/**
 * 为要在socket上发送的所有数据包设置协议定义的优先级。
 */
extern const pj_uint16_t PJ_SO_PRIORITY;

/** IP多播接口
 *  @see pj_IP_MULTICAST_IF() */
extern const pj_uint16_t PJ_IP_MULTICAST_IF;
 
/** IP 多播ttl
 *  @see pj_IP_MULTICAST_TTL() */
extern const pj_uint16_t PJ_IP_MULTICAST_TTL;

/** IP 多播回环
 *  @see pj_IP_MULTICAST_LOOP() */
extern const pj_uint16_t PJ_IP_MULTICAST_LOOP;

/** 添加IP组成员身份
 *  @see pj_IP_ADD_MEMBERSHIP() */
extern const pj_uint16_t PJ_IP_ADD_MEMBERSHIP;

/** 删除IP组成员身份
 *  @see pj_IP_DROP_MEMBERSHIP() */
extern const pj_uint16_t PJ_IP_DROP_MEMBERSHIP;


#if defined(PJ_DLL)
    /** Get #PJ_SO_TYPE constant */
    PJ_DECL(pj_uint16_t) pj_SO_TYPE(void);

    /** Get #PJ_SO_RCVBUF constant */
    PJ_DECL(pj_uint16_t) pj_SO_RCVBUF(void);

    /** Get #PJ_SO_SNDBUF constant */
    PJ_DECL(pj_uint16_t) pj_SO_SNDBUF(void);

    /** Get #PJ_TCP_NODELAY constant */
    PJ_DECL(pj_uint16_t) pj_TCP_NODELAY(void);

    /** Get #PJ_SO_REUSEADDR constant */
    PJ_DECL(pj_uint16_t) pj_SO_REUSEADDR(void);

    /** Get #PJ_SO_NOSIGPIPE constant */
    PJ_DECL(pj_uint16_t) pj_SO_NOSIGPIPE(void);

    /** Get #PJ_SO_PRIORITY constant */
    PJ_DECL(pj_uint16_t) pj_SO_PRIORITY(void);

    /** Get #PJ_IP_MULTICAST_IF constant */
    PJ_DECL(pj_uint16_t) pj_IP_MULTICAST_IF(void);

    /** Get #PJ_IP_MULTICAST_TTL constant */
    PJ_DECL(pj_uint16_t) pj_IP_MULTICAST_TTL(void);

    /** Get #PJ_IP_MULTICAST_LOOP constant */
    PJ_DECL(pj_uint16_t) pj_IP_MULTICAST_LOOP(void);

    /** Get #PJ_IP_ADD_MEMBERSHIP constant */
    PJ_DECL(pj_uint16_t) pj_IP_ADD_MEMBERSHIP(void);

    /** Get #PJ_IP_DROP_MEMBERSHIP constant */
    PJ_DECL(pj_uint16_t) pj_IP_DROP_MEMBERSHIP(void);
#else
    /** 获取socket type常量 */
#   define pj_SO_TYPE()	    PJ_SO_TYPE

    /** 获取接收buf大小的常量 */
#   define pj_SO_RCVBUF()   PJ_SO_RCVBUF

    /** 获取发送端buf大小的常量*/
#   define pj_SO_SNDBUF()   PJ_SO_SNDBUF

    /** Get #PJ_TCP_NODELAY constant */
#   define pj_TCP_NODELAY() PJ_TCP_NODELAY

    /** Get #PJ_SO_REUSEADDR constant */
#   define pj_SO_REUSEADDR() PJ_SO_REUSEADDR

    /** Get #PJ_SO_NOSIGPIPE constant */
#   define pj_SO_NOSIGPIPE() PJ_SO_NOSIGPIPE

    /** Get #PJ_SO_PRIORITY constant */
#   define pj_SO_PRIORITY() PJ_SO_PRIORITY

    /** Get #PJ_IP_MULTICAST_IF constant */
#   define pj_IP_MULTICAST_IF()    PJ_IP_MULTICAST_IF

    /** Get #PJ_IP_MULTICAST_TTL constant */
#   define pj_IP_MULTICAST_TTL()   PJ_IP_MULTICAST_TTL

    /** Get #PJ_IP_MULTICAST_LOOP constant */
#   define pj_IP_MULTICAST_LOOP()  PJ_IP_MULTICAST_LOOP

    /** Get #PJ_IP_ADD_MEMBERSHIP constant */
#   define pj_IP_ADD_MEMBERSHIP()  PJ_IP_ADD_MEMBERSHIP

    /** Get #PJ_IP_DROP_MEMBERSHIP constant */
#   define pj_IP_DROP_MEMBERSHIP() PJ_IP_DROP_MEMBERSHIP
#endif


/*
 * pj_sock_recv, pj_sock_send 指定的标识
 */

/** 带外信息
 *  @see pj_MSG_OOB() */
extern const int PJ_MSG_OOB;

/** 获取，不要从缓冲器中取出
 *  @see pj_MSG_PEEK() */
extern const int PJ_MSG_PEEK;

/** 不路由
 *  @see pj_MSG_DONTROUTE() */
extern const int PJ_MSG_DONTROUTE;


#if defined(PJ_DLL)
    /** Get #PJ_MSG_OOB constant */
    PJ_DECL(int) pj_MSG_OOB(void);

    /** Get #PJ_MSG_PEEK constant */
    PJ_DECL(int) pj_MSG_PEEK(void);

    /** Get #PJ_MSG_DONTROUTE constant */
    PJ_DECL(int) pj_MSG_DONTROUTE(void);
#else
    /** Get #PJ_MSG_OOB constant */
#   define pj_MSG_OOB()		PJ_MSG_OOB

    /** Get #PJ_MSG_PEEK constant */
#   define pj_MSG_PEEK()	PJ_MSG_PEEK

    /** Get #PJ_MSG_DONTROUTE constant */
#   define pj_MSG_DONTROUTE()	PJ_MSG_DONTROUTE
#endif


/**
 * pj_sock_shutdown() 指定的标识
 */
typedef enum pj_socket_sd_type
{
    PJ_SD_RECEIVE   = 0,    /**< 不再接收	    */
    PJ_SHUT_RD	    = 0,    /**< 不再接收别名	    */
    PJ_SD_SEND	    = 1,    /**< 不再发送	    */
    PJ_SHUT_WR	    = 1,    /**< 不再发送别名	    */
    PJ_SD_BOTH	    = 2,    /**< 不再发送和接收  */
    PJ_SHUT_RDWR    = 2     /**< 不再发送和接收别名	    */
} pj_socket_sd_type;



/** 地址接收任何传入的消息*/
#define PJ_INADDR_ANY	    ((pj_uint32_t)0)

/** 表示错误返回的地址 */
#define PJ_INADDR_NONE	    ((pj_uint32_t)0xffffffff)

/** 发送到所有主机的地址 */
#define PJ_INADDR_BROADCAST ((pj_uint32_t)0xffffffff)


/**
 * 由 pj_sock_listen（）指定的最大长度。如果生成系统不重写此值，则将使用最小分母（在Win32系统中为5）。
 */
#if !defined(PJ_SOMAXCONN)
#  define PJ_SOMAXCONN	5
#endif


/**
 * 返回无效的 socket pj_sock_socket() and pj_sock_accept()
 */
#define PJ_INVALID_SOCKET   (-1)

/* 取消定义 s_addr 因为 pj_in_addr 在下面 */
#undef s_addr

/**
 * Internet address 描述
 */
typedef struct pj_in_addr
{
    pj_uint32_t	s_addr;		/**< 32位的IP address.	    */
} pj_in_addr;


/**
 * IPv4地址的最大文本表示长度
 */
#define PJ_INET_ADDRSTRLEN	16

/**
 * IPv6地址的最大文本表示长度
 */
#define PJ_INET6_ADDRSTRLEN	46

/**
 * pj_sockaddr_in 中的 sin_zero 字段的大小。大多数操作系统使用8，但其他操作系统（如eCos中的bsdtcp/IP堆栈）使用24。
 */
#ifndef PJ_SOCKADDR_IN_SIN_ZERO_LEN
#   define PJ_SOCKADDR_IN_SIN_ZERO_LEN	8
#endif

/**
 *
 * 此结构描述Internet套接字地址。如果 PJ_SOCKADDR_HAS_LEN 不是零，那么 sin_zero_len 成员将添加到此结构中。就应用程序而言，此成员的值将始终为零。
 * 在内部，PJLIB可以在调用系统 socket API之前修改该值，并在将结构返回到应用程序之前将该值重置为零。
 */
struct pj_sockaddr_in
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sin_zero_len;	/**< Just ignore this.		    */
    pj_uint8_t  sin_family;	/**< Address family.		    */
#else
    pj_uint16_t	sin_family;	/**< 地址族		    */
#endif
    pj_uint16_t	sin_port;	/**< 传输层的端口号   */
    pj_in_addr	sin_addr;	/**< IP 地址		    */
    char	sin_zero[PJ_SOCKADDR_IN_SIN_ZERO_LEN]; /** 填充 */
};


#undef s6_addr

/**
 * IPv6 地址结构描述
 */
typedef union pj_in6_addr
{
    /* This is the main entry */
    pj_uint8_t  s6_addr[16];   /**< 8-bit array */

    /* 用来正确对齐的 */
    pj_uint32_t	u6_addr32[4];

    /*
     * 不要与Winsock2一起使用，因为这会将 pj_sockaddr_in6 对齐到64位边界，Winsock2不像它
     */
#if 0 && defined(PJ_HAS_INT64) && PJ_HAS_INT64!=0 && \
    (!defined(PJ_WIN32) || PJ_WIN32==0)
    pj_int64_t	u6_addr64[2];
#endif

} pj_in6_addr;


/** 初始 pj_in6_addr 的值 */
#define PJ_IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }

/** 初始化 pj_in6_addr 的值 */
#define PJ_IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

/**
 * 此结构描述IPv6套接字地址。
 * 如果 PJ_SOCKADDR_HAS_LEN 不是零，那么 sin_zero_len 成员将添加到此结构中。就应用程序而言，此成员的值将始终为零。
 * 在内部，PJLIB可以在调用系统 socket API之前修改该值，并在将结构返回到应用程序之前将该值重置为零。
 */
typedef struct pj_sockaddr_in6
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sin6_zero_len;	    /**< Just ignore this.	   */
    pj_uint8_t  sin6_family;	    /**< Address family.	   */
#else
    pj_uint16_t	sin6_family;	    /**< 地址族	    */
#endif
    pj_uint16_t	sin6_port;	    /**< 传输层端口 */
    pj_uint32_t	sin6_flowinfo;	    /**< IPv6 跟随信息	    */
    pj_in6_addr sin6_addr;	    /**< IPv6 地址	    */
    pj_uint32_t sin6_scope_id;	    /**< 作用域的接口	*/
} pj_sockaddr_in6;


/**
 * 此结构描述传输地址中的常见属性。
 * 如果PJ_SOCKADDR_HAS_LEN 不为零，则将 sa_zero_len 成员添加到此结构中。就应用程序而言，此成员的值将始终为零。
 * 在内部，PJLIB 可以在调用系统 socket API之前修改该值，并在将结构返回到应用程序之前将该值重置为零。
 */
typedef struct pj_addr_hdr
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sa_zero_len;
    pj_uint8_t  sa_family;
#else
    pj_uint16_t	sa_family;	/**< Common data: address family.   */
#endif
} pj_addr_hdr;


/**
 * 此联合体描述通用socket地址
 */
typedef union pj_sockaddr
{
    pj_addr_hdr	    addr;	/**< 通用传输地址	    */
    pj_sockaddr_in  ipv4;	/**< IPv4传输地址    */
    pj_sockaddr_in6 ipv6;	/**< IPv6传输地址    */
} pj_sockaddr;


/**
 * 此结构提供IPv4地址的多播组信息
 */
typedef struct pj_ip_mreq {
    pj_in_addr imr_multiaddr;	/**< IP多播地址组 */
    pj_in_addr imr_interface;	/**< 接口的本地IP地址 */
} pj_ip_mreq;


/**
 * socket 设置的选项
 */
typedef struct pj_sockopt_params
{
    /* 要应用的选项数量 */
    unsigned cnt;

    /* 应用的选项组 */
    struct {
	/* 选项级别*/
	int level;

	/* 选项名称 */
	int optname;

	/* 指向指定选项的缓冲区的指针 */
	void *optval;

	/* optval 指向的缓冲区的缓冲区大小 */
	int optlen;
    } options[PJ_MAX_SOCKOPT_PARAMS];
} pj_sockopt_params;

/*****************************************************************************
 *
 * socket 地址操作
 *
 *****************************************************************************
 */

/**
 * 将16位值从网络字节顺序转换为主机字节顺序
 * @param netshort  16-bit network value.
 * @return	    16-bit host value.
 */
PJ_DECL(pj_uint16_t) pj_ntohs(pj_uint16_t netshort);

/**
 * 将16位值从主机字节顺序转换为网络字节顺序。
 *
 * @param hostshort 16-bit host value.
 * @return	    16-bit network value.
 */
PJ_DECL(pj_uint16_t) pj_htons(pj_uint16_t hostshort);

/**
 * 将32位值从网络字节顺序转换为主机字节顺序
 *
 * @param netlong   32-bit network value.
 * @return	    32-bit host value.
 */
PJ_DECL(pj_uint32_t) pj_ntohl(pj_uint32_t netlong);

/**
 * 将32位值从主机字节顺序转换为网络字节顺序
 *
 * @param hostlong  32-bit host value.
 * @return	    32-bit network value.
 */
PJ_DECL(pj_uint32_t) pj_htonl(pj_uint32_t hostlong);

/**
 * 将以网络字节顺序给出的Internet主机地址转换为点分十进制的字符串
 *
 * @param inaddr    The host address.
 * @return	    The string address.
 */
PJ_DECL(char*) pj_inet_ntoa(pj_in_addr inaddr);

/**
 * 此函数将Internet主机地址 cp 从点分十进制转换为网络字节顺序地址，并将其存储在 inp 指向的结构中
 *
 * @param cp	IP address in standard numbers-and-dots notation.
 * @param inp	Structure that holds the output of the conversion.
 *
 * @return	nonzero if the address is valid, zero if not.
 */
PJ_DECL(int) pj_inet_aton(const pj_str_t *cp, struct pj_in_addr *inp);

/**
 * 此函数用于将标准文本表示形式的地址转换为数字二进制形式。它支持IPv4和IPv6地址转换
 *
 * @param af	指定地址族.  PJ_AF_INET 或 PJ_AF_INET6
 * @param src	传入的字符串
 * @param dst	指向函数存储数字地址的缓冲区；该缓冲区应足够大，以容纳数字地址（PJ_AF_INET为32位，PJ_AF_INET6为128位）。
 *
 * @return	PJ_SUCCESS 转化成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_inet_pton(int af, const pj_str_t *src, void *dst);

/**
 * 此函数用于将数字地址转换为适合显示的文本字符串。它支持IPv4和IPv6地址转换
 *
 * @see pj_sockaddr_print()
 *
 * @param af	指定地址的族. PJ_AF_INET 或 PJ_AF_INET6
 * @param src	如果 af 参数是PJ_AF_INET，则指向保存IPv4地址的缓冲区；如果af参数是PJ_AF_INET6，则指向保存IPv6地址的缓冲区；地址必须按网络字节顺序排列
 * @param dst	指向函数存储结果文本字符串的缓冲区；不能为空
 * @param size	指定此缓冲区的大小，其大小应足以容纳文本字符串（IPv4的PJ_INET_ADDRSTRLEN 字符，IPv6的PJ_INET6_ADDRSTRLEN字符）
 *
 * @return	转换成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_inet_ntop(int af, const void *src,
				  char *dst, int size);

/**
 * 将数字地址转换为其文本字符串表示形式
 * @see pj_sockaddr_print()
 *
 * @param af	指定地址的族. PJ_AF_INET 或 PJ_AF_INET6
 * @param src	如果 af 参数是PJ_AF_INET，则指向保存IPv4地址的缓冲区；如果af参数是PJ_AF_INET6，则指向保存IPv6地址的缓冲区；地址必须按网络字节顺序排列
 * @param dst	指向函数存储结果文本字符串的缓冲区；它不应为空
 * @param size	指定此缓冲区的大小，其大小应足以容纳文本字符串（IPv4的PJ_INET_ADDRSTRLEN 字符，IPv6的PJ_INET6_ADDRSTRLEN字符）
 *
 * @return	成功返回地址字符串，失败返回 NULL
 */
PJ_DECL(char*) pj_inet_ntop2(int af, const void *src,
			     char *dst, int size);

/**
 * 打印socket 地址
 *
 * @param addr	socket 地址
 * @param buf	文本缓存
 * @param size	缓存大小
 * @param 标记这些值的位掩码组合：
 *		  	1:包括端口号。
 *		  	2:IPv6地址包含方括号
 *
 * @return	地址字符
 */
PJ_DECL(char*) pj_sockaddr_print(const pj_sockaddr_t *addr,
				 char *buf, int size,
				 unsigned flags);

/**
 * 将点分十进制字符串转换为二进制IP地址
 * 
 * @param cp	    点分十进制表示的地址
 * @return	    如果成功，则按网络字节顺序返回IP地址。如果失败，将返回PJ_INADDR_NONE
 * @remark 	这是 pj_inet_aton() 的过时接口；它是过时的，因为 -1是一个有效地址（255.255.255.255），而 pj_inet_aton()提供了一种更清晰的方法来指示错误返回
 */
PJ_DECL(pj_in_addr) pj_inet_addr(const pj_str_t *cp);

/**
 * 将点分十进制字符串转换为二进制 IP地址
 * 
 * @param cp	    点分十进制字符串
 * @return	    如果成功，则按网络字节顺序返回IP地址。如果失败，将返回PJ_INADDR_NONE
 * @remark
 * 	这是 pj_inet_aton() 的过时接口；它是过时的，因为 -1是一个有效地址（255.255.255.255），而 pj_inet_aton() 提供了一种更清晰的方法来指示错误返回
 */
PJ_DECL(pj_in_addr) pj_inet_addr2(const char *cp);

/**
 * 基于地址和端口信息初始化IPv4 socket地址
 * 字符串地址可以是点分十进制，也可以是主机名。如果指定了 hostname，那么函数将把主机解析为IP地址。
 *
 * @see pj_sockaddr_init()
 *
 * @param addr	    IP socket 地址
 * @param cp	    地址字符串，可以是点分十进制或要解析的主机名
 * @param port	    端口号，按主机字节顺序
 *
 * @return	    成功返回 0
 */
PJ_DECL(pj_status_t) pj_sockaddr_in_init( pj_sockaddr_in *addr,
				          const pj_str_t *cp,
					  pj_uint16_t port);

/**
 * 根据address和port初始化IP socket 地址。
 * 字符串地址可以是点分十进制表示法，也可以是主机名。如果指定了hostname，那么函数将把主机解析为IP地址。
 *
 * @see pj_sockaddr_in_init()
 *
 * @param af	    地址族
 * @param addr	    ip 地址
 * @param cp	    地址字符串，可以是点分十进制或要解析的主机名
 * @param port	    端口号，主机序列
 *
 * @return	    成功返回 0
 */
PJ_DECL(pj_status_t) pj_sockaddr_init(int af, 
				      pj_sockaddr *addr,
				      const pj_str_t *cp,
				      pj_uint16_t port);

/**
 * 比较两 socket 地址
 *
 * @param addr1	    第一个地址
 * @param addr2	    第二个地址
 *
 * @return
 * 		0 ： 相等
 * 		-1： addr1 < addr2
 * 		+1:  addr1 > addr2
 */
PJ_DECL(int) pj_sockaddr_cmp(const pj_sockaddr_t *addr1,
			     const pj_sockaddr_t *addr2);

/**
 * 获取指向socket地址的部分指针
 * 
 * @param addr	    socket 地址
 *
 * @return	    指向地址部分的指针（sin_addr或sin6_addr，取决于地址族）
 */
PJ_DECL(void*) pj_sockaddr_get_addr(const pj_sockaddr_t *addr);

/**
 * 检查套接字地址是否包含非零地址
 *
 * @param addr	    Socket address.
 *
 * @return	    如果地址设置为非零，则为非零
 */
PJ_DECL(pj_bool_t) pj_sockaddr_has_addr(const pj_sockaddr_t *addr);

/**
 * 根据地址族获取套接字地址的地址长度。对于PJ_AF_INET，长度将是sizeof(pj_in_addr)，对于PJ_AF_INET6，长度将是sizeof(pj_in6_addr)
 * 
 * @param addr	    Socket 地址
 *
 * @return	    字节数
 */
PJ_DECL(unsigned) pj_sockaddr_get_addr_len(const pj_sockaddr_t *addr);

/**
 * 根据其地址族获取socket地址长度。对于PJ_AF_INET，长度将是sizeof（pj_sockaddr_in），对于PJ_AF_INET6，长度将是sizeof（pj_sockaddr_in6）。
 * 
 * @param addr	    Socket address.
 *
 * @return	    字节长度
 */
PJ_DECL(unsigned) pj_sockaddr_get_len(const pj_sockaddr_t *addr);

/** 
 * 只复制 socket 地址的地址部分 (sin_addr/sin6_addr)
 *
 * @param dst	    socket 目的地址
 * @param src	    socket 源地址
 *
 * @see @pj_sockaddr_cp()
 */
PJ_DECL(void) pj_sockaddr_copy_addr(pj_sockaddr *dst,
				    const pj_sockaddr *src);
/**
 * 复制 socket 地址,这将根据 源socket地址的地址族复制整个结构
 *
 * @param dst	    socket 目的地址
 * @param src	    socket 源地址
 *
 * @see @pj_sockaddr_copy_addr()
 */
PJ_DECL(void) pj_sockaddr_cp(pj_sockaddr_t *dst, const pj_sockaddr_t *src);

/*
 * 如果源地址和所需的地址族匹配，则复制该地址，否则从源地址合成一个具有所需地址族的新地址。
 * 这对于生成IPv4映射的IPv6地址非常有用。
 *
 * @param dst_af    想要的地址族
 * @param dst	    socket 目的地址，如果需要合成且失败，则无效
 * @param src	    socket 源地址
 *
 * @return	    合成成功则返回 PJ_SUCCESS；如果合成失败，则返回错误状态
 */
PJ_DECL(pj_status_t) pj_sockaddr_synthesize(int dst_af,
				            pj_sockaddr_t *dst,
				            const pj_sockaddr_t *src);

/**
 * 获取IPv4 socket 地址的IP地址
 * 地址以主机字节顺序的32位值返回
 *
 * @param addr	    IP 地址
 * @return	    32位地址，主机序列
 */
PJ_DECL(pj_in_addr) pj_sockaddr_in_get_addr(const pj_sockaddr_in *addr);

/**
 * 设置IPv4 socket 地址的IP地址
 *
 * @param addr	    IP socket 地址
 * @param hostaddr  主机地址，主机序列
 */
PJ_DECL(void) pj_sockaddr_in_set_addr(pj_sockaddr_in *addr,
				      pj_uint32_t hostaddr);

/**
 * 从字符串地址设置IP套接字地址的IP地址，必要时解析主机。字符串地址可以是点分十进制表示法，也可以是主机名。
 * 如果指定了hostname，那么函数将把主机解析为IP地址。
 *
 * @see pj_sockaddr_set_str_addr()
 *
 * @param addr	    设置的 IP socket address
 * @param cp	    地址字符串，点分十进制或是 hostname
 *
 * @return	    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pj_sockaddr_in_set_str_addr( pj_sockaddr_in *addr,
					          const pj_str_t *cp);

/**
 * 从字符串地址设置 IPv4或IPv6套接字地址的IP地址，必要时解析主机。字符串地址可以是标准IPv6或IPv6地址，也可以是主机名。
 * 如果指定了hostname，那么函数将根据地址族将主机解析为IP地址。
 *
 * @param af	    地址族
 * @param addr	    设置 IP socket address
 * @param cp	    地址字符串，可以是标准IP号码（IPv4或IPv6）或要解析的主机名
 *
 * @return	    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pj_sockaddr_set_str_addr(int af,
					      pj_sockaddr *addr,
					      const pj_str_t *cp);

/**
 * 以主机字节顺序获取 socket地址的端口号
 * 此函数可用于IPv4和IPv6套接字地址。
 * 
 * @param addr	    Socket address.
 *
 * @return	    端口号，主机序列
 */
PJ_DECL(pj_uint16_t) pj_sockaddr_get_port(const pj_sockaddr_t *addr);

/**
 * 根据socket 地址获取传输层的端口号，主机序列
 *
 * @param addr	    IP socket address.
 * @return	    端口号，主机序列
 */
PJ_DECL(pj_uint16_t) pj_sockaddr_in_get_port(const pj_sockaddr_in *addr);

/**
 * 设置一个IP 地址的端口号，主机序列
 *
 * @param addr	    IP 地址
 * @param hostport  端口号，主机序列
 */
PJ_DECL(pj_status_t) pj_sockaddr_set_port(pj_sockaddr *addr, 
					  pj_uint16_t hostport);

/**
 * 设置一个 IPv4 socket 地址的端口号
 *
 * @see pj_sockaddr_set_port()
 *
 * @param addr	    IP socket 地址
 * @param hostport  端口号，主机序列
 */
PJ_DECL(void) pj_sockaddr_in_set_port(pj_sockaddr_in *addr, 
				      pj_uint16_t hostport);

/**
 * 将包含IP地址和可选端口的字符串解析为套接字地址，可能还包含地址族检测。此函数同时支持IPv4和IPv6解析，
 * 但是只有在编译期间启用IPv6时才能执行IPv6解析。
 *
 * 此函数支持解析多种格式。
 * IPv4输入及其默认结果示例：
 *  - "10.0.0.1:80": 地址 10.0.0.1  端口 80
 *  - "10.0.0.1": 地址 10.0.0.1 端口 0
 *  - "10.0.0.1:": 地址 10.0.0.1 端口 0
 *  - "10.0.0.1:0": 地址 10.0.0.1 端口 0
 *  - ":80": 地址 0.0.0.0 端口 80
 *  - ":": 地址 0.0.0.0 端口 0
 *  - "localhost": 地址 127.0.0.1 端口 0
 *  - "localhost:": 地址 127.0.0.1 端口 0
 *  - "localhost:80": 地址 127.0.0.1 端口 80
 *
 * IPv6输入及其默认结果示例
 *  - "[fec0::01]:80": 地址 fec0::01 端口 80
 *  - "[fec0::01]": 地址 fec0::01 端口 0
 *  - "[fec0::01]:": 地址 fec0::01 端口 0
 *  - "[fec0::01]:0": 地址 fec0::01 端口 0
 *  - "fec0::01": 地址 fec0::01 端口 0
 *  - "fec0::01:80": 地址 fec0::01:80 端口 0
 *  - "::": 地址 0 (::) 端口 0
 *  - "[::]": 地址 0 (::) 端口 0
 *  - "[::]:": 地址 0 (::) 端口 0
 *  - ":::": 地址 0 (::) 端口 0
 *  - "[::]:80": 地址 0 (::) 端口 0
 *  - ":::80": 地址 0 (::) 端口 80
 *
 * 注意：当IPv6套接字地址包含端口号时，套接字地址的IP部分应使用方括号括起来，
 * 否则端口号将作为IP地址的一部分包含在内（请参见上面的“fec0:：01:80”示例）。
 *
 * @param af	    （可选）指定要使用的地址族。如果要从输入中扣除地址族，请在此处指定 pj_AF_UNSPEC()。
 * 其他支持的值有pj_AF_INET()和pj_AF_INET6()
 *
 * @param options   辅助解析的其他选项，现在必须为零。
 * @param str	    输入字符串
 * @param addr	    存储结果的指针
 *
 * @return	    解析成功返回 PJ_SUCCESS
 *
 * @see pj_sockaddr_parse2()
 */
PJ_DECL(pj_status_t) pj_sockaddr_parse(int af, unsigned options,
				       const pj_str_t *str,
				       pj_sockaddr *addr);

/**
 * 此函数类似于 pj_sockaddr_parse()，只是它不会将 hostpart 转换为IP地址（因此可能会将主机名解析为 pj_sockaddr）
 *
 * 与 pj_sockaddr_parse()不同，此函数有一个限制，即如果在IPv6输入字符串中指定了端口号，则 IPv6套接字地址的IP部分必须括在方括号中，
 * 否则端口号将被视为IPv6 IP地址的一部分。
 *
 * @param af	（可选）指定要使用的地址族。不指定地址族，此处指定 pj_AF_UNSPEC()。其他支持的值有 pj_AF_INET()和 pj_AF_INET6()
 * @param options   辅助解析的其他选项，现在必须为零。
 * @param str	    要分析的输入字符串
 * @param hostpart  用于存储套接字地址的主机部分的可选指针，在删除任何括号下
 * @param port	    用于存储端口号的可选指针。如果找不到端口号，返回时将设置为零
 * @param raf	    用于存储输入地址的地址族的可选指针
 *
 * @return	    解析成功则返回 PJ_SUCCESS
 *
 * @see pj_sockaddr_parse()
 */
PJ_DECL(pj_status_t) pj_sockaddr_parse2(int af, unsigned options,
				        const pj_str_t *str,
				        pj_str_t *hostpart,
				        pj_uint16_t *port,
					int *raf);

/*****************************************************************************
 *
 * 主机名和地址
 *
 *****************************************************************************
 */

/**
 * 获取系统的主机名
 *
 * @return	    主机名，如果没有被定义则返回空字符串
 */
PJ_DECL(const pj_str_t*) pj_gethostname(void);

/**
 * 获取主机的IP地址，它是从主机名解析的第一个IP地址
 *
 * @return	    主机的IP 地址，如果主机IP 地址没有被定义则返回 PJ_INADDR_NONE
 */
PJ_DECL(pj_in_addr) pj_gethostaddr(void);


/*****************************************************************************
 *
 * SOCKET API.
 *
 *****************************************************************************
 */

/**
 * 创建一个新的通信 socket
 *
 * @param family    协议族
 * @param type	    指定类型
 * @param protocol  指定特定协议。通常只有一个协议支持给定协议族中的特定套接字类型，其中case协议可以指定为0
 * @param sock	    新的套接字描述符，错误为 PJ_INVALID_SOCKET
 *
 * @return	    成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_socket(int family, 
				    int type, 
				    int protocol,
				    pj_sock_t *sock);

/**
 * 关闭socket
 *
 * @param sockfd    socket 句柄
 *
 * @return	    成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_close(pj_sock_t sockfd);


/**
 * 此函数为套接字 sockfd 提供本地地址 my_addr,my_addr是addrlen字节长。传统上，这称为为套接字指定名称。使用 pj_sock_socket() 创建套接字时，
 * 它存在于名称空间（地址族）中，但没有指定名称。
 *
 * @param sockfd    socket 句柄
 * @param my_addr   本地地址
 * @param addrlen   地址长度
 *
 * @return	    成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_bind( pj_sock_t sockfd, 
				   const pj_sockaddr_t *my_addr,
				   int addrlen);

/**
 * 将IP套接字 sockfd 绑定到给定的地址和端口。
 *
 * @param sockfd    socket 句柄
 * @param addr	    本地地址，主机序列
 * @param port	    本地端口，主机序列
 *
 * @return	    成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_bind_in( pj_sock_t sockfd, 
				      pj_uint32_t addr,
				      pj_uint16_t port);

/**
 * 将IP套接字 sockfd 绑定到指定的地址和指定范围内的随机端口。
 *
 * @param sockfd    	socket 句柄
 * @param addr      	本地地址和端口
 * @param port_range	端口范围，相对于在地址的端口字段中指定的起始端口号。请注意，如果端口为零，则将忽略此参数
 * @param max_try   	最多尝试
 *
 * @return	    	成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_bind_random( pj_sock_t sockfd,
				          const pj_sockaddr_t *addr,
				          pj_uint16_t port_range,
				          pj_uint16_t max_try);

#if PJ_HAS_TCP
/**
 * 监听连接，此函数仅适用于TCP(PJ_SOCK_STREAM 或 PJ_SOCK_SEQPACKET) 表示接受传入连接的意愿。
 *
 * @param sockfd	socket 句柄
 * @param backlog	定义挂起连接队列可能增长到的最大长度。
 *
 * @return		成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_listen( pj_sock_t sockfd, 
				     int backlog );

/**
 * 服务端，接受新连接
 *
 * @param serverfd  服务器 socket
 * @param newsock   成功时创建新套接字，失败时返回 PJ_INVALID_SOCKET
 * @param addr	    指向 sockaddr 。如果参数不为空，则由连接实体的地址填充
 * @param addrlen   最初指定地址的长度，返回时将填入确切的地址长度
 *
 * @return	    成功返回 0，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_sock_accept( pj_sock_t serverfd,
				     pj_sock_t *newsock,
				     pj_sockaddr_t *addr,
				     int *addrlen);
#endif

/**
 * 文件描述符 sockfd 必须引用套接字。如果套接字是 PJ_SOCK_DGRAM 类型，那么 serv_addr 是默认情况下数据报发送到的地址，也是接收数据报的唯一地址。
 * 如果套接字的类型为 PJ_SOCK_STREAM 或 PJ_SOCK_SEQPACKET，则此调用将尝试连接到另一个套接字。另一个套接字由 serv_addr 指定，它是套接字通信空
 * 间中的地址（addrlen长度）。每个通信空间都以自己的方式解释 serv_addr 参数。
 *
 * @param sockfd	socket 句柄
 * @param serv_addr	服务端地址
 * @param addrlen	服务端地址长度
 *
 * @return		成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_connect( pj_sock_t sockfd,
				      const pj_sockaddr_t *serv_addr,
				      int addrlen);

/**
 * 返回连接到 socket sockfd 的对等方的地址
 *
 * @param sockfd	socket 句柄
 * @param addr		指向将返回地址的 sockaddr 结构的指针
 * @param namelen	最初是地址的长度。返回时，该值将设置为地址的实际长度。
 *
 * @return		成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_getpeername(pj_sock_t sockfd,
					  pj_sockaddr_t *addr,
					  int *namelen);

/**
 * 返回指定套接字的当前名称
 *
 * @param sockfd	socket 句柄
 * @param addr		指向将返回地址的 sockaddr 结构的指针
 * @param namelen	最初是地址的长度，返回时，该值将设置为地址的实际长度
 *
 * @return		成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_getsockname( pj_sock_t sockfd,
					  pj_sockaddr_t *addr,
					  int *namelen);

/**
 * 获取套接字选项。选项可能存在于多个协议级别；它们始终存在于最上面的套接字级别。
 *
 * @param sockfd	socket 句柄
 * @param level		选项的级别
 * @param optname	选项名称
 * @param optval	选项值
 * @param optlen	最初包含缓冲区的长度，返回时将设置为实际大小的值
 *
 * @return		成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_getsockopt( pj_sock_t sockfd,
					 pj_uint16_t level,
					 pj_uint16_t optname,
					 void *optval,
					 int *optlen);
/**
 * 设置套接字选项。选项可能存在于多个协议级别；它们始终存在于最上面的套接字级别。
 *
 * @param sockfd	socket 句柄
 * @param level		选项级别
 * @param optname	选项名称
 * @param optval	选项值
 * @param optlen	值的长度
 *
 * @return		返回 PJ_SUCCESS 或状态值
 */
PJ_DECL(pj_status_t) pj_sock_setsockopt( pj_sock_t sockfd,
					 pj_uint16_t level,
					 pj_uint16_t optname,
					 const void *optval,
					 int optlen);

/**
 * 设置套接字选项。此方法将应用所有指定的选项，并忽略可能引发的任何错误。
 *
 * @param sockfd	socket 句柄
 * @param params	socket 选项
 *
 * @return		成功返回 PJ_SUCCESS;失败返回最后一个错误码
 */
PJ_DECL(pj_status_t) pj_sock_setsockopt_params( pj_sock_t sockfd,
					       const pj_sockopt_params *params);					       

/**
 * Helper函数，用于使用 pj_sock_setsockopt() 设置套接字缓冲区大小，该函数能够使用较低的缓冲区设置值自动重试，直到成功设置可能的最高值。
 *
 * @param sockfd	socket 句柄
 * @param optname	选项名称，有效值为 pj_SO_RCVBUF() 和 pj_SO_SNDBUF()
 * @param auto_retry	具有更低值时是否自动重试
 * @param buf_size	在输入时，指定首选的缓冲区大小设置；在输出时，指定应用的缓冲区大小设置
 *
 * @return		成功返回 PJ_SUCCESS；失败返回状态码
 */
PJ_DECL(pj_status_t) pj_sock_setsockopt_sobuf( pj_sock_t sockfd,
					       pj_uint16_t optname,
					       pj_bool_t auto_retry,
					       unsigned *buf_size);


/**
 * 接收指定套接字的数据流或消息
 *
 * @param sockfd	socket 句柄
 * @param buf		接收数据或消息的buffer
 * @param len		输入，这个buffer 长度。返回，包含接收数据的长度
 * @param flags		标识 (如 pj_MSG_PEEK()).
 *
 * @return		成功返回 PJ_SUCCESS；失败返回错误码
 */
PJ_DECL(pj_status_t) pj_sock_recv(pj_sock_t sockfd,
				  void *buf,
				  pj_ssize_t *len,
				  unsigned flags);

/**
 * 接收指定套接字的数据流或消息
 *
 * @param sockfd	socket 句柄
 * @param buf		接收数据和消息的buffer
 * @param len		输入，buffer 长度；返回，接收数据的长度
 * @param flags		标识 (如 pj_MSG_PEEK()).
 * @param from		如果不为空，它将用连接的源地址填充
 * @param fromlen	最初包含发件人地址的长度，返回时将填写地址的实际长度
 *
 * @return		成功返回 PJ_SUCCESS；失败返回错误码
 */
PJ_DECL(pj_status_t) pj_sock_recvfrom( pj_sock_t sockfd,
				      void *buf,
				      pj_ssize_t *len,
				      unsigned flags,
				      pj_sockaddr_t *from,
				      int *fromlen);

/**
 * 传输数据到 socket
 *
 * @param sockfd	socket 句柄
 * @param buf		发送数据的buffer
 * @param len		输入，数据的长度；返回时，已发送数据的长度
 * @param flags		标识 (如 pj_MSG_DONTROUTE()).
 *
 * @return		成功时返回 PJ_SUCCESS；失败返回状态码
 */
PJ_DECL(pj_status_t) pj_sock_send(pj_sock_t sockfd,
				  const void *buf,
				  pj_ssize_t *len,
				  unsigned flags);

/**
 * 传输数据到指定的地址
 *
 * @param sockfd	socket 句柄
 * @param buf		发送数据的buffer
 * @param len		输入时，buffer数据的长度；返回时，实际发送的数据长度
 * @param flags		标识 (如 pj_MSG_DONTROUTE()).
 * @param to		发送的地址
 * @param tolen		地址长度
 *
 * @return		成功返回 PJ_SUCCESS；失败返回状态码
 */
PJ_DECL(pj_status_t) pj_sock_sendto(pj_sock_t sockfd,
				    const void *buf,
				    pj_ssize_t *len,
				    unsigned flags,
				    const pj_sockaddr_t *to,
				    int tolen);

#if PJ_HAS_TCP
/**
 * shutdown 调用导致与 sockfd关联的套接字上的全双工连接的全部或部分关闭。
 *
 * @param sockfd	socket 句柄
 * @param how		PJ_SHUT_RD	不在接收
 * 					PJ_SHUT_WR	不在发送
 * 					PJ_SHUT_RDWR	不在接收和发送
 *
 * @return		成功返回 0
 */
PJ_DECL(pj_status_t) pj_sock_shutdown( pj_sock_t sockfd,
				       int how);
#endif

/*****************************************************************************
 *
 * 工具
 *
 *****************************************************************************
 */

/**
 * 打印套接字地址字符串。如果地址字符串是 IPv6地址，则此方法将用方括号括起来。
 *
 * @param host_str  主机地址字符串
 * @param port	    端口
 * @param buf	    文本buffer
 * @param size	    buffer 大小
 * @param flags	    这些值的位掩码组合：
 * 						-1:包括端口号
 *
 * @return	The address string.
 */
PJ_DECL(char *) pj_addr_str_print( const pj_str_t *host_str, int port, 
				   char *buf, int size, unsigned flag);


/**
 * @}
 */


PJ_END_DECL

#endif	/* __PJ_SOCK_H__ */

