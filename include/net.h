/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	LiMon Monitor (LiMon) - Network.
 *
 *	Copyright 1994 - 2000 Neil Russell.
 *	(See License)
 *
 * History
 *	9/16/00	  bor  adapted to TQM823L/STK8xxL board, RARP/TFTP boot added
 */

#ifndef __NET_H__
#define __NET_H__

#include <linux/types.h>
#include <asm/cache.h>
#include <asm/byteorder.h>	/* for nton* / ntoh* stuff */
#include <env.h>
#include <hexdump.h>
#include <log.h>
#include <time.h>
#include <linux/if_ether.h>
#include <rand.h>

struct bd_info;
struct cmd_tbl;
struct udevice;

#define DEBUG_LL_STATE 0	/* Link local state machine changes */
#define DEBUG_DEV_PKT 0		/* Packets or info directed to the device */
#define DEBUG_NET_PKT 0		/* Packets on info on the network at large */
#define DEBUG_INT_STATE 0	/* Internal network state changes */
#define DEBUG_NET_PKT_TRACE 0	/* Trace all packet data */

/*
 *	The number of receive packet buffers, and the required packet buffer
 *	alignment in memory.
 *
 */
#define PKTBUFSRX	CONFIG_SYS_RX_ETH_BUFFER
#define PKTALIGN	ARCH_DMA_MINALIGN

/* Number of packets processed together */
#define ETH_PACKETS_BATCH_RECV	32

/* ARP hardware address length */
#define ARP_HLEN 6
/*
 * The size of a MAC address in string form, each digit requires two chars
 * and five separator characters to form '00:00:00:00:00:00'.
 */
#define ARP_HLEN_ASCII (ARP_HLEN * 2) + (ARP_HLEN - 1)

/* IPv4 addresses are always 32 bits in size */
struct in_addr {
	__be32 s_addr;
};

/**
 * do_tftpb - Run the tftpboot command
 *
 * @cmdtp: Command information for tftpboot
 * @flag: Command flags (CMD_FLAG_...)
 * @argc: Number of arguments
 * @argv: List of arguments
 * Return: result (see enum command_ret_t)
 */
int do_tftpb(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);

/**
 * dhcp_run() - Run DHCP on the current ethernet device
 *
 * This sets the autoload variable, then puts it back to similar to its original
 * state (y, n or unset).
 *
 * @addr: Address to load the file into (0 if @autoload is false)
 * @fname: Filename of file to load (NULL if @autoload is false or to use the
 * default filename)
 * @autoload: true to load the file, false to just get the network IP
 * @return 0 if OK, -EINVAL if the environment failed, -ENOENT if ant file was
 * not found
 */
int dhcp_run(ulong addr, const char *fname, bool autoload);

/**
 * An incoming packet handler.
 * @param pkt    pointer to the application packet
 * @param dport  destination UDP port
 * @param sip    source IP address
 * @param sport  source UDP port
 * @param len    packet length
 */
typedef void rxhand_f(uchar *pkt, unsigned dport,
		      struct in_addr sip, unsigned sport,
		      unsigned len);

/**
 * An incoming ICMP packet handler.
 * @param type	ICMP type
 * @param code	ICMP code
 * @param dport	destination UDP port
 * @param sip	source IP address
 * @param sport	source UDP port
 * @param pkt	pointer to the ICMP packet data
 * @param len	packet length
 */
typedef void rxhand_icmp_f(unsigned type, unsigned code, unsigned dport,
		struct in_addr sip, unsigned sport, uchar *pkt, unsigned len);

/*
 *	A timeout handler.  Called after time interval has expired.
 */
typedef void	thand_f(void);

enum eth_state_t {
	ETH_STATE_INIT,
	ETH_STATE_PASSIVE,
	ETH_STATE_ACTIVE
};

/**
 * struct eth_pdata - Platform data for Ethernet MAC controllers
 *
 * @iobase: The base address of the hardware registers
 * @enetaddr: The Ethernet MAC address that is loaded from EEPROM or env
 * @phy_interface: PHY interface to use - see PHY_INTERFACE_MODE_...
 * @max_speed: Maximum speed of Ethernet connection supported by MAC
 * @priv_pdata: device specific plat
 */
struct eth_pdata {
	phys_addr_t iobase;
	unsigned char enetaddr[ARP_HLEN];
	int phy_interface;
	int max_speed;
	void *priv_pdata;
};

