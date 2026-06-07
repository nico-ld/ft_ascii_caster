#ifndef FT_CASTER_H
# define FT_CASTER_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <fcntl.h>
# include <unistd.h>
# include <errno.h>
# include <math.h>
# include <termios.h>

/* ── Map limits ─────────────────────────────────────────── */
# define MAX_MAP_HEIGHT  256
# define MAX_MAP_WIDTH   256
# define READ_BUF_SIZE   4096

/* ── Valid cell characters ──────────────────────────────── */
# define CHAR_WALL       '1'
# define CHAR_FLOOR      '0'
# define VALID_PLAYERS   "NSEW"

/* ── Player directions ──────────────────────────────────── */
typedef enum e_dir
{
	DIR_NORTH = 0,
	DIR_SOUTH,
	DIR_EAST,
	DIR_WEST,
}	t_dir;

/* ── Player state ───────────────────────────────────────── */
typedef struct s_player
{
	double	x;       /* world-space column (float centre of cell) */
	double	y;       /* world-space row */
	t_dir	dir;
}	t_player;

/* ── Parsed map ─────────────────────────────────────────── */
typedef struct s_map
{
	char		**grid;   /* NULL-terminated array of row strings       */
	int			height;
	int			width;    /* width of the widest row (for bounds checks) */
	t_player	player;
	int			player_found;
}	t_map;

/* ── Game state (will grow in later steps) ──────────────── */
typedef struct s_game
{
	t_map	map;
}	t_game;

/* ── parse_map.c ────────────────────────────────────────── */
int		parse_map(const char *path, t_map *map);
void	free_map(t_map *map);

/* ── utils.c ────────────────────────────────────────────── */
char	*ft_strdup(const char *s);
char	*ft_strtrim_nl(char *s);
int		ft_strlen(const char *s);
void	ft_free_strarr(char **arr);
void	error_exit(t_map *map, const char *msg);

#endif
