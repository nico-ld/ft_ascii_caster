#include "../includes/ft_caster.h"

/*
** ─────────────────────────────────────────────────────────────────────────────
**  GAME LOOP
**
**  Each iteration:
**    1. read_input  – non-blocking stdin drain (raw mode, VMIN=0 VTIME=0)
**    2. update      – apply movement / rotation to g->player
**    3. render      – cast rays → fill framebuffer
**    4. flush       – one write() to stdout
**    5. usleep      – cap to ~30 fps
** ─────────────────────────────────────────────────────────────────────────────
*/

/* ── Angle conversion ───────────────────────────────────── */

/*
** dir_to_angle – converts the map spawn direction to radians.
** Convention: angle 0 = East (+x), counter-clockwise positive.
**   East  → 0       West  → π
**   North → π/2     South → 3π/2
*/
static double	dir_to_angle(t_dir dir)
{
	if (dir == DIR_NORTH)
		return (M_PI / 2.0);
	if (dir == DIR_SOUTH)
		return (3.0 * M_PI / 2.0);
	if (dir == DIR_WEST)
		return (M_PI);
	return (0.0);
}

/* ── Collision ──────────────────────────────────────────── */

/*
** is_wall – returns 1 if world-space (wx, wy) lands in a wall cell or
** outside the map grid.  Used for per-axis collision so the player
** can slide along walls rather than stopping dead.
*/
static int	is_wall(t_map *map, double wx, double wy)
{
	int	mx;
	int	my;

	mx = (int)wx;
	my = (int)wy;
	if (my < 0 || my >= map->height || mx < 0)
		return (1);
	if (mx >= ft_strlen(map->grid[my]))
		return (1);
	return (map->grid[my][mx] == CHAR_WALL);
}

/* ── Player update ──────────────────────────────────────── */

/*
** update_player – applies one frame of input.
**
** Rotation: left arrow / A → angle += ROT_SPEED (counter-clockwise = left)
**           right arrow / D → angle -= ROT_SPEED
**
** Movement: W/S along the current facing direction.
**           Axes are tested independently (wall-sliding collision).
**
** Angle is wrapped to [0, 2π) each frame to prevent float drift.
*/
static void	update_player(t_game *g, t_input *in)
{
	double	dx;
	double	dy;
	double	nx;
	double	ny;

	if (in->left)
		g->player.angle += ROT_SPEED;
	if (in->right)
		g->player.angle -= ROT_SPEED;
	if (g->player.angle < 0.0)
		g->player.angle += 2.0 * M_PI;
	if (g->player.angle >= 2.0 * M_PI)
		g->player.angle -= 2.0 * M_PI;

	dx = cos(g->player.angle) * MOVE_SPEED;
	dy = -sin(g->player.angle) * MOVE_SPEED;	/* row 0 = top → invert y */
	if (in->forward)
	{
		nx = g->player.x + dx;
		ny = g->player.y + dy;
		if (!is_wall(&g->map, nx, g->player.y))
			g->player.x = nx;
		if (!is_wall(&g->map, g->player.x, ny))
			g->player.y = ny;
	}
	if (in->backward)
	{
		nx = g->player.x - dx;
		ny = g->player.y - dy;
		if (!is_wall(&g->map, nx, g->player.y))
			g->player.x = nx;
		if (!is_wall(&g->map, g->player.x, ny))
			g->player.y = ny;
	}
}

/* ── Cleanup & signal handling ──────────────────────────── */

/*
** g_game is a module-level pointer used exclusively by the signal handler.
** No other code should access it; all normal paths use the local 'g'.
*/
static t_game	*g_game = NULL;

static void	cleanup(t_game *g)
{
	term_restore(&g->term);
	screen_free(&g->screen);
	free_map(&g->map);
	write(STDOUT_FILENO, ANSI_CLEAR, sizeof(ANSI_CLEAR) - 1);
	write(STDOUT_FILENO, ANSI_HOME, sizeof(ANSI_HOME) - 1);
}

/*
** sig_handler – called on SIGINT (Ctrl+C) or SIGTERM.
** Restores the terminal so the shell is usable after a forced quit,
** then exits with code 0.  Only async-signal-safe calls are used here:
** tcsetattr, write, _exit.
*/
static void	sig_handler(int sig)
{
	(void)sig;
	if (g_game)
		cleanup(g_game);
	_exit(EXIT_SUCCESS);
}

/* ── Public entry point ─────────────────────────────────── */

void	run_game(t_game *g)
{
	t_input	in;

	/* Register signal handler before entering raw mode,
	** so any early Ctrl+C is also caught cleanly.         */
	g_game = g;
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	/* Copy parsed spawn into the live player and compute facing angle */
	g->player = g->map.player;
	g->player.angle = dir_to_angle(g->map.player.dir);

	/* Allocate framebuffer (depends on terminal size) */
	if (screen_init(&g->screen) < 0)
	{
		free_map(&g->map);
		write(STDERR_FILENO, "Error\nFailed to allocate screen buffer.\n", 40);
		exit(EXIT_FAILURE);
	}

	/* Switch terminal to raw mode (non-blocking reads, no echo) */
	term_set_raw(&g->term);

	/* Full clear before first frame so no leftover shell text shows */
	write(STDOUT_FILENO, ANSI_CLEAR, sizeof(ANSI_CLEAR) - 1);

	while (1)
	{
		read_input(&in);
		if (in.quit)
			break ;
		update_player(g, &in);
		screen_clear_buf(&g->screen);
		render_frame(g);
		screen_flush(&g->screen);
		usleep(FRAME_US);
	}

	cleanup(g);
}
