/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the TCP module.
 *
 * Version:	@(#)tcp.h	1.0.5	05/23/93
 *
 * Authors:	Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _TCP_H
#define _TCP_H

#define FASTRETRANS_DEBUG 1

#include <linux/list.h>
#include <linux/tcp.h>
#include <linux/bug.h>
#include <linux/slab.h>
#include <linux/cache.h>
#include <linux/percpu.h>
#include <linux/skbuff.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <linux/kref.h>
#include <linux/ktime.h>

#include <net/inet_connection_sock.h>
#include <net/inet_timewait_sock.h>
#include <net/inet_hashtables.h>
#include <net/checksum.h>
#include <net/request_sock.h>
#include <net/sock.h>
#include <net/snmp.h>
#include <net/ip.h>
#include <net/tcp_states.h>
#include <net/inet_ecn.h>
#include <net/dst.h>

#include <linux/seq_file.h>
#include <linux/memcontrol.h>

extern struct inet_hashinfo tcp_hashinfo;

extern struct percpu_counter tcp_orphan_count;
void tcp_time_wait(struct sock *sk, int state, int timeo);

#define MAX_TCP_HEADER	(128 + MAX_HEADER)
#define MAX_TCP_OPTION_SPACE 40

/* 
 * Never offer a window over 32767 without using window scaling. Some
 * poor stacks do signed 16bit maths! 
 */
#define MAX_TCP_WINDOW		32767U

/* Minimal accepted MSS. It is (60+60+8) - (20+20). */
#define TCP_MIN_MSS		88U

/* The least MTU to use for probing */
#define TCP_BASE_MSS		512

/* After receiving this amount of duplicate ACKs fast retransmit starts. */
#define TCP_FASTRETRANS_THRESH 3

/* Maximal reordering. */
#define TCP_MAX_REORDERING	127

/* Maximal number of ACKs sent quickly to accelerate slow-start. */
#define TCP_MAX_QUICKACKS	16U

/* urg_data states */
#define TCP_URG_VALID	0x0100
#define TCP_URG_NOTYET	0x0200
#define TCP_URG_READ	0x0400

#define TCP_RETR1	3	/*
				 * This is how many retries it does before it
				 * tries to figure out if the gateway is
				 * down. Minimal RFC value is 3; it corresponds
				 * to ~3sec-8min depending on RTO.
				 */

#define TCP_RETR2	15	/*
				 * This should take at least
				 * 90 minutes to time out.
				 * RFC1122 says that the limit is 100 sec.
				 * 15 is ~13-30min depending on RTO.
				 */

#define TCP_SYN_RETRIES	 6	/* This is how many retries are done
				 * when active opening a connection.
				 * RFC1122 says the minimum retry MUST
				 * be at least 180secs.  Nevertheless
				 * this value is corresponding to
				 * 63secs of retransmission with the
				 * current initial RTO.
				 */

#define TCP_SYNACK_RETRIES 5	/* This is how may retries are done
				 * when passive opening a connection.
				 * This is corresponding to 31secs of
				 * retransmission with the current
				 * initial RTO.
				 */

#define TCP_TIMEWAIT_LEN (60*HZ) /* how long to wait to destroy TIME-WAIT
				  * state, about 60 seconds	*/
#define TCP_FIN_TIMEOUT	TCP_TIMEWAIT_LEN
                                 /* BSD style FIN_WAIT2 deadlock breaker.
				  * It used to be 3min, new value is 60sec,
				  * to combine FIN-WAIT-2 timeout with
				  * TIME-WAIT timer.
				  */

#define TCP_DELACK_MAX	((unsigned)(HZ/5))	/* maximal time to delay before sending an ACK */
#if HZ >= 100
#define TCP_DELACK_MIN	((unsigned)(HZ/25))	/* minimal time to delay before sending an ACK */
#define TCP_ATO_MIN	((unsigned)(HZ/25))
#else
#define TCP_DELACK_MIN	4U
#define TCP_ATO_MIN	4U
#endif
#define TCP_RTO_MAX	((unsigned)(120*HZ))
#define TCP_RTO_MIN	((unsigned)(HZ/5))
#define TCP_TIMEOUT_INIT ((unsigned)(1*HZ))	/* RFC6298 2.1 initial RTO value	*/
#define TCP_TIMEOUT_FALLBACK ((unsigned)(3*HZ))	/* RFC 1122 initial RTO value, now
						 * used as a fallback RTO for the
						 * initial data transmission if no
						 * valid RTT sample has been acquired,
						 * most likely due to retrans in 3WHS.
						 */

/* Number of full MSS to receive before Acking RFC2581 */
#define TCP_DELACK_SEG          1

#define TCP_RESOURCE_PROBE_INTERVAL ((unsigned)(HZ/2U)) /* Maximal interval between probes
					                 * for local resources.
					                 */

#define TCP_KEEPALIVE_TIME	(120*60*HZ)	/* two hours */
#define TCP_KEEPALIVE_PROBES	9		/* Max of 9 keepalive probes	*/
#define TCP_KEEPALIVE_INTVL	(75*HZ)

#define MAX_TCP_KEEPIDLE	32767
#define MAX_TCP_KEEPINTVL	32767
#define MAX_TCP_KEEPCNT		127
#define MAX_TCP_SYNCNT		127

#define TCP_SYNQ_INTERVAL	(HZ/5)	/* Period of SYNACK timer */

#define TCP_PAWS_24DAYS	(60 * 60 * 24 * 24)
#define TCP_PAWS_MSL	60		/* Per-host timestamps are invalidated
					 * after this time. It should be equal
					 * (or greater than) TCP_TIMEWAIT_LEN
					 * to provide reliability equal to one
					 * provided by timewait state.
					 */
#define TCP_PAWS_WINDOW	1		/* Replay window for per-host
					 * timestamps. It must be less than
					 * minimal timewait lifetime.
					 */
/*
 *	TCP option
 */
 
#define TCPOPT_NOP		1	/* Padding */
#define TCPOPT_EOL		0	/* End of options */
#define TCPOPT_MSS		2	/* Segment size negotiating */
#define TCPOPT_WINDOW		3	/* Window scaling */
#define TCPOPT_SACK_PERM        4       /* SACK Permitted */
#define TCPOPT_SACK             5       /* SACK Block */
#define TCPOPT_TIMESTAMP	8	/* Better RTT estimations/PAWS */
#define TCPOPT_MD5SIG		19	/* MD5 Signature (RFC2385) */
#ifdef CONFIG_MPTCP
#define TCPOPT_MPTCP		30
#endif
#define TCPOPT_EXP		254	/* Experimental */
/* Magic number to be after the option value for sharing TCP
 * experimental options. See draft-ietf-tcpm-experimental-options-00.txt
 */
#define TCPOPT_FASTOPEN_MAGIC	0xF989

/*
 *     TCP option lengths
 */

#define TCPOLEN_MSS            4
#define TCPOLEN_WINDOW         3
#define TCPOLEN_SACK_PERM      2
#define TCPOLEN_TIMESTAMP      10
#define TCPOLEN_MD5SIG         18
#define TCPOLEN_EXP_FASTOPEN_BASE  4

/* But this is what stacks really send out. */
#define TCPOLEN_TSTAMP_ALIGNED		12
#define TCPOLEN_WSCALE_ALIGNED		4
#define TCPOLEN_SACKPERM_ALIGNED	4
#define TCPOLEN_SACK_BASE		2
#define TCPOLEN_SACK_BASE_ALIGNED	4
#define TCPOLEN_SACK_PERBLOCK		8
#define TCPOLEN_MD5SIG_ALIGNED		20
#define TCPOLEN_MSS_ALIGNED		4

/* Flags in tp->nonagle */
#define TCP_NAGLE_OFF		1	/* Nagle's algo is disabled */
#define TCP_NAGLE_CORK		2	/* Socket is corked	    */
#define TCP_NAGLE_PUSH		4	/* Cork is overridden for already queued data */

/* TCP thin-stream limits */
#define TCP_THIN_LINEAR_RETRIES 6       /* After 6 linear retries, do exp. backoff */

/* TCP initial congestion window as per draft-hkchu-tcpm-initcwnd-01 */
#define TCP_INIT_CWND		10

/* Bit Flags for sysctl_tcp_fastopen */
#define	TFO_CLIENT_ENABLE	1
#define	TFO_SERVER_ENABLE	2
#define	TFO_CLIENT_NO_COOKIE	4	/* Data in SYN w/o cookie option */

/* Accept SYN data w/o any cookie option */
#define	TFO_SERVER_COOKIE_NOT_REQD	0x200

/* Force enable TFO on all listeners, i.e., not requiring the
 * TCP_FASTOPEN socket option. SOCKOPT1/2 determine how to set max_qlen.
 */
#define	TFO_SERVER_WO_SOCKOPT1	0x400
#define	TFO_SERVER_WO_SOCKOPT2	0x800

#ifdef CONFIG_MPTCP
/* Flags from tcp_input.c for tcp_ack */
#define FLAG_DATA               0x01 /* Incoming frame contained data.          */
#define FLAG_WIN_UPDATE         0x02 /* Incoming ACK was a window update.       */
#define FLAG_DATA_ACKED         0x04 /* This ACK acknowledged new data.         */
#define FLAG_RETRANS_DATA_ACKED 0x08 /* "" "" some of which was retransmitted.  */
#define FLAG_SYN_ACKED          0x10 /* This ACK acknowledged SYN.              */
#define FLAG_DATA_SACKED        0x20 /* New SACK.                               */
#define FLAG_ECE                0x40 /* ECE in this ACK                         */
#define FLAG_SLOWPATH           0x100 /* Do not skip RFC checks for window update.*/
#define FLAG_ORIG_SACK_ACKED    0x200 /* Never retransmitted data are (s)acked  */
#define FLAG_SND_UNA_ADVANCED   0x400 /* Snd_una was changed (!= FLAG_DATA_ACKED) */
#define FLAG_DSACKING_ACK       0x800 /* SACK blocks contained D-SACK info */
#define FLAG_SACK_RENEGING      0x2000 /* snd_una advanced to a sacked seq */
#define FLAG_UPDATE_TS_RECENT   0x4000 /* tcp_replace_ts_recent() */
#define MPTCP_FLAG_DATA_ACKED	0x8000

#define FLAG_ACKED              (FLAG_DATA_ACKED|FLAG_SYN_ACKED)
#define FLAG_NOT_DUP            (FLAG_DATA|FLAG_WIN_UPDATE|FLAG_ACKED)
#define FLAG_CA_ALERT           (FLAG_DATA_SACKED|FLAG_ECE)
#define FLAG_FORWARD_PROGRESS   (FLAG_ACKED|FLAG_DATA_SACKED)
#endif

extern struct inet_timewait_death_row tcp_death_row;

/* sysctl variables for tcp */
extern int sysctl_tcp_timestamps;
extern int sysctl_tcp_window_scaling;
extern int sysctl_tcp_sack;
extern int sysctl_tcp_fin_timeout;
extern int sysctl_tcp_keepalive_time;
extern int sysctl_tcp_keepalive_probes;
extern int sysctl_tcp_keepalive_intvl;
extern int sysctl_tcp_syn_retries;
extern int sysctl_tcp_synack_retries;
extern int sysctl_tcp_retries1;
extern int sysctl_tcp_retries2;
extern int sysctl_tcp_orphan_retries;
extern int sysctl_tcp_syncookies;
extern int sysctl_tcp_fastopen;
extern int sysctl_tcp_retrans_collapse;
extern int sysctl_tcp_stdurg;
extern int sysctl_tcp_rfc1337;
extern int sysctl_tcp_abort_on_overflow;
extern int sysctl_tcp_max_orphans;
extern int sysctl_tcp_fack;
extern int sysctl_tcp_reordering;
extern int sysctl_tcp_dsack;
extern long sysctl_tcp_mem[3];
extern int sysctl_tcp_wmem[3];
extern int sysctl_tcp_rmem[3];
extern int sysctl_tcp_app_win;
extern int sysctl_tcp_adv_win_scale;
extern int sysctl_tcp_tw_reuse;
extern int sysctl_tcp_frto;
extern int sysctl_tcp_low_latency;
extern int sysctl_tcp_nometrics_save;
extern int sysctl_tcp_moderate_rcvbuf;
extern int sysctl_tcp_tso_win_divisor;
extern int sysctl_tcp_mtu_probing;
extern int sysctl_tcp_base_mss;
extern int sysctl_tcp_workaround_signed_windows;
extern int sysctl_tcp_slow_start_after_idle;
extern int sysctl_tcp_thin_linear_timeouts;
extern int sysctl_tcp_thin_dupack;
extern int sysctl_tcp_early_retrans;
extern int sysctl_tcp_limit_output_bytes;
extern int sysctl_tcp_challenge_ack_limit;
extern unsigned int sysctl_tcp_notsent_lowat;
extern int sysctl_tcp_min_tso_segs;
extern int sysctl_tcp_autocorking;
extern int sysctl_tcp_default_init_rwnd;

extern atomic_long_t tcp_memory_allocated;

/* sysctl variables for controlling various tcp parameters */
extern int sysctl_tcp_delack_seg;
extern int sysctl_tcp_use_userconfig;

extern struct percpu_counter tcp_sockets_allocated;
extern int tcp_memory_pressure;

/*
 * The next routines deal with comparing 32 bit unsigned ints
 * and worry about wraparound (automatic with unsigned arithmetic).
 */

static inline bool before(__u32 seq1, __u32 seq2)
{
        return (__s32)(seq1-seq2) < 0;
}
#define after(seq2, seq1) 	before(seq1, seq2)

/* is s2<=s1<=s3 ? */
static inline bool between(__u32 seq1, __u32 seq2, __u32 seq3)
{
	return seq3 - seq2 >= seq1 - seq2;
}

static inline bool tcp_out_of_memory(struct sock *sk)
{
	if (sk->sk_wmem_queued > SOCK_MIN_SNDBUF &&
	    sk_memory_allocated(sk) > sk_prot_mem_limits(sk, 2))
		return true;
	return false;
}

static inline bool tcp_too_many_orphans(struct sock *sk, int shift)
{
	struct percpu_counter *ocp = sk->sk_prot->orphan_count;
	int orphans = percpu_counter_read_positive(ocp);

	if (orphans << shift > sysctl_tcp_max_orphans) {
		orphans = percpu_counter_sum_positive(ocp);
		if (orphans << shift > sysctl_tcp_max_orphans)
			return true;
	}
	return false;
}

bool tcp_check_oom(struct sock *sk, int shift);

/* syncookies: remember time of last synqueue overflow */
static inline void tcp_synq_overflow(struct sock *sk)
{
	tcp_sk(sk)->rx_opt.ts_recent_stamp = jiffies;
}

/* syncookies: no recent synqueue overflow on this listening socket? */
static inline bool tcp_synq_no_recent_overflow(const struct sock *sk)
{
	unsigned long last_overflow = tcp_sk(sk)->rx_opt.ts_recent_stamp;
	return time_after(jiffies, last_overflow + TCP_TIMEOUT_FALLBACK);
}

extern struct proto tcp_prot;

#define TCP_INC_STATS(net, field)	SNMP_INC_STATS((net)->mib.tcp_statistics, field)
#define TCP_INC_STATS_BH(net, field)	SNMP_INC_STATS_BH((net)->mib.tcp_statistics, field)
#define TCP_DEC_STATS(net, field)	SNMP_DEC_STATS((net)->mib.tcp_statistics, field)
#define TCP_ADD_STATS_USER(net, field, val) SNMP_ADD_STATS_USER((net)->mib.tcp_statistics, field, val)
#define TCP_ADD_STATS(net, field, val)	SNMP_ADD_STATS((net)->mib.tcp_statistics, field, val)

#ifdef CONFIG_MPTCP
/**** START - Exports needed for MPTCP ****/
extern const struct tcp_request_sock_ops tcp_request_sock_ipv4_ops;
extern const struct tcp_request_sock_ops tcp_request_sock_ipv6_ops;

struct mptcp_options_received;

void tcp_cleanup_rbuf(struct sock *sk, int copied);
void tcp_cwnd_validate(struct sock *sk, bool is_cwnd_limited);
void tcp_enter_quickack_mode(struct sock *sk);
int tcp_close_state(struct sock *sk);
void tcp_minshall_update(struct tcp_sock *tp, unsigned int mss_now,
			 const struct sk_buff *skb);
int tcp_xmit_probe_skb(struct sock *sk, int urgent);
void tcp_event_new_data_sent(struct sock *sk, const struct sk_buff *skb);
int tcp_transmit_skb(struct sock *sk, struct sk_buff *skb, int clone_it,
		     gfp_t gfp_mask);
unsigned int tcp_mss_split_point(const struct sock *sk,
				 const struct sk_buff *skb,
				 unsigned int mss_now,
				 unsigned int max_segs,
				 int nonagle);
bool tcp_nagle_test(const struct tcp_sock *tp, const struct sk_buff *skb,
		    unsigned int cur_mss, int nonagle);
bool tcp_snd_wnd_test(const struct tcp_sock *tp, const struct sk_buff *skb,
		      unsigned int cur_mss);
unsigned int tcp_cwnd_test(const struct tcp_sock *tp, const struct sk_buff *skb);
int tcp_init_tso_segs(const struct sock *sk, struct sk_buff *skb,
		      unsigned int mss_now);
void __pskb_trim_head(struct sk_buff *skb, int len);
void tcp_queue_skb(struct sock *sk, struct sk_buff *skb);
void tcp_init_nondata_skb(struct sk_buff *skb, u32 seq, u8 flags);
void tcp_reset(struct sock *sk);
bool tcp_may_update_window(const struct tcp_sock *tp, const u32 ack,
			   const u32 ack_seq, const u32 nwin);
bool tcp_urg_mode(const struct tcp_sock *tp);
void tcp_ack_probe(struct sock *sk);
void tcp_rearm_rto(struct sock *sk);
int tcp_write_timeout(struct sock *sk);
bool retransmits_timed_out(struct sock *sk, unsigned int boundary,
			   unsigned int timeout, bool syn_set);
void tcp_write_err(struct sock *sk);
void tcp_adjust_pcount(struct sock *sk, const struct sk_buff *skb, int decr);
void tcp_set_skb_tso_segs(const struct sock *sk, struct sk_buff *skb,
			  unsigned int mss_now);

void tcp_v4_reqsk_send_ack(struct sock *sk, struct sk_buff *skb,
			   struct request_sock *req);
int tcp_v4_send_synack(struct sock *sk, struct dst_entry *dst,
		       struct flowi *fl,
		       struct request_sock *req,
		       u16 queue_mapping,
		       struct tcp_fastopen_cookie *foc);
void tcp_v4_send_reset(struct sock *sk, struct sk_buff *skb);
struct sock *tcp_v4_hnd_req(struct sock *sk, struct sk_buff *skb);
void tcp_v4_reqsk_destructor(struct request_sock *req);

void tcp_v6_reqsk_send_ack(struct sock *sk, struct sk_buff *skb,
			   struct request_sock *req);
__u32 tcp_v6_init_sequence(const struct sk_buff *skb);
int tcp_v6_send_synack(struct sock *sk, struct dst_entry *dst,
		       struct flowi *fl, struct request_sock *req,
		       u16 queue_mapping, struct tcp_fastopen_cookie *foc);
void tcp_v6_send_reset(struct sock *sk, struct sk_buff *skb);
int tcp_v6_do_rcv(struct sock *sk, struct sk_buff *skb);
int tcp_v6_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len);
void tcp_v6_destroy_sock(struct sock *sk);
void inet6_sk_rx_dst_set(struct sock *sk, const struct sk_buff *skb);
void tcp_v6_hash(struct sock *sk);
struct sock *tcp_v6_hnd_req(struct sock *sk,struct sk_buff *skb);
struct sock *tcp_v6_syn_recv_sock(struct sock *sk, struct sk_buff *skb,
			          struct request_sock *req,
				  struct dst_entry *dst);