enum eth_recv_flags {
	/*
	 * Check hardware device for new packets (otherwise only return those
	 * which are already in the memory buffer ready to process)
	 */
	ETH_RECV_CHECK_DEVICE		= 1 << 0,
};

/**
 * struct eth_ops - functions of Ethernet MAC controllers
 *
 * start: Prepare the hardware to send and receive packets
 * send: Send the bytes passed in "packet" as a packet on the wire
 * recv: Check if the hardware received a packet. If so, set the pointer to the
 *	 packet buffer in the packetp parameter. If not, return an error or 0 to
 *	 indicate that the hardware receive FIFO is empty. If 0 is returned, the
 *	 network stack will not process the empty packet, but free_pkt() will be
 *	 called if supplied
 * free_pkt: Give the driver an opportunity to manage its packet buffer memory
 *	     when the network stack is finished processing it. This will only be
 *	     called when no error was returned from recv - optional
 * stop: Stop the hardware from looking for packets - may be called even if
 *	 state == PASSIVE
 * mcast: Join or leave a multicast group (for TFTP) - optional
 * write_hwaddr: Write a MAC address to the hardware (used to pass it to Linux
 *		 on some platforms like ARM). This function expects the
 *		 eth_pdata::enetaddr field to be populated. The method can
 *		 return -ENOSYS to indicate that this is not implemented for
		 this hardware - optional.
 * read_rom_hwaddr: Some devices have a backup of the MAC address stored in a
 *		    ROM on the board. This is how the driver should expose it
 *		    to the network stack. This function should fill in the
 *		    eth_pdata::enetaddr field - optional
 * set_promisc: Enable or Disable promiscuous mode
 * get_sset_count: Number of statistics counters
 * get_string: Names of the statistic counters
 * get_stats: The values of the statistic counters
 */
struct eth_ops {
	int (*start)(struct udevice *dev);
	int (*send)(struct udevice *dev, void *packet, int length);
	int (*recv)(struct udevice *dev, int flags, uchar **packetp);
	int (*free_pkt)(struct udevice *dev, uchar *packet, int length);
	void (*stop)(struct udevice *dev);
	int (*mcast)(struct udevice *dev, const u8 *enetaddr, int join);
	int (*write_hwaddr)(struct udevice *dev);
	int (*read_rom_hwaddr)(struct udevice *dev);
	int (*set_promisc)(struct udevice *dev, bool enable);
	int (*get_sset_count)(struct udevice *dev);
	void (*get_strings)(struct udevice *dev, u8 *data);
	void (*get_stats)(struct udevice *dev, u64 *data);
};

#define eth_get_ops(dev) ((struct eth_ops *)(dev)->driver->ops)

struct udevice *eth_get_dev(void); /* get the current device */
/*
 * The devname can be either an exact name given by the driver or device tree
 * or it can be an alias of the form "eth%d"
 */
struct udevice *eth_get_dev_by_name(const char *devname);
unsigned char *eth_get_ethaddr(void); /* get the current device MAC */

/* Used only when NetConsole is enabled */
int eth_is_active(struct udevice *dev); /* Test device for active state */
int eth_init_state_only(void); /* Set active state */
void eth_halt_state_only(void); /* Set passive state */

int eth_initialize(void);		/* Initialize network subsystem */
void eth_try_another(int first_restart);	/* Change the device */
void eth_set_current(void);		/* set nterface to ethcur var */

int eth_get_dev_index(void);		/* get the device index */
void eth_parse_enetaddr(const char *addr, uchar *enetaddr);

/**
 * eth_env_set_enetaddr_by_index() - set the MAC address environment variable
 *
 * This sets up an environment variable with the given MAC address (@enetaddr).
 * The environment variable to be set is defined by <@base_name><@index>addr.
 * If @index is 0 it is omitted. For common Ethernet this means ethaddr,
 * eth1addr, etc.
 *
 * @base_name:  Base name for variable, typically "eth"
 * @index:      Index of interface being updated (>=0)
 * @enetaddr:   Pointer to MAC address to put into the variable
 * Return: 0 if OK, other value on error
 */
int eth_env_set_enetaddr_by_index(const char *base_name, int index,
				  uchar *enetaddr);


/*
 * Initialize USB ethernet device with CONFIG_DM_ETH
 * Returns:
 *	0 is success, non-zero is error status.
 */
int usb_ether_init(void);

