#include "../includes/ft_caster.h"

/*
** ─────────────────────────────────────────────────────────────────────────────
**  GAME LOOP
**
**  Each iteration:
**    1. read_input  – non-blocking stdin drain
**    2. update      – apply movement / rotation to g->player
**    3. render      – cast rays → fill framebuffer
**    4. flush       – one write() to stdout
**    5. usleep      – cap to ~30 fps
** ─────────────────────────────────────────────────────────────────────────────
*/

/* ── Angle utilities ────────────────────────────────────── */

/*
** init_angle – converts the map spawn direction (N/S/E/W) to radians.
** Convention: angle 0 = East (+x), angles increase counter-clockwise.
**   East  → 0
**   North → π/2   (map row decreases)
**   West  → π
**   South → 3π/2  (map row increases)
*/
static double	dir_to_angle(t_dir dir)
{
	if (dir == DIR_NORTH)
		return (M_PI / 2.0);
	if (dir == DIR_SOUTH)
		return (3.0 * M_PI / 2.0);
	if (dir == DIR_WEST)
		return (M_PI);
	return (0.0);   /* DIR_EAST */
}

/* ── Collision helper ───────────────────────────────────── */

/*
** is_wall – returns 1 if world position (wx, wy) is inside a wall or
** outside the map boundaries.  Used to prevent the player from walking
** through walls.
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
** update_player – applies one frame of input to the player state.
** Movement uses a small margin (MOVE_SPEED) so the player doesn't
** clip directly into walls.  Rotation simply adds ±ROT_SPEED to the angle.
*/
static void	update_player(t_game *g, t_input *in)
{
	double	dx;
	double	dy;
	double	nx;
	double	ny;

	/* Rotation */
	if (in->left)
		g->player.angle += ROT_SPEED;
	if (in->right)
		g->player.angle -= ROT_SPEED;

	/* Keep angle in [0, 2π) to avoid accumulated floating-point drift */
	if (g->player.angle < 0)
		g->player.angle += 2.0 * M_PI;
	if (g->player.angle >= 2.0 * M_PI)
		g->player.angle -= 2.0 * M_PI;

	/* Forward / backward movement */
	dx = cos(g->player.angle) * MOVE_SPEED;
	dy = -sin(g->player.angle) * MOVE_SPEED;   /* y-axis inversion */

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

/* ── Cleanup ────────────────────────────────────────────── */

/*
** cleanup – restores terminal, frees all heap memory.
** Called both on normal exit and on signal.
** We keep a global pointer so a signal handler can reach it.
*/
static t_game	*g_game_ptr = NULL;

static void	cleanup(t_game *g)
{
	term_restore(&g->term);
	screen_free(&g->screen);
	free_map(&g->map);
	write(STDOUT_FILENO, ANSI_CLEAR, sizeof(ANSI_CLEAR) - 1);
	write(STDOUT_FILENO, ANSI_HOME, sizeof(ANSI_HOME) - 1);
}

/* ── Public: run_game ───────────────────────────────────── */

void	run_game(t_game *g)
{
	t_input	in;

	g_game_ptr = g;

	/* Initialise live player from parsed map data */
	g->player = g->map.player;
	g->player.angle = dir_to_angle(g->map.player.dir);

	/* Screen */
	if (screen_init(&g->screen) < 0)
	{
		free_map(&g->map);
		write(STDERR_FILENO, "Error\nFailed to allocate screen buffer.\n", 40);
		exit(EXIT_FAILURE);
	}

	/* Raw terminal */
	term_set_raw(&g->term);

	/* Clear screen once before the loop starts */
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