void tcp_v6_reqsk_destructor(struct request_sock *req);

unsigned int tcp_xmit_size_goal(struct sock *sk, u32 mss_now,
				       int large_allowed);
u32 tcp_tso_acked(struct sock *sk, struct sk_buff *skb);

void skb_clone_fraglist(struct sk_buff *skb);
void copy_skb_header(struct sk_buff *new, const struct sk_buff *old);

void inet_twsk_free(struct inet_timewait_sock *tw);
int tcp_v6_conn_request(struct sock *sk, struct sk_buff *skb);
/* These states need RST on ABORT according to RFC793 */
static inline bool tcp_need_reset(int state)
{
	return (1 << state) &
	       (TCPF_ESTABLISHED | TCPF_CLOSE_WAIT | TCPF_FIN_WAIT1 |
		TCPF_FIN_WAIT2 | TCPF_SYN_RECV);
}

int __must_check tcp_queue_rcv(struct sock *sk, struct sk_buff *skb, int hdrlen,
			       bool *fragstolen);
bool tcp_try_coalesce(struct sock *sk, struct sk_buff *to,
		      struct sk_buff *from, bool *fragstolen);
/**** END - Exports needed for MPTCP ****/
#endif

void tcp_tasklet_init(void);

void tcp_v4_err(struct sk_buff *skb, u32);