/*
 * Get the hardware address for an ethernet interface .
 * Args:
 *	base_name - base name for device (normally "eth")
 *	index - device index number (0 for first)
 *	enetaddr - returns 6 byte hardware address
 * Returns:
 *	Return true if the address is valid.
 */
int eth_env_get_enetaddr_by_index(const char *base_name, int index,
				  uchar *enetaddr);

/*
 * Set the environment variable for an ethernet interface if not already set
 * Args:
 *	base_name - base name for device (normally "eth")
 *	index - device index number (0 for first)
 *	enetaddr - 6 byte hardware address
 * Returns:
 *	Return 0 if the variable could be set
 */
int eth_setenv_enetaddr_by_index(const char *base_name, int index,
				 uchar *enetaddr);

int eth_init(void);			/* Initialize the device */
int eth_send(void *packet, int length);	   /* Send a packet */

#if defined(CONFIG_API) || defined(CONFIG_EFI_LOADER)
int eth_receive(void *packet, int length); /* Receive a packet*/
extern void (*push_packet)(void *packet, int length);
#endif
int eth_rx(void);			/* Check for received packets */
void eth_halt(void);			/* stop SCC */
const char *eth_get_name(void);		/* get name of current device */
int eth_mcast_join(struct in_addr mcast_addr, int join);

/**********************************************************************/
/*
 *	Protocol headers.
 */

/*
 *	Ethernet header
 */

struct ethernet_hdr {
	u8		et_dest[ARP_HLEN];	/* Destination node	*/
	u8		et_src[ARP_HLEN];	/* Source node		*/
	u16		et_protlen;		/* Protocol or length	*/
} __attribute__((packed));

/* Ethernet header size */
#define ETHER_HDR_SIZE	(sizeof(struct ethernet_hdr))

#define ETH_FCS_LEN	4		/* Octets in the FCS		*/

struct e802_hdr {
	u8		et_dest[ARP_HLEN];	/* Destination node	*/
	u8		et_src[ARP_HLEN];	/* Source node		*/
	u16		et_protlen;		/* Protocol or length	*/
	u8		et_dsap;		/* 802 DSAP		*/
	u8		et_ssap;		/* 802 SSAP		*/
	u8		et_ctl;			/* 802 control		*/
	u8		et_snap1;		/* SNAP			*/
	u8		et_snap2;
	u8		et_snap3;
	u16		et_prot;		/* 802 protocol		*/
} __attribute__((packed));

/* 802 + SNAP + ethernet header size */
#define E802_HDR_SIZE	(sizeof(struct e802_hdr))

/*
 *	Virtual LAN Ethernet header
 */
struct vlan_ethernet_hdr {
	u8		vet_dest[ARP_HLEN];	/* Destination node	*/
	u8		vet_src[ARP_HLEN];	/* Source node		*/
	u16		vet_vlan_type;		/* PROT_VLAN		*/
	u16		vet_tag;		/* TAG of VLAN		*/
	u16		vet_type;		/* protocol type	*/
} __attribute__((packed));

/* VLAN Ethernet header size */
#define VLAN_ETHER_HDR_SIZE	(sizeof(struct vlan_ethernet_hdr))

#define PROT_IP		0x0800		/* IP protocol			*/
#define PROT_ARP	0x0806		/* IP ARP protocol		*/
#define PROT_WOL	0x0842		/* ether-wake WoL protocol	*/
#define PROT_RARP	0x8035		/* IP ARP protocol		*/
#define PROT_VLAN	0x8100		/* IEEE 802.1q protocol		*/
#define PROT_IPV6	0x86dd		/* IPv6 over bluebook		*/
#define PROT_PPP_SES	0x8864		/* PPPoE session messages	*/
#define PROT_NCSI	0x88f8		/* NC-SI control packets        */

#define IPPROTO_ICMP	 1	/* Internet Control Message Protocol	*/
#define IPPROTO_TCP	6	/* Transmission Control Protocol	*/
#define IPPROTO_UDP	17	/* User Datagram Protocol		*/

/*
 *	Internet Protocol (IP) header.
 */
struct ip_hdr {
	u8		ip_hl_v;	/* header length and version	*/
	u8		ip_tos;		/* type of service		*/
	u16		ip_len;		/* total length			*/
	u16		ip_id;		/* identification		*/
	u16		ip_off;		/* fragment offset field	*/
	u8		ip_ttl;		/* time to live			*/
	u8		ip_p;		/* protocol			*/
	u16		ip_sum;		/* checksum			*/
	struct in_addr	ip_src;		/* Source IP address		*/
	struct in_addr	ip_dst;		/* Destination IP address	*/
} __attribute__((packed));

