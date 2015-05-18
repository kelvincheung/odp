/* Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */


/**
 * @file
 *
 * ODP packet descriptor - implementation internal
 */

#ifndef ODP_PACKET_INTERNAL_H_
#define ODP_PACKET_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <odp/align.h>
#include <odp_debug_internal.h>
#include <odp/debug.h>
#include <odp_buffer_internal.h>
#include <odp_pool_internal.h>
#include <odp/packet.h>
#include <odp/packet_io.h>

#include <rte_acl_osdep.h>

/**
 * Packet input & protocol flags
 */
typedef union {
	/* All input flags */
	uint32_t all;

	struct {
		uint32_t unparsed:1;  /**< Set to inticate parse needed */

		uint32_t l2:1;        /**< known L2 protocol present */
		uint32_t l3:1;        /**< known L3 protocol present */
		uint32_t l4:1;        /**< known L4 protocol present */

		uint32_t eth:1;       /**< Ethernet */
		uint32_t jumbo:1;     /**< Jumbo frame */
		uint32_t vlan:1;      /**< VLAN hdr found */
		uint32_t vlan_qinq:1; /**< Stacked VLAN found, QinQ */

		uint32_t snap:1;      /**< SNAP */
		uint32_t arp:1;       /**< ARP */

		uint32_t ipv4:1;      /**< IPv4 */
		uint32_t ipv6:1;      /**< IPv6 */
		uint32_t ipfrag:1;    /**< IP fragment */
		uint32_t ipopt:1;     /**< IP optional headers */
		uint32_t ipsec:1;     /**< IPSec decryption may be needed */

		uint32_t udp:1;       /**< UDP */
		uint32_t tcp:1;       /**< TCP */
		uint32_t tcpopt:1;    /**< TCP options present */
		uint32_t sctp:1;      /**< SCTP */
		uint32_t icmp:1;      /**< ICMP */
	};
} input_flags_t;

_ODP_STATIC_ASSERT(sizeof(input_flags_t) == sizeof(uint32_t),
		   "INPUT_FLAGS_SIZE_ERROR");

/**
 * Packet error flags
 */
typedef union {
	/* All error flags */
	uint32_t all;

	struct {
		/* Bitfield flags for each detected error */
		uint32_t app_error:1; /**< Error bit for application use */
		uint32_t frame_len:1; /**< Frame length error */
		uint32_t snap_len:1;  /**< Snap length error */
		uint32_t l2_chksum:1; /**< L2 checksum error, checks TBD */
		uint32_t ip_err:1;    /**< IP error,  checks TBD */
		uint32_t tcp_err:1;   /**< TCP error, checks TBD */
		uint32_t udp_err:1;   /**< UDP error, checks TBD */
	};
} error_flags_t;

_ODP_STATIC_ASSERT(sizeof(error_flags_t) == sizeof(uint32_t),
		   "ERROR_FLAGS_SIZE_ERROR");

/**
 * Packet output flags
 */
typedef union {
	/* All output flags */
	uint32_t all;

	struct {
		/* Bitfield flags for each output option */
		uint32_t l3_chksum_set:1; /**< L3 chksum bit is valid */
		uint32_t l3_chksum:1;     /**< L3 chksum override */
		uint32_t l4_chksum_set:1; /**< L3 chksum bit is valid */
		uint32_t l4_chksum:1;     /**< L4 chksum override  */
	};
} output_flags_t;

_ODP_STATIC_ASSERT(sizeof(output_flags_t) == sizeof(uint32_t),
		   "OUTPUT_FLAGS_SIZE_ERROR");

/**
 * Internal Packet header
 */
typedef struct {
	/* common buffer header */
	odp_buffer_hdr_t buf_hdr;

	input_flags_t  input_flags;
	error_flags_t  error_flags;
	output_flags_t output_flags;

	uint32_t frame_offset; /**< offset to start of frame, even on error */
	uint32_t l2_offset; /**< offset to L2 hdr, e.g. Eth */
	uint32_t l3_offset; /**< offset to L3 hdr, e.g. IPv4, IPv6 */
	uint32_t l4_offset; /**< offset to L4 hdr (TCP, UDP, SCTP, also ICMP) */
	uint32_t payload_offset; /**< offset to payload */

	uint32_t vlan_s_tag;     /**< Parsed 1st VLAN header (S-TAG) */
	uint32_t vlan_c_tag;     /**< Parsed 2nd VLAN header (C-TAG) */
	uint32_t l3_protocol;    /**< Parsed L3 protocol */
	uint32_t l3_len;         /**< Layer 3 length */
	uint32_t l4_protocol;    /**< Parsed L4 protocol */
	uint32_t l4_len;         /**< Layer 4 length */
	odp_pktio_t input;
	uint32_t uarea_size;     /**< User metadata size, it's right after
				      odp_packet_hdr_t*/
} odp_packet_hdr_t __rte_cache_aligned;

/**
 * Return the packet header
 */
static inline odp_packet_hdr_t *odp_packet_hdr(odp_packet_t pkt)
{
	return (odp_packet_hdr_t *)pkt;
}

/**
 * Parse packet and set internal metadata
 */
void odp_packet_parse(odp_packet_t pkt, size_t len, size_t l2_offset);

#define ODP_PACKET_UNPARSED ~0

static inline void _odp_packet_reset_parse(odp_packet_t pkt)
{
	odp_packet_hdr_t *pkt_hdr = odp_packet_hdr(pkt);
	pkt_hdr->input_flags.all = ODP_PACKET_UNPARSED;
}

/* Forward declarations */
int _odp_packet_copy_to_packet(odp_packet_t srcpkt, uint32_t srcoffset,
			       odp_packet_t dstpkt, uint32_t dstoffset,
			       uint32_t len);

int _odp_packet_parse(odp_packet_hdr_t *pkt_hdr);

void _odp_packet_copy_md_to_packet(odp_packet_t srcpkt, odp_packet_t dstpkt);

/* Convert a packet handle to a buffer handle */
odp_buffer_t _odp_packet_to_buffer(odp_packet_t pkt);

/* Convert a buffer handle to a packet handle */
odp_packet_t _odp_packet_from_buffer(odp_buffer_t buf);

/* DPDK will reserve RTE_PKTMBUF_HEADROOM in any case */
_ODP_STATIC_ASSERT(ODP_CONFIG_PACKET_HEADROOM <= RTE_PKTMBUF_HEADROOM,
		   "ERROR: Headroom has to be smaller or equal to DPDK");

/* We can't enforce tailroom reservation for received packets */
_ODP_STATIC_ASSERT(ODP_CONFIG_PACKET_TAILROOM == 0,
		   "ERROR: Tailroom has to be 0, DPDK doesn't support this");

/*
 * These options are causing a bug in DPDK. It is fixed by Olivier Matz's series
 * starting with 1d493 "mbuf: fix data room size calculation in pool init",
 * which was upstreamed after 2.0.0.
 */

#ifdef RTE_MBUF_SCATTER_GATHER
_ODP_STATIC_ASSERT(0, "ERROR: Scatter gather is not supported!");
#endif

#ifdef RTE_LIBRTE_IP_FRAG
_ODP_STATIC_ASSERT(0, "ERROR: IP frags are not supported!");
#endif

#ifdef __cplusplus
}
#endif

#endif
