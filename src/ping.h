/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rlambert <rlambert@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/08/21 10:21:37 by rlambert          #+#    #+#             */
/*   Updated: 2017/08/21 10:30:41 by rlambert         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PING_H
# define PING_H

typedef struct	s_ping {
	struct timeval	sent;
	struct timeval	recv;
	int				seq;
	size_t			size;
	unsigned char	ttl;
}				t_ping;

typedef struct	s_opts
{
	int		help;
	int		verbose;
	int		v4;
	int		v6;
	int		numeric_output;
	char	*host;
}				t_opts;

#endif