void tcp_shutdown(struct sock *sk, int how);

void tcp_v4_early_demux(struct sk_buff *skb);
int tcp_v4_rcv(struct sk_buff *skb);

int tcp_v4_tw_remember_stamp(struct inet_timewait_sock *tw);
int tcp_sendmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
		size_t size);
int tcp_sendpage(struct sock *sk, struct page *page, int offset, size_t size,
		 int flags);
void tcp_release_cb(struct sock *sk);
void tcp_wfree(struct sk_buff *skb);
void tcp_write_timer_handler(struct sock *sk);
void tcp_delack_timer_handler(struct sock *sk);
int tcp_ioctl(struct sock *sk, int cmd, unsigned long arg);
int tcp_rcv_state_process(struct sock *sk, struct sk_buff *skb,
			  const struct tcphdr *th, unsigned int len);
void tcp_rcv_established(struct sock *sk, struct sk_buff *skb,
			 const struct tcphdr *th, unsigned int len);
void tcp_rcv_space_adjust(struct sock *sk);
int tcp_twsk_unique(struct sock *sk, struct sock *sktw, void *twp);
void tcp_twsk_destructor(struct sock *sk);
ssize_t tcp_splice_read(struct socket *sk, loff_t *ppos,
			struct pipe_inode_info *pipe, size_t len,
			unsigned int flags);

/* sysctl master controller */
extern int tcp_use_userconfig_sysctl_handler(struct ctl_table *, int,
				void __user *, size_t *, loff_t *);
extern int tcp_proc_delayed_ack_control(struct ctl_table *, int,
				void __user *, size_t *, loff_t *);

void tcp_enter_quickack_mode(struct sock *sk, unsigned int max_quickacks);
static inline void tcp_dec_quickack_mode(struct sock *sk,
					 const unsigned int pkts)
{
	struct inet_connection_sock *icsk = inet_csk(sk);

	if (icsk->icsk_ack.quick) {
		if (pkts >= icsk->icsk_ack.quick) {
			icsk->icsk_ack.quick = 0;
			/* Leaving quickack mode we deflate ATO. */
			icsk->icsk_ack.ato   = TCP_ATO_MIN;
		} else
			icsk->icsk_ack.quick -= pkts;
	}
}

#define	TCP_ECN_OK		1
#define	TCP_ECN_QUEUE_CWR	2
#define	TCP_ECN_DEMAND_CWR	4
#define	TCP_ECN_SEEN		8

enum tcp_tw_status {
	TCP_TW_SUCCESS = 0,
	TCP_TW_RST = 1,
	TCP_TW_ACK = 2,
	TCP_TW_SYN = 3
};


enum tcp_tw_status tcp_timewait_state_process(struct inet_timewait_sock *tw,
					      struct sk_buff *skb,
					      const struct tcphdr *th);
struct sock *tcp_check_req(struct sock *sk, struct sk_buff *skb,
			   struct request_sock *req, struct request_sock **prev,
			   bool fastopen);
int tcp_child_process(struct sock *parent, struct sock *child,
		      struct sk_buff *skb);
void tcp_enter_loss(struct sock *sk);
void tcp_clear_retrans(struct tcp_sock *tp);
void tcp_update_metrics(struct sock *sk);
void tcp_init_metrics(struct sock *sk);
void tcp_metrics_init(void);
bool tcp_peer_is_proven(struct request_sock *req, struct dst_entry *dst,
			bool paws_check, bool timestamps);
bool tcp_remember_stamp(struct sock *sk);
bool tcp_tw_remember_stamp(struct inet_timewait_sock *tw);
void tcp_fetch_timewait_stamp(struct sock *sk, struct dst_entry *dst);
void tcp_disable_fack(struct tcp_sock *tp);
void tcp_close(struct sock *sk, long timeout);
void tcp_init_sock(struct sock *sk);
unsigned int tcp_poll(struct file *file, struct socket *sock,
		      struct poll_table_struct *wait);
int tcp_getsockopt(struct sock *sk, int level, int optname,
		   char __user *optval, int __user *optlen);
int tcp_setsockopt(struct sock *sk, int level, int optname,
		   char __user *optval, unsigned int optlen);
int compat_tcp_getsockopt(struct sock *sk, int level, int optname,
			  char __user *optval, int __user *optlen);
int compat_tcp_setsockopt(struct sock *sk, int level, int optname,
			  char __user *optval, unsigned int optlen);
void tcp_set_keepalive(struct sock *sk, int val);
void tcp_syn_ack_timeout(struct sock *sk, struct request_sock *req);
int tcp_recvmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
		size_t len, int nonblock, int flags, int *addr_len);
void tcp_parse_options(const struct sk_buff *skb,
		       struct tcp_options_received *opt_rx,
#ifdef CONFIG_MPTCP
		       struct mptcp_options_received *mopt_rx,
#endif
		       int estab, struct tcp_fastopen_cookie *foc);
const u8 *tcp_parse_md5sig_option(const struct tcphdr *th);

/*
 *	TCP v4 functions exported for the inet6 API
 */

void tcp_v4_send_check(struct sock *sk, struct sk_buff *skb);
void tcp_v4_mtu_reduced(struct sock *sk);
#ifdef CONFIG_MPTCP
void tcp_v6_mtu_reduced(struct sock *sk);
#endif
int tcp_v4_conn_request(struct sock *sk, struct sk_buff *skb);
struct sock *tcp_create_openreq_child(struct sock *sk,
				      struct request_sock *req,
				      struct sk_buff *skb);
struct sock *tcp_v4_syn_recv_sock(struct sock *sk, struct sk_buff *skb,
				  struct request_sock *req,
				  struct dst_entry *dst);
int tcp_v4_do_rcv(struct sock *sk, struct sk_buff *skb);
int tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len);
int tcp_connect(struct sock *sk);
struct sk_buff *tcp_make_synack(struct sock *sk, struct dst_entry *dst,
				struct request_sock *req,
				struct tcp_fastopen_cookie *foc);
int tcp_disconnect(struct sock *sk, int flags);

void tcp_finish_connect(struct sock *sk, struct sk_buff *skb);
int tcp_send_rcvq(struct sock *sk, struct msghdr *msg, size_t size);
void inet_sk_rx_dst_set(struct sock *sk, const struct sk_buff *skb);

/* From syncookies.c */
int __cookie_v4_check(const struct iphdr *iph, const struct tcphdr *th,
		      u32 cookie);
struct sock *cookie_v4_check(struct sock *sk, struct sk_buff *skb);
#ifdef CONFIG_SYN_COOKIES

/* Syncookies use a monotonic timer which increments every 60 seconds.
 * This counter is used both as a hash input and partially encoded into
 * the cookie value.  A cookie is only validated further if the delta
 * between the current counter value and the encoded one is less than this,
 * i.e. a sent cookie is valid only at most for 2*60 seconds (or less if
 * the counter advances immediately after a cookie is generated).
 */
#define MAX_SYNCOOKIE_AGE 2

static inline u32 tcp_cookie_time(void)
{
	u64 val = get_jiffies_64();

	do_div(val, 60 * HZ);
	return val;
}

u32 __cookie_v4_init_sequence(const struct iphdr *iph, const struct tcphdr *th,
			      u16 *mssp);
__u32 cookie_v4_init_sequence(struct sock *sk, const struct sk_buff *skb,
			      __u16 *mss);