#define IP_OFFS		0x1fff /* ip offset *= 8 */
#define IP_FLAGS	0xe000 /* first 3 bits */
#define IP_FLAGS_RES	0x8000 /* reserved */
#define IP_FLAGS_DFRAG	0x4000 /* don't fragments */
#define IP_FLAGS_MFRAG	0x2000 /* more fragments */

#define IP_HDR_SIZE		(sizeof(struct ip_hdr))

#define IP_MIN_FRAG_DATAGRAM_SIZE	(IP_HDR_SIZE + 8)

/*
 *	Internet Protocol (IP) + UDP header.
 */
struct ip_udp_hdr {
	u8		ip_hl_v;	/* header length and version	*/
	u8		ip_tos;		/* type of service		*/
	u16		ip_len;		/* total length			*/
	u16		ip_id;		/* identification		*/
	u16		ip_off;		/* fragment offset field	*/
	u8		ip_ttl;		/* time to live			*/
	u8		ip_p;		/* protocol			*/
	u16		ip_sum;		/* checksum			*/
	struct in_addr	ip_src;		/* Source IP address		*/
	struct in_addr	ip_dst;		/* Destination IP address	*/
	u16		udp_src;	/* UDP source port		*/
	u16		udp_dst;	/* UDP destination port		*/
	u16		udp_len;	/* Length of UDP packet		*/
	u16		udp_xsum;	/* Checksum			*/
} __attribute__((packed));

#define IP_UDP_HDR_SIZE		(sizeof(struct ip_udp_hdr))
#define UDP_HDR_SIZE		(IP_UDP_HDR_SIZE - IP_HDR_SIZE)

/*
 *	Address Resolution Protocol (ARP) header.
 */
struct arp_hdr {
	u16		ar_hrd;		/* Format of hardware address	*/
#   define ARP_ETHER	    1		/* Ethernet  hardware address	*/
	u16		ar_pro;		/* Format of protocol address	*/
	u8		ar_hln;		/* Length of hardware address	*/
	u8		ar_pln;		/* Length of protocol address	*/
#   define ARP_PLEN	4
	u16		ar_op;		/* Operation			*/
#   define ARPOP_REQUEST    1		/* Request  to resolve  address	*/
#   define ARPOP_REPLY	    2		/* Response to previous request	*/

#   define RARPOP_REQUEST   3		/* Request  to resolve  address	*/
#   define RARPOP_REPLY	    4		/* Response to previous request */

	/*
	 * The remaining fields are variable in size, according to
	 * the sizes above, and are defined as appropriate for
	 * specific hardware/protocol combinations.
	 */
	u8		ar_data[0];
#define ar_sha		ar_data[0]
#define ar_spa		ar_data[ARP_HLEN]
#define ar_tha		ar_data[ARP_HLEN + ARP_PLEN]
#define ar_tpa		ar_data[ARP_HLEN + ARP_PLEN + ARP_HLEN]
#if 0
	u8		ar_sha[];	/* Sender hardware address	*/
	u8		ar_spa[];	/* Sender protocol address	*/
	u8		ar_tha[];	/* Target hardware address	*/
	u8		ar_tpa[];	/* Target protocol address	*/
#endif /* 0 */
} __attribute__((packed));

#define ARP_HDR_SIZE	(8+20)		/* Size assuming ethernet	*/

/*
 * ICMP stuff (just enough to handle (host) redirect messages)
 */
#define ICMP_ECHO_REPLY		0	/* Echo reply			*/
#define ICMP_NOT_REACH		3	/* Detination unreachable	*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO_REQUEST	8	/* Echo request			*/

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET		0	/* Redirect Net			*/
#define ICMP_REDIR_HOST		1	/* Redirect Host		*/

/* Codes for NOT_REACH */
#define ICMP_NOT_REACH_PORT	3	/* Port unreachable		*/

struct icmp_hdr {
	u8		type;
	u8		code;
	u16		checksum;
	union {
		struct {
			u16	id;
			u16	sequence;
		} echo;
		u32	gateway;
		struct {
			u16	unused;
			u16	mtu;
		} frag;
		u8 data[0];
	} un;
} __attribute__((packed));

