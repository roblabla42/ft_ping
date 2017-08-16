#ifndef PING_H
# define PING_H
typedef struct	ping_s {
	struct timeval	sent;
	struct timeval	recv;
	int				seq;
	size_t			size;
	unsigned char	ttl;
}				ping_t;
#endif