#endif

__u32 cookie_init_timestamp(struct request_sock *req);
bool cookie_check_timestamp(struct tcp_options_received *opt, struct net *net,
			    bool *ecn_ok);

/* From net/ipv6/syncookies.c */
int __cookie_v6_check(const struct ipv6hdr *iph, const struct tcphdr *th,
		      u32 cookie);
struct sock *cookie_v6_check(struct sock *sk, struct sk_buff *skb);
#ifdef CONFIG_SYN_COOKIES
u32 __cookie_v6_init_sequence(const struct ipv6hdr *iph,
			      const struct tcphdr *th, u16 *mssp);
__u32 cookie_v6_init_sequence(struct sock *sk, const struct sk_buff *skb,
			      __u16 *mss);
#endif
/* tcp_output.c */

void __tcp_push_pending_frames(struct sock *sk, unsigned int cur_mss,
			       int nonagle);
bool tcp_may_send_now(struct sock *sk);
int __tcp_retransmit_skb(struct sock *, struct sk_buff *);
int tcp_retransmit_skb(struct sock *, struct sk_buff *);
void tcp_retransmit_timer(struct sock *sk);
void tcp_xmit_retransmit_queue(struct sock *);
void tcp_simple_retransmit(struct sock *);
int tcp_trim_head(struct sock *, struct sk_buff *, u32);
int tcp_fragment(struct sock *, struct sk_buff *, u32, unsigned int, gfp_t);

void tcp_send_probe0(struct sock *);
void tcp_send_partial(struct sock *);
int tcp_write_wakeup(struct sock *);
void tcp_send_fin(struct sock *sk);
void tcp_send_active_reset(struct sock *sk, gfp_t priority);
int tcp_send_synack(struct sock *);
bool tcp_syn_flood_action(struct sock *sk, const struct sk_buff *skb,
			  const char *proto);
void tcp_push_one(struct sock *, unsigned int mss_now);
void __tcp_send_ack(struct sock *sk, u32 rcv_nxt);
void tcp_send_ack(struct sock *sk);
void tcp_send_delayed_ack(struct sock *sk);
void tcp_send_loss_probe(struct sock *sk);
bool tcp_schedule_loss_probe(struct sock *sk);

#ifdef CONFIG_MPTCP
u16 tcp_select_window(struct sock *sk);
bool tcp_write_xmit(struct sock *sk, unsigned int mss_now, int nonagle,
		int push_one, gfp_t gfp);
#endif

/* tcp_input.c */
void tcp_resume_early_retransmit(struct sock *sk);
void tcp_rearm_rto(struct sock *sk);
void tcp_reset(struct sock *sk);
#ifdef CONFIG_MPTCP
void tcp_set_rto(struct sock *sk);
bool tcp_should_expand_sndbuf(const struct sock *sk);
bool tcp_prune_ofo_queue(struct sock *sk);
#endif

/* tcp_timer.c */
void tcp_init_xmit_timers(struct sock *);
static inline void tcp_clear_xmit_timers(struct sock *sk)
{
	inet_csk_clear_xmit_timers(sk);
}

unsigned int tcp_sync_mss(struct sock *sk, u32 pmtu);
unsigned int tcp_current_mss(struct sock *sk);

/* Bound MSS / TSO packet size with the half of the window */
static inline int tcp_bound_to_half_wnd(struct tcp_sock *tp, int pktsize)
{
	int cutoff;

	/* When peer uses tiny windows, there is no use in packetizing
	 * to sub-MSS pieces for the sake of SWS or making sure there
	 * are enough packets in the pipe for fast recovery.
	 *
	 * On the other hand, for extremely large MSS devices, handling
	 * smaller than MSS windows in this way does make sense.
	 */
	if (tp->max_window >= 512)
		cutoff = (tp->max_window >> 1);
	else
		cutoff = tp->max_window;

	if (cutoff && pktsize > cutoff)
		return max_t(int, cutoff, 68U - tp->tcp_header_len);
	else
		return pktsize;
}

/* tcp.c */
void tcp_get_info(const struct sock *, struct tcp_info *);

/* Read 'sendfile()'-style from a TCP socket */
typedef int (*sk_read_actor_t)(read_descriptor_t *, struct sk_buff *,
				unsigned int, size_t);
int tcp_read_sock(struct sock *sk, read_descriptor_t *desc,
		  sk_read_actor_t recv_actor);

void tcp_initialize_rcv_mss(struct sock *sk);

int tcp_mtu_to_mss(struct sock *sk, int pmtu);
int tcp_mss_to_mtu(struct sock *sk, int mss);
void tcp_mtup_init(struct sock *sk);
void tcp_init_buffer_space(struct sock *sk);

static inline void tcp_bound_rto(const struct sock *sk)
{
	if (inet_csk(sk)->icsk_rto > TCP_RTO_MAX)
		inet_csk(sk)->icsk_rto = TCP_RTO_MAX;
}

static inline u32 __tcp_set_rto(const struct tcp_sock *tp)
{
	return usecs_to_jiffies((tp->srtt_us >> 3) + tp->rttvar_us);
}

static inline void __tcp_fast_path_on(struct tcp_sock *tp, u32 snd_wnd)
{
	tp->pred_flags = htonl((tp->tcp_header_len << 26) |
			       ntohl(TCP_FLAG_ACK) |
			       snd_wnd);
}

static inline void tcp_fast_path_on(struct tcp_sock *tp)
{
	__tcp_fast_path_on(tp, tp->snd_wnd >> tp->rx_opt.snd_wscale);
}

static inline void tcp_fast_path_check(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (skb_queue_empty(&tp->out_of_order_queue) &&
	    tp->rcv_wnd &&
	    atomic_read(&sk->sk_rmem_alloc) < sk->sk_rcvbuf &&
	    !tp->urg_data)
		tcp_fast_path_on(tp);
}

/* Compute the actual rto_min value */
static inline u32 tcp_rto_min(struct sock *sk)
{
	const struct dst_entry *dst = __sk_dst_get(sk);
	u32 rto_min = TCP_RTO_MIN;

	if (dst && dst_metric_locked(dst, RTAX_RTO_MIN))
		rto_min = dst_metric_rtt(dst, RTAX_RTO_MIN);
	return rto_min;
}

static inline u32 tcp_rto_min_us(struct sock *sk)
{
	return jiffies_to_usecs(tcp_rto_min(sk));
}

/* Compute the actual receive window we are currently advertising.
 * Rcv_nxt can be after the window if our peer push more data
 * than the offered window.
 */
static inline u32 tcp_receive_window(const struct tcp_sock *tp)
{
	s32 win = tp->rcv_wup + tp->rcv_wnd - tp->rcv_nxt;

	if (win < 0)
		win = 0;
	return (u32) win;
}

/* Choose a new window, without checks for shrinking, and without
 * scaling applied to the result.  The caller does these things
 * if necessary.  This is a "raw" window selection.
 */
u32 __tcp_select_window(struct sock *sk);

void tcp_send_window_probe(struct sock *sk);

/* TCP timestamps are only 32-bits, this causes a slight
 * complication on 64-bit systems since we store a snapshot
 * of jiffies in the buffer control blocks below.  We decided
 * to use only the low 32-bits of jiffies and hide the ugly
 * casts with the following macro.
 */
#define tcp_time_stamp		((__u32)(jiffies))

static inline u32 tcp_skb_timestamp(const struct sk_buff *skb)
{
	return skb->skb_mstamp.stamp_jiffies;
}


#define tcp_flag_byte(th) (((u_int8_t *)th)[13])

#define TCPHDR_FIN 0x01
#define TCPHDR_SYN 0x02
#define TCPHDR_RST 0x04
#define TCPHDR_PSH 0x08
#define TCPHDR_ACK 0x10
#define TCPHDR_URG 0x20
#define TCPHDR_ECE 0x40
#define TCPHDR_CWR 0x80

/* This is what the send packet queuing engine uses to pass
 * TCP per-packet control information to the transmission code.
 * We also store the host-order sequence numbers in here too.
 * This is 44 bytes if IPV6 is enabled.
 * If this grows please adjust skbuff.h:skbuff->cb[xxx] size appropriately.
 */
struct tcp_skb_cb {
	__u32		seq;		/* Starting sequence number	*/
	__u32		end_seq;	/* SEQ + FIN + SYN + datalen	*/
	union {
		/* Note : tcp_tw_isn is used in input path only
		 *	  (isn chosen by tcp_timewait_state_process())
		 *
		 * 	  tcp_gso_segs is used in write queue only,
		 *	  cf tcp_skb_pcount()
		 */
		__u32		tcp_tw_isn;
		__u32		tcp_gso_segs;
	};

#ifdef CONFIG_MPTCP
	__u8		mptcp_flags;	/* flags for the MPTCP layer    */
	__u8		dss_off;	/* Number of 4-byte words until
					 * seq-number */
#endif
	__u8		tcp_flags;	/* TCP header flags. (tcp[13])	*/

	__u8		sacked;		/* State flags for SACK/FACK.	*/
#define TCPCB_SACKED_ACKED	0x01	/* SKB ACK'd by a SACK block	*/
#define TCPCB_SACKED_RETRANS	0x02	/* SKB retransmitted		*/
#define TCPCB_LOST		0x04	/* SKB is lost			*/
#define TCPCB_TAGBITS		0x07	/* All tag bits			*/
#define TCPCB_REPAIRED		0x10	/* SKB repaired (no skb_mstamp)	*/
#define TCPCB_EVER_RETRANS	0x80	/* Ever retransmitted frame	*/
#define TCPCB_RETRANS		(TCPCB_SACKED_RETRANS|TCPCB_EVER_RETRANS| \
				TCPCB_REPAIRED)

	__u8		ip_dsfield;	/* IPv4 tos or IPv6 dsfield	*/
	/* 1 byte hole */
	__u32		ack_seq;	/* Sequence number ACK'd	*/