#define ICMP_HDR_SIZE		(sizeof(struct icmp_hdr))
#define IP_ICMP_HDR_SIZE	(IP_HDR_SIZE + ICMP_HDR_SIZE)

/*
 * Maximum packet size; used to allocate packet storage. Use
 * the maxium Ethernet frame size as specified by the Ethernet
 * standard including the 802.1Q tag (VLAN tagging).
 * maximum packet size =  1522
 * maximum packet size and multiple of 32 bytes =  1536
 */
#define PKTSIZE			1522
#ifndef CONFIG_DM_DSA
#define PKTSIZE_ALIGN		1536
#else
/* Maximum DSA tagging overhead (headroom and/or tailroom) */
#define DSA_MAX_OVR		256
#define PKTSIZE_ALIGN		(1536 + DSA_MAX_OVR)
#endif

/*
 * Maximum receive ring size; that is, the number of packets
 * we can buffer before overflow happens. Basically, this just
 * needs to be enough to prevent a packet being discarded while
 * we are processing the previous one.
 */
#define RINGSZ		4
#define RINGSZ_LOG2	2

/**********************************************************************/
/*
 *	Globals.
 *
 * Note:
 *
 * All variables of type struct in_addr are stored in NETWORK byte order
 * (big endian).
 */

/* net.c */
/** BOOTP EXTENTIONS **/
extern struct in_addr net_gateway;	/* Our gateway IP address */
extern struct in_addr net_netmask;	/* Our subnet mask (0 = unknown) */
/* Our Domain Name Server (0 = unknown) */
extern struct in_addr net_dns_server;
#if defined(CONFIG_BOOTP_DNS2)
/* Our 2nd Domain Name Server (0 = unknown) */
extern struct in_addr net_dns_server2;
#endif
extern char	net_nis_domain[32];	/* Our IS domain */
extern char	net_hostname[32];	/* Our hostname */
#ifdef CONFIG_NET
extern char	net_root_path[CONFIG_BOOTP_MAX_ROOT_PATH_LEN];	/* Our root path */
#endif
/* Indicates whether the pxe path prefix / config file was specified in dhcp option */
extern char *pxelinux_configfile;
/** END OF BOOTP EXTENTIONS **/
extern u8		net_ethaddr[ARP_HLEN];		/* Our ethernet address */
extern u8		net_server_ethaddr[ARP_HLEN];	/* Boot server enet address */
extern struct in_addr	net_ip;		/* Our    IP addr (0 = unknown) */
extern struct in_addr	net_server_ip;	/* Server IP addr (0 = unknown) */
extern uchar		*net_tx_packet;		/* THE transmit packet */
extern uchar		*net_rx_packets[PKTBUFSRX]; /* Receive packets */
extern uchar		*net_rx_packet;		/* Current receive packet */
extern int		net_rx_packet_len;	/* Current rx packet length */
extern const u8		net_bcast_ethaddr[ARP_HLEN];	/* Ethernet broadcast address */
extern const u8		net_null_ethaddr[ARP_HLEN];

#define VLAN_NONE	4095			/* untagged */
#define VLAN_IDMASK	0x0fff			/* mask of valid vlan id */
extern ushort		net_our_vlan;		/* Our VLAN */
extern ushort		net_native_vlan;	/* Our Native VLAN */

extern int		net_restart_wrap;	/* Tried all network devices */

enum proto_t {
	BOOTP, RARP, ARP, TFTPGET, DHCP, DHCP6, PING, PING6, DNS, NFS, CDP,
	NETCONS, SNTP, TFTPSRV, TFTPPUT, LINKLOCAL, FASTBOOT_UDP, FASTBOOT_TCP,
	WOL, UDP, NCSI, WGET, RS
};

extern char	net_boot_file_name[1024];/* Boot File name */
/* Indicates whether the file name was specified on the command line */
extern bool	net_boot_file_name_explicit;
/* The actual transferred size of the bootfile (in bytes) */
extern u32	net_boot_file_size;
/* Boot file size in blocks as reported by the DHCP server */
extern u32	net_boot_file_expected_size_in_blocks;

#if defined(CONFIG_CMD_DNS)
extern char *net_dns_resolve;		/* The host to resolve  */
extern char *net_dns_env_var;		/* the env var to put the ip into */
#endif

#if defined(CONFIG_CMD_PING)
extern struct in_addr net_ping_ip;	/* the ip address to ping */
#endif

