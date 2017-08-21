#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include "libft.h"
#include "ping.h"

#define PAYLOAD_LEN sizeof(struct timeval)
#define PACKET_LEN (sizeof(struct icmphdr) + PAYLOAD_LEN)
#define MAX_DNSLEN 1024
#define HELP_STR "Usage: ping [-hvn] destination\n"

typedef struct	s_opts
{
	int help;
	int verbose;
	int v4;
	int v6;
	int numeric_output;
	char *host;
}		t_opts;

short	ft_htons(short num)
{
	return num >> 8 | num << 8;
}

const char	*addr2str(struct sockaddr *addr, socklen_t addrlen, int nameinfo, char **addr_str)
{
	const void	*res;
	char		ip_addr[INET6_ADDRSTRLEN];
	char		host_addr[MAX_DNSLEN];


	(void)addrlen;
	(void)nameinfo;
	if (addr->sa_family == AF_INET)
		res = inet_ntop(addr->sa_family, &((struct sockaddr_in*)addr)->sin_addr, ip_addr, INET6_ADDRSTRLEN);
	else if (addr->sa_family == AF_INET6)
		res = inet_ntop(addr->sa_family, &((struct sockaddr_in6*)addr)->sin6_addr, ip_addr, INET6_ADDRSTRLEN);
	else
		return (NULL);
	if (res == NULL)
		return (NULL);
	if (nameinfo && getnameinfo(addr, addrlen, host_addr, MAX_DNSLEN, NULL, 0, 0) == 0) {
		*addr_str = ft_multistrjoin(4, host_addr, " (", ip_addr, ")");
	} else {
		*addr_str = ft_strdup(ip_addr);
	}
	return *addr_str;
}

uint32_t ft_cksum(char *buf, size_t size)
{
	uint32_t sum = 0;
	uint32_t i = 0;

	while (i < size - 1) {
		sum += *((uint16_t*)(buf + i));
		i += 2;
	}
	if (size & 1)  {
		sum += buf[i];
	}
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	return (~sum);
}

int		connect_sock(struct sockaddr *addr)
{
	int sock;
	int proto;

	if (addr->sa_family == AF_INET)
		proto = IPPROTO_ICMP;
	else
		proto = IPPROTO_ICMPV6;
	if ((sock = socket(addr->sa_family, SOCK_RAW, proto)) < 0)
	{
		perror("ft_ping: icmp open socket");
		return -1;
	}
	if (connect(sock, addr, sizeof(struct sockaddr)) < 0)
	{
		perror("ft_ping: icmp connect socket");
		return -1;
	}
	return sock;
}

int		setup_sock(int sock) {
	// TODO: TTL ?
	struct timeval	tv;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0) {
		perror("ft_ping: setsockopt");
		return -1;
	}
	return 0;
}

unsigned short	checksum(void *msg, size_t msg_len)
{
	short *data = msg;
	size_t i;
	unsigned short sum;

	i = 0;
	sum = 0;
	while (i < msg_len)
	{
		sum += data[i];
		i++;
	}
	return (sum);
}

int		receive_ping(int sock, int seq, ping_t *ping)
{
	struct msghdr	msghdr;
	struct iovec	iov;
	char			msg[sizeof(struct ip) + PACKET_LEN];
	struct ip		*ip;
	struct icmphdr	*icmp;
	char			ttl_buf[sizeof(struct cmsghdr) + 4]; // TODO: Bleh

	iov.iov_base = &msg;
	iov.iov_len = sizeof(msg);
	ft_bzero(&msghdr, sizeof(struct msghdr));
	msghdr.msg_iov = &iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = ttl_buf;
	msghdr.msg_controllen = sizeof(ttl_buf);
	ip = (struct ip*)msg;
	icmp = (struct icmphdr*)(msg + sizeof(struct ip));
	icmp->type = ICMP_ECHO;
	while (icmp->type != ICMP_ECHOREPLY || ntohs(icmp->un.echo.sequence) != seq) {
		if (recvmsg(sock, &msghdr, 0) < 0)
		{
			if (errno == EAGAIN) {
				return 2;
			} else {
				perror("error recvmsg");
				return FALSE;
			}
		}
	}
	if (gettimeofday(&ping->recv, NULL) < 0)
	{
		perror("error gettimeofday");
		return FALSE;
	}
	ping->sent = *((struct timeval*)(msg + sizeof(struct ip) + sizeof(struct icmphdr)));
	ping->seq = seq;
	ping->size = iov.iov_len;
	ping->ttl = ip->ip_ttl;
	return TRUE;
}