#ifdef CONFIG_MPTCP
	union {
#endif
		union {
			struct inet_skb_parm	h4;
#if IS_ENABLED(CONFIG_IPV6)
			struct inet6_skb_parm	h6;
#endif
		} header;	/* For incoming frames		*/
#ifdef CONFIG_MPTCP
		union {			/* For MPTCP outgoing frames */
			__u32 path_mask; /* paths that tried to send this skb */
			__u32 dss[6];	/* DSS options */
		};
	};
#endif
};

#define TCP_SKB_CB(__skb)	((struct tcp_skb_cb *)&((__skb)->cb[0]))


#if IS_ENABLED(CONFIG_IPV6)
/* This is the variant of inet6_iif() that must be used by TCP,
 * as TCP moves IP6CB into a different location in skb->cb[]
 */
static inline int tcp_v6_iif(const struct sk_buff *skb)
{
	return TCP_SKB_CB(skb)->header.h6.iif;
}
#endif

/* Due to TSO, an SKB can be composed of multiple actual
 * packets.  To keep these tracked properly, we use this.
 */
static inline int tcp_skb_pcount(const struct sk_buff *skb)
{
	return TCP_SKB_CB(skb)->tcp_gso_segs;
}

static inline void tcp_skb_pcount_set(struct sk_buff *skb, int segs)
{
	TCP_SKB_CB(skb)->tcp_gso_segs = segs;
}

static inline void tcp_skb_pcount_add(struct sk_buff *skb, int segs)
{
	TCP_SKB_CB(skb)->tcp_gso_segs += segs;
}

/* This is valid iff tcp_skb_pcount() > 1. */
static inline int tcp_skb_mss(const struct sk_buff *skb)
{
	return skb_shinfo(skb)->gso_size;
}

/* Events passed to congestion control interface */
enum tcp_ca_event {
	CA_EVENT_TX_START,	/* first transmit when no packets in flight */
	CA_EVENT_CWND_RESTART,	/* congestion window restart */
	CA_EVENT_COMPLETE_CWR,	/* end of congestion recovery */
	CA_EVENT_LOSS,		/* loss timeout */
	CA_EVENT_ECN_NO_CE,	/* ECT set, but not CE marked */
	CA_EVENT_ECN_IS_CE,	/* received CE marked IP packet */
	CA_EVENT_DELAYED_ACK,	/* Delayed ack is sent */
	CA_EVENT_NON_DELAYED_ACK,
};

/* Information about inbound ACK, passed to cong_ops->in_ack_event() */
enum tcp_ca_ack_event_flags {
	CA_ACK_SLOWPATH		= (1 << 0),	/* In slow path processing */
	CA_ACK_WIN_UPDATE	= (1 << 1),	/* ACK updated window */
	CA_ACK_ECE		= (1 << 2),	/* ECE bit is set on ack */
};

/*
 * Interface for adding new TCP congestion control handlers
 */
#define TCP_CA_NAME_MAX	16
#define TCP_CA_MAX	128
#define TCP_CA_BUF_MAX	(TCP_CA_NAME_MAX*TCP_CA_MAX)

/* Algorithm can be set on socket without CAP_NET_ADMIN privileges */
#define TCP_CONG_NON_RESTRICTED 0x1
/* Requires ECN/ECT set on all packets */
#define TCP_CONG_NEEDS_ECN	0x2

struct tcp_congestion_ops {
	struct list_head	list;
	unsigned long flags;

	/* initialize private data (optional) */
	void (*init)(struct sock *sk);
	/* cleanup private data  (optional) */
	void (*release)(struct sock *sk);

	/* return slow start threshold (required) */
	u32 (*ssthresh)(struct sock *sk);
	/* do new cwnd calculation (required) */
	void (*cong_avoid)(struct sock *sk, u32 ack, u32 acked);
	/* call before changing ca_state (optional) */
	void (*set_state)(struct sock *sk, u8 new_state);
	/* call when cwnd event occurs (optional) */
	void (*cwnd_event)(struct sock *sk, enum tcp_ca_event ev);
	/* call when ack arrives (optional) */
	void (*in_ack_event)(struct sock *sk, u32 flags);
	/* new value of cwnd after loss (optional) */
	u32  (*undo_cwnd)(struct sock *sk);
	/* hook for packet ack accounting (optional) */
	void (*pkts_acked)(struct sock *sk, u32 num_acked, s32 rtt_us);
	/* get info for inet_diag (optional) */
	void (*get_info)(struct sock *sk, u32 ext, struct sk_buff *skb);

	char 		name[TCP_CA_NAME_MAX];
	struct module 	*owner;
};

int tcp_register_congestion_control(struct tcp_congestion_ops *type);
void tcp_unregister_congestion_control(struct tcp_congestion_ops *type);

void tcp_assign_congestion_control(struct sock *sk);
void tcp_init_congestion_control(struct sock *sk);
void tcp_cleanup_congestion_control(struct sock *sk);
int tcp_set_default_congestion_control(const char *name);
void tcp_get_default_congestion_control(char *name);
void tcp_get_available_congestion_control(char *buf, size_t len);
void tcp_get_allowed_congestion_control(char *buf, size_t len);
int tcp_set_allowed_congestion_control(char *allowed);
int tcp_set_congestion_control(struct sock *sk, const char *name);
void tcp_slow_start(struct tcp_sock *tp, u32 acked);
void tcp_cong_avoid_ai(struct tcp_sock *tp, u32 w);

u32 tcp_reno_ssthresh(struct sock *sk);
void tcp_reno_cong_avoid(struct sock *sk, u32 ack, u32 acked);
extern struct tcp_congestion_ops tcp_reno;

static inline bool tcp_ca_needs_ecn(const struct sock *sk)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);

	return icsk->icsk_ca_ops->flags & TCP_CONG_NEEDS_ECN;
}

static inline void tcp_set_ca_state(struct sock *sk, const u8 ca_state)
{
	struct inet_connection_sock *icsk = inet_csk(sk);

	if (icsk->icsk_ca_ops->set_state)
		icsk->icsk_ca_ops->set_state(sk, ca_state);
	icsk->icsk_ca_state = ca_state;
}

static inline void tcp_ca_event(struct sock *sk, const enum tcp_ca_event event)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);

	if (icsk->icsk_ca_ops->cwnd_event)
		icsk->icsk_ca_ops->cwnd_event(sk, event);
}

/* These functions determine how the current flow behaves in respect of SACK
 * handling. SACK is negotiated with the peer, and therefore it can vary
 * between different flows.
 *
 * tcp_is_sack - SACK enabled
 * tcp_is_reno - No SACK
 * tcp_is_fack - FACK enabled, implies SACK enabled
 */
static inline int tcp_is_sack(const struct tcp_sock *tp)
{
	return tp->rx_opt.sack_ok;
}

static inline bool tcp_is_reno(const struct tcp_sock *tp)
{
	return !tcp_is_sack(tp);
}

static inline bool tcp_is_fack(const struct tcp_sock *tp)
{
	return tp->rx_opt.sack_ok & TCP_FACK_ENABLED;
}

static inline void tcp_enable_fack(struct tcp_sock *tp)
{
	tp->rx_opt.sack_ok |= TCP_FACK_ENABLED;
}

/* TCP early-retransmit (ER) is similar to but more conservative than
 * the thin-dupack feature.  Enable ER only if thin-dupack is disabled.
 */
static inline void tcp_enable_early_retrans(struct tcp_sock *tp)
{
	tp->do_early_retrans = sysctl_tcp_early_retrans &&
		sysctl_tcp_early_retrans < 4 && !sysctl_tcp_thin_dupack &&
		sysctl_tcp_reordering == 3;
}

static inline void tcp_disable_early_retrans(struct tcp_sock *tp)
{
	tp->do_early_retrans = 0;
}

static inline unsigned int tcp_left_out(const struct tcp_sock *tp)
{
	return tp->sacked_out + tp->lost_out;
}

/* This determines how many packets are "in the network" to the best
 * of our knowledge.  In many cases it is conservative, but where
 * detailed information is available from the receiver (via SACK
 * blocks etc.) we can make more aggressive calculations.
 *
 * Use this for decisions involving congestion control, use just
 * tp->packets_out to determine if the send queue is empty or not.
 *
 * Read this equation as:
 *
 *	"Packets sent once on transmission queue" MINUS
 *	"Packets left network, but not honestly ACKed yet" PLUS
 *	"Packets fast retransmitted"
 */
static inline unsigned int tcp_packets_in_flight(const struct tcp_sock *tp)
{
	return tp->packets_out - tcp_left_out(tp) + tp->retrans_out;
}

#define TCP_INFINITE_SSTHRESH	0x7fffffff

static inline bool tcp_in_initial_slowstart(const struct tcp_sock *tp)
{
	return tp->snd_ssthresh >= TCP_INFINITE_SSTHRESH;
}

static inline bool tcp_in_cwnd_reduction(const struct sock *sk)
{
	return (TCPF_CA_CWR | TCPF_CA_Recovery) &
	       (1 << inet_csk(sk)->icsk_ca_state);
}

/* If cwnd > ssthresh, we may raise ssthresh to be half-way to cwnd.
 * The exception is cwnd reduction phase, when cwnd is decreasing towards
 * ssthresh.
 */