#if defined(CONFIG_CMD_CDP)
/* when CDP completes these hold the return values */
extern ushort cdp_native_vlan;		/* CDP returned native VLAN */
extern ushort cdp_appliance_vlan;	/* CDP returned appliance VLAN */

/*
 * Check for a CDP packet by examining the received MAC address field
 */
static inline int is_cdp_packet(const uchar *ethaddr)
{
	extern const u8 net_cdp_ethaddr[ARP_HLEN];

	return memcmp(ethaddr, net_cdp_ethaddr, ARP_HLEN) == 0;
}
#endif

#if defined(CONFIG_CMD_SNTP)
extern struct in_addr	net_ntp_server;		/* the ip address to NTP */
extern int net_ntp_time_offset;			/* offset time from UTC */
#endif

/* Initialize the network adapter */
int net_init(void);
int net_loop(enum proto_t);

/* Load failed.	 Start again. */
int net_start_again(void);

/* Get size of the ethernet header when we send */
int net_eth_hdr_size(void);

/* Set ethernet header; returns the size of the header */
int net_set_ether(uchar *xet, const uchar *dest_ethaddr, uint prot);
int net_update_ether(struct ethernet_hdr *et, uchar *addr, uint prot);

/* Set IP header */
void net_set_ip_header(uchar *pkt, struct in_addr dest, struct in_addr source,
		       u16 pkt_len, u8 proto);
void net_set_udp_header(uchar *pkt, struct in_addr dest, int dport,
				int sport, int len);

/**
 * compute_ip_checksum() - Compute IP checksum
 *
 * @addr:	Address to check (must be 16-bit aligned)
 * @nbytes:	Number of bytes to check (normally a multiple of 2)
 * Return: 16-bit IP checksum
 */
unsigned compute_ip_checksum(const void *addr, unsigned nbytes);

/**
 * add_ip_checksums() - add two IP checksums
 *
 * @offset:	Offset of first sum (if odd we do a byte-swap)
 * @sum:	First checksum
 * @new_sum:	New checksum to add
 * Return: updated 16-bit IP checksum
 */
unsigned add_ip_checksums(unsigned offset, unsigned sum, unsigned new_sum);

/**
 * ip_checksum_ok() - check if a checksum is correct
 *
 * This works by making sure the checksum sums to 0
 *
 * @addr:	Address to check (must be 16-bit aligned)
 * @nbytes:	Number of bytes to check (normally a multiple of 2)
 * Return: true if the checksum matches, false if not
 */
int ip_checksum_ok(const void *addr, unsigned nbytes);

/* Callbacks */
rxhand_f *net_get_udp_handler(void);	/* Get UDP RX packet handler */
void net_set_udp_handler(rxhand_f *);	/* Set UDP RX packet handler */
rxhand_f *net_get_arp_handler(void);	/* Get ARP RX packet handler */
void net_set_arp_handler(rxhand_f *);	/* Set ARP RX packet handler */
bool arp_is_waiting(void);		/* Waiting for ARP reply? */
void net_set_icmp_handler(rxhand_icmp_f *f); /* Set ICMP RX handler */
void net_set_timeout_handler(ulong, thand_f *);/* Set timeout handler */

/* Network loop state */
enum net_loop_state {
	NETLOOP_CONTINUE,
	NETLOOP_RESTART,
	NETLOOP_SUCCESS,
	NETLOOP_FAIL
};
extern enum net_loop_state net_state;

static inline void net_set_state(enum net_loop_state state)
{
	debug_cond(DEBUG_INT_STATE, "--- NetState set to %d\n", state);
	net_state = state;
}

/*
 * net_get_async_tx_pkt_buf - Get a packet buffer that is not in use for
 *			      sending an asynchronous reply
 *
 * returns - ptr to packet buffer
 */
uchar * net_get_async_tx_pkt_buf(void);

/* Transmit a packet */
static inline void net_send_packet(uchar *pkt, int len)
{
	if (DEBUG_NET_PKT_TRACE)
		print_hex_dump_bytes("tx: ", DUMP_PREFIX_OFFSET, pkt, len);
	/* Currently no way to return errors from eth_send() */
	(void) eth_send(pkt, len);
}

