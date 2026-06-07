NAME     = ft_ascii_caster

CC       = cc
CFLAGS   = -Wall -Wextra -Werror
IFLAGS   = -I includes
LFLAGS   = -lm

SRCS     = src/main.c \
           src/parse_map.c \
           src/utils.c

OBJS     = $(SRCS:.c=.o)

# ── Rules ────────────────────────────────────────────────
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LFLAGS) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