static inline __u32 tcp_current_ssthresh(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	if (tcp_in_cwnd_reduction(sk))
		return tp->snd_ssthresh;
	else
		return max(tp->snd_ssthresh,
			   ((tp->snd_cwnd >> 1) +
			    (tp->snd_cwnd >> 2)));
}

/* Use define here intentionally to get WARN_ON location shown at the caller */
#define tcp_verify_left_out(tp)	WARN_ON(tcp_left_out(tp) > tp->packets_out)

void tcp_enter_cwr(struct sock *sk);
__u32 tcp_init_cwnd(const struct tcp_sock *tp, const struct dst_entry *dst);

/* The maximum number of MSS of available cwnd for which TSO defers
 * sending if not using sysctl_tcp_tso_win_divisor.
 */
static inline __u32 tcp_max_tso_deferred_mss(const struct tcp_sock *tp)
{
	return 3;
}

/* Slow start with delack produces 3 packets of burst, so that
 * it is safe "de facto".  This will be the default - same as
 * the default reordering threshold - but if reordering increases,
 * we must be able to allow cwnd to burst at least this much in order
 * to not pull it back when holes are filled.
 */
static __inline__ __u32 tcp_max_burst(const struct tcp_sock *tp)
{
	return tp->reordering;
}

/* Returns end sequence number of the receiver's advertised window */
static inline u32 tcp_wnd_end(const struct tcp_sock *tp)
{
	return tp->snd_una + tp->snd_wnd;
}

/* We follow the spirit of RFC2861 to validate cwnd but implement a more
 * flexible approach. The RFC suggests cwnd should not be raised unless
 * it was fully used previously. And that's exactly what we do in
 * congestion avoidance mode. But in slow start we allow cwnd to grow
 * as long as the application has used half the cwnd.
 * Example :
 *    cwnd is 10 (IW10), but application sends 9 frames.
 *    We allow cwnd to reach 18 when all frames are ACKed.
 * This check is safe because it's as aggressive as slow start which already
 * risks 100% overshoot. The advantage is that we discourage application to
 * either send more filler packets or data to artificially blow up the cwnd
 * usage, and allow application-limited process to probe bw more aggressively.
 */
static inline bool tcp_is_cwnd_limited(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	/* If in slow start, ensure cwnd grows to twice what was ACKed. */
	if (tp->snd_cwnd <= tp->snd_ssthresh)
		return tp->snd_cwnd < 2 * tp->max_packets_out;

	return tp->is_cwnd_limited;
}

static inline void tcp_check_probe_timer(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	const struct inet_connection_sock *icsk = inet_csk(sk);

	if (!tp->packets_out && !icsk->icsk_pending)
		inet_csk_reset_xmit_timer(sk, ICSK_TIME_PROBE0,
					  icsk->icsk_rto, TCP_RTO_MAX);
}

static inline void tcp_init_wl(struct tcp_sock *tp, u32 seq)
{
	tp->snd_wl1 = seq;
}

static inline void tcp_update_wl(struct tcp_sock *tp, u32 seq)
{
	tp->snd_wl1 = seq;
}

/*
 * Calculate(/check) TCP checksum
 */
static inline __sum16 tcp_v4_check(int len, __be32 saddr,
				   __be32 daddr, __wsum base)
{
	return csum_tcpudp_magic(saddr,daddr,len,IPPROTO_TCP,base);
}

static inline __sum16 __tcp_checksum_complete(struct sk_buff *skb)
{
	return __skb_checksum_complete(skb);
}

static inline bool tcp_checksum_complete(struct sk_buff *skb)
{
	return !skb_csum_unnecessary(skb) &&
		__tcp_checksum_complete(skb);
}

/* Prequeue for VJ style copy to user, combined with checksumming. */

static inline void tcp_prequeue_init(struct tcp_sock *tp)
{
	tp->ucopy.task = NULL;
	tp->ucopy.len = 0;
	tp->ucopy.memory = 0;
	skb_queue_head_init(&tp->ucopy.prequeue);
}

bool tcp_prequeue(struct sock *sk, struct sk_buff *skb);
int tcp_filter(struct sock *sk, struct sk_buff *skb);

#undef STATE_TRACE

#ifdef STATE_TRACE
static const char *statename[]={
	"Unused","Established","Syn Sent","Syn Recv",
	"Fin Wait 1","Fin Wait 2","Time Wait", "Close",
	"Close Wait","Last ACK","Listen","Closing"
};
#endif
void tcp_set_state(struct sock *sk, int state);

void tcp_done(struct sock *sk);

int tcp_abort(struct sock *sk, int err);

static inline void tcp_sack_reset(struct tcp_options_received *rx_opt)
{
	rx_opt->dsack = 0;
	rx_opt->num_sacks = 0;
}

u32 tcp_default_init_rwnd(u32 mss);

/* Determine a window scaling and initial window to offer. */
void tcp_select_initial_window(int __space, __u32 mss, __u32 *rcv_wnd,
			       __u32 *window_clamp, int wscale_ok,
			       __u8 *rcv_wscale, __u32 init_rcv_wnd
#ifdef CONFIG_MPTCP
			       ,const struct sock *sk
#endif
			       			       			);

static inline int tcp_win_from_space(int space)
{
	int tcp_adv_win_scale = sysctl_tcp_adv_win_scale;

	return tcp_adv_win_scale <= 0 ?
		(space>>(-tcp_adv_win_scale)) :
		space - (space>>tcp_adv_win_scale);
}

#ifdef CONFIG_MPTCP
extern struct static_key mptcp_static_key;
static inline bool mptcp(const struct tcp_sock *tp)
{
	return static_key_false(&mptcp_static_key) && tp->mpc;
}
#else
static inline bool mptcp(const struct tcp_sock *tp)
{
	return 0;
}
#endif

/* Note: caller must be prepared to deal with negative returns */ 
static inline int tcp_space(const struct sock *sk)
{
	return tcp_win_from_space(sk->sk_rcvbuf -
				  atomic_read(&sk->sk_rmem_alloc));
} 

static inline int tcp_full_space(const struct sock *sk)
{
	return tcp_win_from_space(sk->sk_rcvbuf); 
}

static inline void tcp_openreq_init(struct request_sock *req,
				    struct tcp_options_received *rx_opt,
				    struct sk_buff *skb, struct sock *sk)
{
	struct inet_request_sock *ireq = inet_rsk(req);

	req->rcv_wnd = 0;		/* So that tcp_send_synack() knows! */
	req->cookie_ts = 0;
	tcp_rsk(req)->rcv_isn = TCP_SKB_CB(skb)->seq;
	tcp_rsk(req)->rcv_nxt = TCP_SKB_CB(skb)->seq + 1;
	tcp_rsk(req)->snt_synack = tcp_time_stamp;
	req->mss = rx_opt->mss_clamp;
	req->ts_recent = rx_opt->saw_tstamp ? rx_opt->rcv_tsval : 0;
	ireq->tstamp_ok = rx_opt->tstamp_ok;
	ireq->sack_ok = rx_opt->sack_ok;
	ireq->snd_wscale = rx_opt->snd_wscale;
	ireq->wscale_ok = rx_opt->wscale_ok;
	ireq->acked = 0;
	ireq->ecn_ok = 0;
#ifdef CONFIG_MPTCP
	ireq->mptcp_rqsk = 0;
	ireq->saw_mpc = 0;
#endif
	ireq->ir_rmt_port = tcp_hdr(skb)->source;
	ireq->ir_num = ntohs(tcp_hdr(skb)->dest);
	ireq->ir_mark = inet_request_mark(sk, skb);
}

extern void tcp_openreq_init_rwin(struct request_sock *req,
				  struct sock *sk, struct dst_entry *dst);

void tcp_enter_memory_pressure(struct sock *sk);

static inline int keepalive_intvl_when(const struct tcp_sock *tp)
{
	return tp->keepalive_intvl ? : sysctl_tcp_keepalive_intvl;
}

static inline int keepalive_time_when(const struct tcp_sock *tp)
{
	return tp->keepalive_time ? : sysctl_tcp_keepalive_time;
}

static inline int keepalive_probes(const struct tcp_sock *tp)
{
	return tp->keepalive_probes ? : sysctl_tcp_keepalive_probes;
}

static inline u32 keepalive_time_elapsed(const struct tcp_sock *tp)
{
	const struct inet_connection_sock *icsk = &tp->inet_conn;

	return min_t(u32, tcp_time_stamp - icsk->icsk_ack.lrcvtime,
			  tcp_time_stamp - tp->rcv_tstamp);
}

static inline int tcp_fin_time(const struct sock *sk)
{
	int fin_timeout = tcp_sk(sk)->linger2 ? : sysctl_tcp_fin_timeout;
	const int rto = inet_csk(sk)->icsk_rto;

	if (fin_timeout < (rto << 2) - (rto >> 1))
		fin_timeout = (rto << 2) - (rto >> 1);

	return fin_timeout;
}

static inline bool tcp_paws_check(const struct tcp_options_received *rx_opt,
				  int paws_win)
{
	if ((s32)(rx_opt->ts_recent - rx_opt->rcv_tsval) <= paws_win)
		return true;
	if (unlikely(get_seconds() >= rx_opt->ts_recent_stamp + TCP_PAWS_24DAYS))
		return true;
	/*
	 * Some OSes send SYN and SYNACK messages with tsval=0 tsecr=0,
	 * then following tcp messages have valid values. Ignore 0 value,
	 * or else 'negative' tsval might forbid us to accept their packets.
	 */
	if (!rx_opt->ts_recent)
		return true;
	return false;
}