/**
 * net_send_ip_packet() - Transmit "net_tx_packet" as UDP or TCP packet,
 *                        send ARP request if needed (ether will be populated)
 * @ether: Raw packet buffer
 * @dest: IP address to send the datagram to
 * @dport: Destination UDP port
 * @sport: Source UDP port
 * @payload_len: Length of data after the UDP header
 * @action: TCP action to be performed
 * @tcp_seq_num: TCP sequence number of this transmission
 * @tcp_ack_num: TCP stream acknolegement number
 *
 * Return: 0 on success, other value on failure
 */
int net_send_ip_packet(uchar *ether, struct in_addr dest, int dport, int sport,
		       int payload_len, int proto, u8 action, u32 tcp_seq_num,
		       u32 tcp_ack_num);
/**
 * net_send_tcp_packet() - Transmit TCP packet.
 * @payload_len: length of payload
 * @dport: Destination TCP port
 * @sport: Source TCP port
 * @action: TCP action to be performed
 * @tcp_seq_num: TCP sequence number of this transmission
 * @tcp_ack_num: TCP stream acknolegement number
 *
 * Return: 0 on success, other value on failure
 */
int net_send_tcp_packet(int payload_len, int dport, int sport, u8 action,
			u32 tcp_seq_num, u32 tcp_ack_num);
int net_send_udp_packet(uchar *ether, struct in_addr dest, int dport,
			int sport, int payload_len);

/* Processes a received packet */
void net_process_received_packet(uchar *in_packet, int len);

#if defined(CONFIG_NETCONSOLE) && !defined(CONFIG_SPL_BUILD)
void nc_start(void);
int nc_input_packet(uchar *pkt, struct in_addr src_ip, unsigned dest_port,
	unsigned src_port, unsigned len);
#endif

static __always_inline int eth_is_on_demand_init(void)
{
#if defined(CONFIG_NETCONSOLE) && !defined(CONFIG_SPL_BUILD)
	extern enum proto_t net_loop_last_protocol;

	return net_loop_last_protocol != NETCONS;
#else
	return 1;
#endif
}

static inline void eth_set_last_protocol(int protocol)
{
#if defined(CONFIG_NETCONSOLE) && !defined(CONFIG_SPL_BUILD)
	extern enum proto_t net_loop_last_protocol;

	net_loop_last_protocol = protocol;
#endif
}

/*
 * Check if autoload is enabled. If so, use either NFS or TFTP to download
 * the boot file.
 */
void net_auto_load(void);

/*
 * The following functions are a bit ugly, but necessary to deal with
 * alignment restrictions on ARM.
 *
 * We're using inline functions, which had the smallest memory
 * footprint in our tests.
 */
/* return IP *in network byteorder* */
static inline struct in_addr net_read_ip(void *from)
{
	struct in_addr ip;

	memcpy((void *)&ip, (void *)from, sizeof(ip));
	return ip;
}

/* return ulong *in network byteorder* */
static inline u32 net_read_u32(void *from)
{
	u32 l;

	memcpy((void *)&l, (void *)from, sizeof(l));
	return l;
}

/* write IP *in network byteorder* */
static inline void net_write_ip(void *to, struct in_addr ip)
{
	memcpy(to, (void *)&ip, sizeof(ip));
}

/* copy IP */
static inline void net_copy_ip(void *to, void *from)
{
	memcpy((void *)to, from, sizeof(struct in_addr));
}

/* copy ulong */
static inline void net_copy_u32(void *to, void *from)
{
	memcpy((void *)to, (void *)from, sizeof(u32));
}

/**
 * is_zero_ethaddr - Determine if give Ethernet address is all zeros.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is all zeroes.
 */
static inline int is_zero_ethaddr(const u8 *addr)
{
	return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

/**
 * is_multicast_ethaddr - Determine if the Ethernet address is a multicast.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is a multicast address.
 * By definition the broadcast address is also a multicast address.
 */
static inline int is_multicast_ethaddr(const u8 *addr)
{
	return 0x01 & addr[0];
}

/*
 * is_broadcast_ethaddr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is the broadcast address.
 */
static inline int is_broadcast_ethaddr(const u8 *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) ==
		0xff;
}

/*
 * is_valid_ethaddr - Determine if the given Ethernet address is valid
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Check that the Ethernet address (MAC) is not 00:00:00:00:00:00, is not
 * a multicast address, and is not FF:FF:FF:FF:FF:FF.
 *
 * Return true if the address is valid.
 */
static inline int is_valid_ethaddr(const u8 *addr)
{
	/* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
	 * explicitly check for it here. */
	return !is_multicast_ethaddr(addr) && !is_zero_ethaddr(addr);
}

