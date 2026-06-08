#ifndef FT_CASTER_BONUS_H
# define FT_CASTER_BONUS_H

# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <unistd.h>
# include <math.h>
# include <termios.h>

/* ── Compile-time tuning ────────────────────────────────── */
# define READ_BUF_SIZE      4096
# define DEFAULT_SCREEN_W   120
# define DEFAULT_SCREEN_H   40
# define FOV                1.0471975512
# define HALF_FOV           0.5235987756
# define MOVE_SPEED         0.05
# define ROT_SPEED          0.04
# define FRAME_US           33333

/* ── Map cell identifiers ───────────────────────────────── */
# define CHAR_WALL          '1'
# define CHAR_FLOOR         '0'

/* ── Wall shading palette (near → far) ─────────────────── */
# define SHADE_CHARS        "@#Ox."
# define SHADE_D0           1.5
# define SHADE_D1           3.0
# define SHADE_D2           5.5
# define SHADE_D3           9.0

/* ── ANSI escape helpers ────────────────────────────────── */
# define ANSI_CLEAR         "\033[2J"
# define ANSI_HOME          "\033[H"
# define ANSI_CURSOR_HIDE   "\033[?25l"
# define ANSI_CURSOR_SHOW   "\033[?25h"
# define ANSI_RESET         "\033[0m"

/*
** ── Directional wall colours (ANSI 256-colour foreground) ──
**
** Each cardinal face of a wall gets a distinct hue so the player
** can orient themselves by colour alone:
**
**   NORTH  →  cold blue   (colour 33  – dodger blue)
**   SOUTH  →  warm red    (colour 160 – bright red)
**   EAST   →  yellow      (colour 220 – gold)
**   WEST   →  green       (colour 34  – medium green)
**
** The escape sequence  \033[38;5;<n>m  sets the foreground to
** 256-colour index <n>.  \033[0m  resets all attributes.
**
** We emit the colour prefix once at the start of each wall cell
** and a reset at the end.  Because every cell in the row buffer
** is a single ASCII character, we write the ANSI codes and the
** character separately via the line-assembly routine in screen.c.
*/
# define COLOR_NORTH        "\033[38;5;33m"    /* blue    */
# define COLOR_SOUTH        "\033[38;5;160m"   /* red     */
# define COLOR_EAST         "\033[38;5;220m"   /* yellow  */
# define COLOR_WEST         "\033[38;5;34m"    /* green   */
# define COLOR_RESET        "\033[0m"

/* ── Wall face ──────────────────────────────────────────── */
typedef enum e_face
{
	FACE_NONE  = 0,
	FACE_NORTH,
	FACE_SOUTH,
	FACE_EAST,
	FACE_WEST,
}	t_face;

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
	double	x;
	double	y;
	double	angle;
	t_dir	dir;
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
/*
** The bonus screen adds a face_buf parallel to buf:
** face_buf[y * w + x] stores the t_face of the wall drawn at (x,y),
** or FACE_NONE for ceiling/floor cells.
** screen_flush uses this to emit ANSI colour codes around wall chars.
*/
typedef struct s_screen
{
	int		w;
	int		h;
	char	*buf;       /* shade characters, w*h cells                 */
	t_face	*face_buf;  /* wall face per cell, same dimensions         */
	int		buf_size;
}	t_screen;

/* ── DDA ray result (one per screen column) ─────────────── */
typedef struct s_ray
{
	double	dist;
	int		wall_h;
	int		wall_top;
	int		wall_bot;
	char	shade;
	t_face	face;       /* which cardinal face was hit                 */
}	t_ray;

/* ── Input flags ────────────────────────────────────────── */
typedef struct s_input
{
	int	left;
	int	right;
	int	forward;
	int	backward;
	int	strafe_left;
	int	strafe_right;
	int	quit;
}	t_input;

/* ── Saved terminal state ───────────────────────────────── */
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
	t_player	player;
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
void	*ft_memset(void *b, int c, int len);
void	*ft_memcpy(void *dst, const void *src, int n);
int		ft_strcmp(const char *s1, const char *s2);

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