static inline bool tcp_paws_reject(const struct tcp_options_received *rx_opt,
				   int rst)
{
	if (tcp_paws_check(rx_opt, 0))
		return false;

	/* RST segments are not recommended to carry timestamp,
	   and, if they do, it is recommended to ignore PAWS because
	   "their cleanup function should take precedence over timestamps."
	   Certainly, it is mistake. It is necessary to understand the reasons
	   of this constraint to relax it: if peer reboots, clock may go
	   out-of-sync and half-open connections will not be reset.
	   Actually, the problem would be not existing if all
	   the implementations followed draft about maintaining clock
	   via reboots. Linux-2.2 DOES NOT!

	   However, we can relax time bounds for RST segments to MSL.
	 */
	if (rst && get_seconds() >= rx_opt->ts_recent_stamp + TCP_PAWS_MSL)
		return false;
	return true;
}

static inline void tcp_mib_init(struct net *net)
{
	/* See RFC 2012 */
	TCP_ADD_STATS_USER(net, TCP_MIB_RTOALGORITHM, 1);
	TCP_ADD_STATS_USER(net, TCP_MIB_RTOMIN, TCP_RTO_MIN*1000/HZ);
	TCP_ADD_STATS_USER(net, TCP_MIB_RTOMAX, TCP_RTO_MAX*1000/HZ);
	TCP_ADD_STATS_USER(net, TCP_MIB_MAXCONN, -1);
}

/* from STCP */
static inline void tcp_clear_retrans_hints_partial(struct tcp_sock *tp)
{
	tp->lost_skb_hint = NULL;
}

static inline void tcp_clear_all_retrans_hints(struct tcp_sock *tp)
{
	tcp_clear_retrans_hints_partial(tp);
	tp->retransmit_skb_hint = NULL;
}

/* MD5 Signature */
struct crypto_hash;

union tcp_md5_addr {
	struct in_addr  a4;
#if IS_ENABLED(CONFIG_IPV6)
	struct in6_addr	a6;
#endif
};

/* - key database */
struct tcp_md5sig_key {
	struct hlist_node	node;
	u8			keylen;
	u8			family; /* AF_INET or AF_INET6 */
	union tcp_md5_addr	addr;
	u8			key[TCP_MD5SIG_MAXKEYLEN];
	struct rcu_head		rcu;
};

/* - sock block */
struct tcp_md5sig_info {
	struct hlist_head	head;
	struct rcu_head		rcu;
};

/* - pseudo header */
struct tcp4_pseudohdr {
	__be32		saddr;
	__be32		daddr;
	__u8		pad;
	__u8		protocol;
	__be16		len;
};

struct tcp6_pseudohdr {
	struct in6_addr	saddr;
	struct in6_addr daddr;
	__be32		len;
	__be32		protocol;	/* including padding */
};

union tcp_md5sum_block {
	struct tcp4_pseudohdr ip4;
#if IS_ENABLED(CONFIG_IPV6)
	struct tcp6_pseudohdr ip6;
#endif
};

/* - pool: digest algorithm, hash description and scratch buffer */
struct tcp_md5sig_pool {
	struct hash_desc	md5_desc;
	union tcp_md5sum_block	md5_blk;
};

/* - functions */
int tcp_v4_md5_hash_skb(char *md5_hash, struct tcp_md5sig_key *key,
			const struct sock *sk, const struct request_sock *req,
			const struct sk_buff *skb);
int tcp_md5_do_add(struct sock *sk, const union tcp_md5_addr *addr,
		   int family, const u8 *newkey, u8 newkeylen, gfp_t gfp);
int tcp_md5_do_del(struct sock *sk, const union tcp_md5_addr *addr,
		   int family);
struct tcp_md5sig_key *tcp_v4_md5_lookup(struct sock *sk,
					 struct sock *addr_sk);

#ifdef CONFIG_TCP_MD5SIG
struct tcp_md5sig_key *tcp_md5_do_lookup(struct sock *sk,
					 const union tcp_md5_addr *addr,
					 int family);
#define tcp_twsk_md5_key(twsk)	((twsk)->tw_md5_key)
#else
static inline struct tcp_md5sig_key *tcp_md5_do_lookup(struct sock *sk,
					 const union tcp_md5_addr *addr,
					 int family)
{
	return NULL;
}
#define tcp_twsk_md5_key(twsk)	NULL
#endif

bool tcp_alloc_md5sig_pool(void);

struct tcp_md5sig_pool *tcp_get_md5sig_pool(void);
static inline void tcp_put_md5sig_pool(void)
{
	local_bh_enable();
}

int tcp_md5_hash_header(struct tcp_md5sig_pool *, const struct tcphdr *);
int tcp_md5_hash_skb_data(struct tcp_md5sig_pool *, const struct sk_buff *,
			  unsigned int header_len);
int tcp_md5_hash_key(struct tcp_md5sig_pool *hp,
		     const struct tcp_md5sig_key *key);

/* From tcp_fastopen.c */
void tcp_fastopen_cache_get(struct sock *sk, u16 *mss,
			    struct tcp_fastopen_cookie *cookie, int *syn_loss,
			    unsigned long *last_syn_loss);
void tcp_fastopen_cache_set(struct sock *sk, u16 mss,
			    struct tcp_fastopen_cookie *cookie, bool syn_lost);
struct tcp_fastopen_request {
	/* Fast Open cookie. Size 0 means a cookie request */
	struct tcp_fastopen_cookie	cookie;
	struct msghdr			*data;  /* data in MSG_FASTOPEN */
	size_t				size;
	int				copied;	/* queued in tcp_connect() */
};
void tcp_free_fastopen_req(struct tcp_sock *tp);

extern struct tcp_fastopen_context __rcu *tcp_fastopen_ctx;
int tcp_fastopen_reset_cipher(void *key, unsigned int len);
bool tcp_try_fastopen(struct sock *sk, struct sk_buff *skb,
		      struct request_sock *req,
		      struct tcp_fastopen_cookie *foc,
		      struct dst_entry *dst);
void tcp_fastopen_init_key_once(bool publish);
#define TCP_FASTOPEN_KEY_LENGTH 16

/* Fastopen key context */
struct tcp_fastopen_context {
	struct crypto_cipher	*tfm;
	__u8			key[TCP_FASTOPEN_KEY_LENGTH];
	struct rcu_head		rcu;
};

/* write queue abstraction */
static inline void tcp_write_queue_purge(struct sock *sk)
{
	struct sk_buff *skb;

	while ((skb = __skb_dequeue(&sk->sk_write_queue)) != NULL)
		sk_wmem_free_skb(sk, skb);
	sk_mem_reclaim(sk);
	tcp_clear_all_retrans_hints(tcp_sk(sk));
	inet_csk(sk)->icsk_backoff = 0;
}

static inline struct sk_buff *tcp_write_queue_head(const struct sock *sk)
{
	return skb_peek(&sk->sk_write_queue);
}

static inline struct sk_buff *tcp_write_queue_tail(const struct sock *sk)
{
	return skb_peek_tail(&sk->sk_write_queue);
}

static inline struct sk_buff *tcp_write_queue_next(const struct sock *sk,
						   const struct sk_buff *skb)
{
	return skb_queue_next(&sk->sk_write_queue, skb);
}

static inline struct sk_buff *tcp_write_queue_prev(const struct sock *sk,
						   const struct sk_buff *skb)
{
	return skb_queue_prev(&sk->sk_write_queue, skb);
}

#define tcp_for_write_queue(skb, sk)					\
	skb_queue_walk(&(sk)->sk_write_queue, skb)

#define tcp_for_write_queue_from(skb, sk)				\
	skb_queue_walk_from(&(sk)->sk_write_queue, skb)

#define tcp_for_write_queue_from_safe(skb, tmp, sk)			\
	skb_queue_walk_from_safe(&(sk)->sk_write_queue, skb, tmp)

static inline struct sk_buff *tcp_send_head(const struct sock *sk)
{
	return sk->sk_send_head;
}

static inline bool tcp_skb_is_last(const struct sock *sk,
				   const struct sk_buff *skb)
{
	return skb_queue_is_last(&sk->sk_write_queue, skb);
}

static inline void tcp_advance_send_head(struct sock *sk, const struct sk_buff *skb)
{
	if (tcp_skb_is_last(sk, skb))
		sk->sk_send_head = NULL;
	else
		sk->sk_send_head = tcp_write_queue_next(sk, skb);
}

static inline void tcp_check_send_head(struct sock *sk, struct sk_buff *skb_unlinked)
{
	if (sk->sk_send_head == skb_unlinked)
		sk->sk_send_head = NULL;
	if (tcp_sk(sk)->highest_sack == skb_unlinked)
		tcp_sk(sk)->highest_sack = NULL;
}

static inline void tcp_init_send_head(struct sock *sk)
{
	sk->sk_send_head = NULL;
}

static inline void __tcp_add_write_queue_tail(struct sock *sk, struct sk_buff *skb)
{
	__skb_queue_tail(&sk->sk_write_queue, skb);
}

static inline void tcp_add_write_queue_tail(struct sock *sk, struct sk_buff *skb)
{
	__tcp_add_write_queue_tail(sk, skb);

	/* Queue it, remembering where we must start sending. */
	if (sk->sk_send_head == NULL) {
		sk->sk_send_head = skb;

		if (tcp_sk(sk)->highest_sack == NULL)
			tcp_sk(sk)->highest_sack = skb;
	}
}

static inline void __tcp_add_write_queue_head(struct sock *sk, struct sk_buff *skb)
{
	__skb_queue_head(&sk->sk_write_queue, skb);
}

