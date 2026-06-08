NAME         = ft_ascii_caster
NAME_BONUS   = ft_ascii_caster_bonus

CC           = cc
CFLAGS       = -Wall -Wextra -Werror
LFLAGS       = -lm

# ── Mandatory ────────────────────────────────────────────
SRCS         = src/main.c      \
               src/parse_map.c \
               src/utils.c     \
               src/term.c      \
               src/screen.c    \
               src/input.c     \
               src/render.c    \
               src/game.c

OBJS         = $(SRCS:.c=.o)
IFLAGS       = -I includes

# ── Bonus ────────────────────────────────────────────────
SRCS_BONUS   = src_bonus/main.c      \
               src_bonus/parse_map.c \
               src_bonus/utils.c     \
               src_bonus/term.c      \
               src_bonus/screen.c    \
               src_bonus/input.c     \
               src_bonus/render.c    \
               src_bonus/game.c

OBJS_BONUS   = $(SRCS_BONUS:.c=.o)
IFLAGS_BONUS = -I includes_bonus

# ── Rules ────────────────────────────────────────────────
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LFLAGS) -o $(NAME)

bonus: $(NAME_BONUS)

$(NAME_BONUS): $(OBJS_BONUS)
	$(CC) $(CFLAGS) $(OBJS_BONUS) $(LFLAGS) -o $(NAME_BONUS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

src_bonus/%.o: src_bonus/%.c
	$(CC) $(CFLAGS) $(IFLAGS_BONUS) -c $< -o $@

clean:
	rm -f $(OBJS) $(OBJS_BONUS)

fclean: clean
	rm -f $(NAME) $(NAME_BONUS)

re: fclean all

.PHONY: all bonus clean fclean re
