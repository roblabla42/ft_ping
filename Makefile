# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rlambert <rlambert@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2014/11/03 11:25:08 by rlambert          #+#    #+#              #
#    Updated: 2017/01/19 17:34:46 by roblabla         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CFLAGS += -Wall -Wextra -Werror -Ilibft/include/ -Iinclude/

SRCS = src/ping.c \

OBJS = $(SRCS:.c=.o)

RM = rm -f

all: ft_ping

libft/libft.a:
	$(MAKE) -C libft/

.PHONY: libft/libft.a

ft_ping: libft/libft.a $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -Llibft/ -lft

clean:
	$(MAKE) -C libft/ clean
	$(RM) $(SRV_OBJS)
	$(RM) $(CLI_OBJS)

fclean: clean
	$(MAKE) -C libft/ fclean
	$(RM) ft_ping

re: fclean all

.PHONY: clean fclean re all