/* Insert buff after skb on the write queue of sk.  */
static inline void tcp_insert_write_queue_after(struct sk_buff *skb,
						struct sk_buff *buff,
						struct sock *sk)
{
	__skb_queue_after(&sk->sk_write_queue, skb, buff);
}

/* Insert new before skb on the write queue of sk.  */
static inline void tcp_insert_write_queue_before(struct sk_buff *new,
						  struct sk_buff *skb,
						  struct sock *sk)
{
	__skb_queue_before(&sk->sk_write_queue, skb, new);

	if (sk->sk_send_head == skb)
		sk->sk_send_head = new;
}

static inline void tcp_unlink_write_queue(struct sk_buff *skb, struct sock *sk)
{
	__skb_unlink(skb, &sk->sk_write_queue);
}

static inline bool tcp_write_queue_empty(struct sock *sk)
{
	return skb_queue_empty(&sk->sk_write_queue);
}

static inline void tcp_push_pending_frames(struct sock *sk)
{
	if (tcp_send_head(sk)) {
		struct tcp_sock *tp = tcp_sk(sk);

		__tcp_push_pending_frames(sk, tcp_current_mss(sk), tp->nonagle);
	}
}

/* Start sequence of the skb just after the highest skb with SACKed
 * bit, valid only if sacked_out > 0 or when the caller has ensured
 * validity by itself.
 */
static inline u32 tcp_highest_sack_seq(struct tcp_sock *tp)
{
	if (!tp->sacked_out)
		return tp->snd_una;

	if (tp->highest_sack == NULL)
		return tp->snd_nxt;

	return TCP_SKB_CB(tp->highest_sack)->seq;
}

static inline void tcp_advance_highest_sack(struct sock *sk, struct sk_buff *skb)
{
	tcp_sk(sk)->highest_sack = tcp_skb_is_last(sk, skb) ? NULL :
						tcp_write_queue_next(sk, skb);
}

static inline struct sk_buff *tcp_highest_sack(struct sock *sk)
{
	return tcp_sk(sk)->highest_sack;
}

static inline void tcp_highest_sack_reset(struct sock *sk)
{
	tcp_sk(sk)->highest_sack = tcp_write_queue_head(sk);
}

/* Called when old skb is about to be deleted and replaced by new skb */
static inline void tcp_highest_sack_replace(struct sock *sk,
					    struct sk_buff *old,
					    struct sk_buff *new)
{
	if (old == tcp_highest_sack(sk))
		tcp_sk(sk)->highest_sack = new;
}

/* Determines whether this is a thin stream (which may suffer from
 * increased latency). Used to trigger latency-reducing mechanisms.
 */
static inline bool tcp_stream_is_thin(struct tcp_sock *tp)
{
	return tp->packets_out < 4 && !tcp_in_initial_slowstart(tp);
}

/* /proc */
enum tcp_seq_states {
	TCP_SEQ_STATE_LISTENING,
	TCP_SEQ_STATE_OPENREQ,
	TCP_SEQ_STATE_ESTABLISHED,
};

int tcp_seq_open(struct inode *inode, struct file *file);

struct tcp_seq_afinfo {
	char				*name;
	sa_family_t			family;
	const struct file_operations	*seq_fops;
	struct seq_operations		seq_ops;
};

struct tcp_iter_state {
	struct seq_net_private	p;
	sa_family_t		family;
	enum tcp_seq_states	state;
	struct sock		*syn_wait_sk;
	int			bucket, offset, sbucket, num;
	kuid_t			uid;
	loff_t			last_pos;
};

int tcp_proc_register(struct net *net, struct tcp_seq_afinfo *afinfo);
void tcp_proc_unregister(struct net *net, struct tcp_seq_afinfo *afinfo);

extern struct request_sock_ops tcp_request_sock_ops;
extern struct request_sock_ops tcp6_request_sock_ops;

void tcp_v4_destroy_sock(struct sock *sk);

struct sk_buff *tcp_gso_segment(struct sk_buff *skb,
				netdev_features_t features);
struct sk_buff **tcp_gro_receive(struct sk_buff **head, struct sk_buff *skb);
int tcp_gro_complete(struct sk_buff *skb);

void __tcp_v4_send_check(struct sk_buff *skb, __be32 saddr, __be32 daddr);

static inline u32 tcp_notsent_lowat(const struct tcp_sock *tp)
{
	return tp->notsent_lowat ?: sysctl_tcp_notsent_lowat;
}

static inline bool tcp_stream_memory_free(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	u32 notsent_bytes = tp->write_seq - tp->snd_nxt;

	return notsent_bytes < tcp_notsent_lowat(tp);
}

#ifdef CONFIG_PROC_FS
int tcp4_proc_init(void);
void tcp4_proc_exit(void);
#endif

int tcp_rtx_synack(struct sock *sk, struct request_sock *req);
int tcp_conn_request(struct request_sock_ops *rsk_ops,
		     const struct tcp_request_sock_ops *af_ops,
		     struct sock *sk, struct sk_buff *skb);

/* TCP af-specific functions */
struct tcp_sock_af_ops {
#ifdef CONFIG_TCP_MD5SIG
	struct tcp_md5sig_key	*(*md5_lookup) (struct sock *sk,
						struct sock *addr_sk);
	int			(*calc_md5_hash) (char *location,
						  struct tcp_md5sig_key *md5,
						  const struct sock *sk,
						  const struct request_sock *req,
						  const struct sk_buff *skb);
	int			(*md5_parse) (struct sock *sk,
					      char __user *optval,
					      int optlen);
#endif
};

#ifdef CONFIG_MPTCP
/* TCP/MPTCP-specific functions */
struct tcp_sock_ops {
	u32 (*__select_window)(struct sock *sk);
	u16 (*select_window)(struct sock *sk);
	void (*select_initial_window)(int __space, __u32 mss, __u32 *rcv_wnd,
				      __u32 *window_clamp, int wscale_ok,
				      __u8 *rcv_wscale, __u32 init_rcv_wnd,
				      const struct sock *sk);
	void (*init_buffer_space)(struct sock *sk);
	void (*set_rto)(struct sock *sk);
	bool (*should_expand_sndbuf)(const struct sock *sk);
	void (*send_fin)(struct sock *sk);
	bool (*write_xmit)(struct sock *sk, unsigned int mss_now, int nonagle,
			   int push_one, gfp_t gfp);
	void (*send_active_reset)(struct sock *sk, gfp_t priority);
	int (*write_wakeup)(struct sock *sk);
	bool (*prune_ofo_queue)(struct sock *sk);
	void (*retransmit_timer)(struct sock *sk);
	void (*time_wait)(struct sock *sk, int state, int timeo);
	void (*cleanup_rbuf)(struct sock *sk, int copied);
	void (*cwnd_validate)(struct sock *sk, bool is_cwnd_limited);
};
extern const struct tcp_sock_ops tcp_specific;
#endif

struct tcp_request_sock_ops {
	u16 mss_clamp;
#ifdef CONFIG_TCP_MD5SIG
	struct tcp_md5sig_key	*(*md5_lookup) (struct sock *sk,
						struct request_sock *req);
	int			(*calc_md5_hash) (char *location,
						  struct tcp_md5sig_key *md5,
						  const struct sock *sk,
						  const struct request_sock *req,
						  const struct sk_buff *skb);
#endif
#ifdef CONFIG_MPTCP
	int (*init_req)(struct request_sock *req, struct sock *sk,
			 struct sk_buff *skb, bool want_cookie);
#else
	void (*init_req)(struct request_sock *req, struct sock *sk,
			 struct sk_buff *skb);
#endif
#ifdef CONFIG_SYN_COOKIES
	__u32 (*cookie_init_seq)(struct sock *sk, const struct sk_buff *skb,
				 __u16 *mss);
#endif
	struct dst_entry *(*route_req)(struct sock *sk, struct flowi *fl,
				       const struct request_sock *req,
				       bool *strict);
	__u32 (*init_seq)(const struct sk_buff *skb);
	int (*send_synack)(struct sock *sk, struct dst_entry *dst,
			   struct flowi *fl, struct request_sock *req,
			   u16 queue_mapping, struct tcp_fastopen_cookie *foc);
	void (*queue_hash_add)(struct sock *sk, struct request_sock *req,
			       const unsigned long timeout);
};

#ifdef CONFIG_SYN_COOKIES
static inline __u32 cookie_init_sequence(const struct tcp_request_sock_ops *ops,
					 struct sock *sk, struct sk_buff *skb,
					 __u16 *mss)
{
	return ops->cookie_init_seq(sk, skb, mss);
}
#else
static inline __u32 cookie_init_sequence(const struct tcp_request_sock_ops *ops,
					 struct sock *sk, struct sk_buff *skb,
					 __u16 *mss)
{
	return 0;
}
#endif

int tcpv4_offload_init(void);

void tcp_v4_init(void);
void tcp_init(void);

/*
 * Save and compile IPv4 options, return a pointer to it
 */
static inline struct ip_options_rcu *tcp_v4_save_options(struct sk_buff *skb)
{
	const struct ip_options *opt = &TCP_SKB_CB(skb)->header.h4.opt;
	struct ip_options_rcu *dopt = NULL;

	if (opt->optlen) {
		int opt_size = sizeof(*dopt) + opt->optlen;

		dopt = kmalloc(opt_size, GFP_ATOMIC);
		if (dopt && __ip_options_echo(&dopt->opt, skb, opt)) {
			kfree(dopt);
			dopt = NULL;
		}
	}
	return dopt;
}

#endif	/* _TCP_H */