int		send_ping(int sock, struct sockaddr *addr, socklen_t len, ping_t *ping)
{
	char			msg[PACKET_LEN];
	struct icmphdr	*icmp;
	static short	seq = 0;

	ft_bzero(msg, PACKET_LEN);
	icmp = (struct icmphdr*)msg;
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->un.echo.sequence = ft_htons(seq);
	icmp->un.echo.id = 12;
	if (gettimeofday((struct timeval*)(msg + sizeof(struct icmphdr)), NULL) < 0) {
		perror("error gettimeofday");
		return FALSE;
	}
	icmp->checksum = 0;
	icmp->checksum = ft_cksum(msg, sizeof(msg));
	if (sendto(sock, msg, sizeof(msg), 0, addr, len) < 0)
	{
		perror("ft_ping: icmp send packet");
		return FALSE;
	}
	return receive_ping(sock, seq++, ping);
}

int	parse_args(t_opts *opts, int argc, char **argv)
{
	int i;
	int j;

	ft_bzero(opts, sizeof(t_opts));
	if (argc <= 1) {
		opts->help = 1;
	}
	i = 1;
	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			j = 1;
			while (argv[i][j] != '\0')
			{
				switch (argv[i][j])
				{
					case 'h':
						opts->help = 1;
						break;
					case 'v':
						opts->verbose = 1;
						break;
					case '6':
						opts->v6 = 1;
						break;
					case '4':
						opts->v4 = 1;
						break;
					case 'n':
						opts->numeric_output = 1;
						break;
				}
				j++;
			}
		} else if (opts->host == NULL) {
			opts->host = argv[i];
		} else {
			opts->help = 1;
		}
		i++;
	}
	if (opts->host == NULL) {
		opts->help = 1;
	}
	return (0);
}

static volatile int running = 1;

void int_handler(int dummy) {
	(void)dummy;

	running = 0;
}

double	sub_ms(struct timeval v1, struct timeval v2) {
	return (v1.tv_sec - v2.tv_sec) * 1000 + (v1.tv_usec - v2.tv_usec) / 1000.0;
}

int	ping_loop(int sock, struct addrinfo *addr_out, t_opts *opts, char *addr_str)
{
	ping_t		ping;
	int	res;
	unsigned	packet_transmitted = 0;
	unsigned	packet_received = 0;
	struct timeval	start_time, end_time;

	signal(SIGINT, int_handler);
	printf("PING %s (%s) %lu(%lu) bytes of data.\n", opts->host, addr_str, PAYLOAD_LEN, PACKET_LEN);
	gettimeofday(&start_time, NULL);
	while (running) {
		res = send_ping(sock, addr_out->ai_addr, addr_out->ai_addrlen, &ping);
		packet_transmitted++;
		if (res == 2) {
			// Timed out
		} else if (res) {
			packet_received++;
			printf("%lu bytes from %s: icmp_seq=%u ttl=%u time=%.2f ms\n", ping.size, addr_str, ping.seq, ping.ttl, sub_ms(ping.recv, ping.sent));
		} else {
			return (1);
		}
		sleep(1);
	}
	gettimeofday(&end_time, NULL);
	printf("\n--- %s ping statistics ---\n", opts->host);
	printf("%u packets transmitted, %u received, %u%% packet loss, time %.0fms\n", packet_transmitted, packet_received, 100 - (packet_received * 100 / packet_transmitted), sub_ms(end_time, start_time));
	//printf("%u min/avg/max/mdev = %u/%u/%u/%u\n", min_ping, avg_ping, max_ping, deriv_ping);
	return (0);
}

int	get_family(t_opts *opts)
{
	if (opts->v4)
		return AF_INET;
	else if (opts->v6)
		return AF_INET6;
	else
		return AF_UNSPEC;
}

int	ft_getaddr(t_opts *opts, struct addrinfo *out) {
	struct addrinfo hints = {0};
	struct addrinfo *addr_out;

	hints.ai_family = get_family(opts);
	if (getaddrinfo(opts->host, NULL, &hints, &addr_out) != 0) {
		perror("error getaddrinfo");
		return (1);
	}
	while (addr_out != NULL && (addr_out->ai_family != AF_INET && addr_out->ai_family != AF_INET6)) {
		addr_out = addr_out->ai_next;
	}
	if (addr_out == NULL) {
		printf("error getaddrinfo: no ip info\n");
		return (1);
	}
	*out = *addr_out;
	return (0);
}


int	main(int argc, char **argv)
{
	struct addrinfo	addr_out;
	int		sock;
	t_opts		opts;
	char		*addr_str;

	parse_args(&opts, argc, argv);
	if (opts.help)
	{
		printf(HELP_STR);
		return (2);
	}
	if (ft_getaddr(&opts, &addr_out)) {
		return (1);
	}
	if (addr2str(addr_out.ai_addr, addr_out.ai_addrlen, !opts.numeric_output, &addr_str) == NULL) {
		perror("error inet_ntop");
		return (1);
	}

	if ((sock = connect_sock(addr_out.ai_addr)) <= 0)
		return (1);

	if (setup_sock(sock) < 0)
		return (1);

	return (ping_loop(sock, &addr_out, &opts, addr_str));
}