/**
 * net_random_ethaddr - Generate software assigned random Ethernet address
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Generate a random Ethernet address (MAC) that is not multicast
 * and has the local assigned bit set.
 */
static inline void net_random_ethaddr(uchar *addr)
{
	int i;
	unsigned int seed = get_ticks();

	for (i = 0; i < 6; i++)
		addr[i] = rand_r(&seed);

	addr[0] &= 0xfe;	/* clear multicast bit */
	addr[0] |= 0x02;	/* set local assignment bit (IEEE802) */
}

/**
 * string_to_enetaddr() - Parse a MAC address
 *
 * Convert a string MAC address
 *
 * Implemented in lib/net_utils.c (built unconditionally)
 *
 * @addr: MAC address in aa:bb:cc:dd:ee:ff format, where each part is a 2-digit
 *	hex value
 * @enetaddr: Place to put MAC address (6 bytes)
 */
void string_to_enetaddr(const char *addr, uint8_t *enetaddr);

/* Convert an IP address to a string */
void ip_to_string(struct in_addr x, char *s);

/**
 * string_to_ip() - Convert a string to ip address
 *
 * Implemented in lib/net_utils.c (built unconditionally)
 *
 * @s: Input string to parse
 * @return: in_addr struct containing the parsed IP address
 */
struct in_addr string_to_ip(const char *s);

/* Convert a VLAN id to a string */
void vlan_to_string(ushort x, char *s);

/* Convert a string to a vlan id */
ushort string_to_vlan(const char *s);

/* read a VLAN id from an environment variable */
ushort env_get_vlan(char *);

/* copy a filename (allow for "..." notation, limit length) */
void copy_filename(char *dst, const char *src, int size);

/* check if serverip is specified in filename from the command line */
int is_serverip_in_cmd(void);

/**
 * net_parse_bootfile - Parse the bootfile env var / cmd line param
 *
 * @param ipaddr - a pointer to the ipaddr to populate if included in bootfile
 * @param filename - a pointer to the string to save the filename part
 * @param max_len - The longest - 1 that the filename part can be
 *
 * return 1 if parsed, 0 if bootfile is empty
 */
int net_parse_bootfile(struct in_addr *ipaddr, char *filename, int max_len);

/**
 * update_tftp - Update firmware over TFTP (via DFU)
 *
 * This function updates board's firmware via TFTP
 *
 * @param addr - memory address where data is stored
 * @param interface - the DFU medium name - e.g. "mmc"
 * @param devstring - the DFU medium number - e.g. "1"
 *
 * Return: - 0 on success, other value on failure
 */
int update_tftp(ulong addr, char *interface, char *devstring);

/**
 * env_get_ip() - Convert an environment value to to an ip address
 *
 * @var: Environment variable to convert. The value of this variable must be
 *	in the format format a.b.c.d, where each value is a decimal number from
 *	0 to 255
 * Return: IP address, or 0 if invalid
 */
static inline struct in_addr env_get_ip(char *var)
{
	return string_to_ip(env_get(var));
}

/**
 * reset_phy() - Reset the Ethernet PHY
 *
 * This should be implemented by boards if CONFIG_RESET_PHY_R is enabled
 */
void reset_phy(void);

#if CONFIG_IS_ENABLED(NET)
/**
 * eth_set_enable_bootdevs() - Enable or disable binding of Ethernet bootdevs
 *
 * These get in the way of bootstd testing, so are normally disabled by tests.
 * This provide control of this setting. It only affects binding of Ethernet
 * devices, so if that has already happened, this flag does nothing.
 *
 * @enable: true to enable binding of bootdevs when binding new Ethernet
 * devices, false to disable it
 */
void eth_set_enable_bootdevs(bool enable);
#else
static inline void eth_set_enable_bootdevs(bool enable) {}
#endif

/**
 * wget_with_dns() - runs dns host IP address resulution before wget
 *
 * @dst_addr:	destination address to download the file
 * @uri:	uri string of target file of wget
 * Return:	downloaded file size, negative if failed
 */
int wget_with_dns(ulong dst_addr, char *uri);

/**
 * wget_validate_uri() - varidate the uri
 *
 * @uri:	uri string of target file of wget
 * Return:	true if uri is valid, false if uri is invalid
 */
bool wget_validate_uri(char *uri);

#endif /* __NET_H__ */
