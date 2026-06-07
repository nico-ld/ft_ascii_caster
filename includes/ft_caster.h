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
# include <signal.h>
# include <sys/ioctl.h>

/* ── Compile-time tuning ────────────────────────────────── */
# define READ_BUF_SIZE      4096
# define DEFAULT_SCREEN_W   120
# define DEFAULT_SCREEN_H   40
# define FOV                1.0471975512   /* 60 degrees in radians          */
# define HALF_FOV           0.5235987756   /* 30 degrees in radians          */
# define MOVE_SPEED         0.05           /* world units per frame          */
# define ROT_SPEED          0.04           /* radians per frame              */
# define FRAME_US           33333          /* ~30 fps  (microseconds)        */

/* ── Map cell identifiers ───────────────────────────────── */
# define CHAR_WALL          '1'
# define CHAR_FLOOR         '0'

/* ── Wall shading palette (near → far) ─────────────────── */
# define SHADE_COUNT        5
# define SHADE_CHARS        "@#Ox."
/* distance thresholds that trigger each shade (world units) */
# define SHADE_D0           1.5
# define SHADE_D1           3.0
# define SHADE_D2           5.5
# define SHADE_D3           9.0
/* beyond D3 → last char in SHADE_CHARS */

/* ── ANSI escape helpers ────────────────────────────────── */
# define ANSI_CLEAR         "\033[2J"
# define ANSI_HOME          "\033[H"
# define ANSI_CURSOR_HIDE   "\033[?25l"
# define ANSI_CURSOR_SHOW   "\033[?25h"
# define ANSI_RESET         "\033[0m"

/* ── Player directions (initial facing from map char) ───── */
typedef enum e_dir
{
	DIR_NORTH = 0,
	DIR_SOUTH,
	DIR_EAST,
	DIR_WEST,
}	t_dir;

/* ── Player ─────────────────────────────────────────────── */
typedef struct s_player
{
	double	x;       /* world-space position (centre of starting cell) */
	double	y;
	double	angle;   /* current facing angle in radians                */
	t_dir	dir;     /* kept for map-init reference                    */
}	t_player;

/* ── Parsed map ─────────────────────────────────────────── */
typedef struct s_map
{
	char		**grid;
	int			height;
	int			width;
	t_player	player;
	int			player_found;
}	t_map;

/* ── Terminal / screen ──────────────────────────────────── */
typedef struct s_screen
{
	int		w;           /* columns                                      */
	int		h;           /* rows                                         */
	char	*buf;        /* framebuffer: w * h + newlines + NUL          */
	int		buf_size;
}	t_screen;

/* ── DDA ray result (one per screen column) ─────────────── */
typedef struct s_ray
{
	double	dist;        /* corrected perpendicular wall distance        */
	int		wall_h;      /* pixel height of the wall slice               */
	int		wall_top;    /* first screen row of the wall slice           */
	int		wall_bot;    /* last  screen row of the wall slice           */
	char	shade;       /* ASCII character to draw for this wall slice  */
}	t_ray;

/* ── Input flags (bitfield set by read_input) ───────────── */
typedef struct s_input
{
	int	left;
	int	right;
	int	forward;
	int	backward;
	int	quit;
}	t_input;

/* ── Saved terminal state for restore-on-exit ───────────── */
typedef struct s_term
{
	struct termios	original;
	int				raw_active;
}	t_term;

/* ── Top-level game state ───────────────────────────────── */
typedef struct s_game
{
	t_map		map;
	t_screen	screen;
	t_player	player;   /* live copy, updated each frame               */
	t_term		term;
}	t_game;

/* ── parse_map.c ────────────────────────────────────────── */
int		parse_map(const char *path, t_map *map);
void	free_map(t_map *map);

/* ── utils.c ────────────────────────────────────────────── */
char	*ft_strdup(const char *s);
int		ft_strlen(const char *s);
void	ft_free_strarr(char **arr);
void	error_exit(t_map *map, const char *msg);

/* ── screen.c ───────────────────────────────────────────── */
int		screen_init(t_screen *sc);
void	screen_free(t_screen *sc);
void	screen_clear_buf(t_screen *sc);
void	screen_flush(t_screen *sc);

/* ── term.c ─────────────────────────────────────────────── */
void	term_set_raw(t_term *t);
void	term_restore(t_term *t);

/* ── render.c ───────────────────────────────────────────── */
void	render_frame(t_game *g);

/* ── input.c ────────────────────────────────────────────── */
void	read_input(t_input *in);

/* ── game.c ─────────────────────────────────────────────── */
void	run_game(t_game *g);

#endif
