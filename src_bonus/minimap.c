#include "../includes_bonus/ft_caster_bonus.h"

/*
** ─────────────────────────────────────────────────────────────────────────────
**  MINIMAP
**
**  Renders a top-left overlay of the loaded map into the existing framebuffer.
**  No extra memory is allocated: we write directly into g->screen.buf and
**  g->screen.face_buf, overwriting whatever the raycaster drew there.
**
**  Layout
**  ──────
**  MINI_BORDER rows/columns of spaces surround the map on the screen.
**  Each map cell occupies MINI_CELL_W columns × MINI_CELL_H rows so that
**  cells appear roughly square (terminal character cells are ~2:1 wide).
**
**  The map is scaled down if it would exceed MINI_MAX_COLS or MINI_MAX_ROWS:
**  we compute a step size (how many map cells one screen cell represents)
**  and sample the map at those intervals.  For typical maps (≤ 30 cells
**  wide) every cell maps 1:1 and step = 1.
**
**  Cell colours  (background blocks for clean visual separation):
**    Wall        → dark grey  (FACE_MINI_WALL)
**    Floor       → near-black (FACE_MINI_FLOOR)
**    Player cell → orange     (FACE_MINI_PLAYER)
**
**  Player direction indicator
**  ──────────────────────────
**  A single arrow character (↑ ↓ ← →, or the closest diagonal) is drawn
**  on the player cell using the player's current angle, so the player can
**  see both their position and facing direction on the minimap.
**
**  Only open/read/write/malloc/free and math.h functions are used.
** ─────────────────────────────────────────────────────────────────────────────
*/

/* ── write_cell ─────────────────────────────────────────── */

/*
** write_cell – writes a single minimap cell (screen_col, screen_row) into
** the framebuffer.  Clamps silently if the position is outside the screen.
** Each map cell is MINI_CELL_W wide so we fill that many columns.
*/
static void	write_cell(t_screen *sc, int scol, int srow,
	t_face face, char ch)
{
	int	c;
	int	idx;

	if (srow < 0 || srow >= sc->h)
		return ;
	c = 0;
	while (c < MINI_CELL_W)
	{
		if (scol + c >= 0 && scol + c < sc->w)
		{
			idx = srow * sc->w + scol + c;
			sc->buf[idx] = ch;
			sc->face_buf[idx] = face;
		}
		c++;
	}
}

/* ── arrow_for_angle ────────────────────────────────────── */

/*
** Returns a simple ASCII arrow character representing the player's
** facing direction, divided into 8 octants.
**
** Angle 0 = East, increasing counter-clockwise (our convention).
** We map:
**   [-π/8 .. π/8)    → '>'   East
**   [π/8  .. 3π/8)   → '^'   NE  (use '^' – cleaner than diagonal)
**   [3π/8 .. 5π/8)   → '^'   North
**   [5π/8 .. 7π/8)   → '^'   NW
**   [7π/8 .. 9π/8)   → '<'   West
**   [9π/8 .. 11π/8)  → 'v'   SW
**   [11π/8..13π/8)   → 'v'   South
**   [13π/8..15π/8)   → 'v'   SE
**
** Since terminal arrows ↑↓←→ are multi-byte UTF-8, we use ASCII ^ v < >
** to stay in the single-byte char domain.
*/
static char	arrow_for_angle(double angle)
{
	double	a;
	double	pi;

	pi = M_PI;
	/* normalise to [0, 2π) */
	a = angle;
	while (a < 0.0)
		a += 2.0 * pi;
	while (a >= 2.0 * pi)
		a -= 2.0 * pi;
	if (a < pi / 8.0 || a >= 15.0 * pi / 8.0)
		return ('>');
	if (a < 3.0 * pi / 8.0)
		return ('^');
	if (a < 5.0 * pi / 8.0)
		return ('^');
	if (a < 7.0 * pi / 8.0)
		return ('^');
	if (a < 9.0 * pi / 8.0)
		return ('<');
	if (a < 11.0 * pi / 8.0)
		return ('v');
	if (a < 13.0 * pi / 8.0)
		return ('v');
	return ('v');
}

/* ── cell_at_map ────────────────────────────────────────── */

/*
** Returns the map character at (mx, my), or '1' if out of bounds.
** Used to sample the map at scaled coordinates.
*/
static char	cell_at_map(t_map *map, int mx, int my)
{
	if (my < 0 || my >= map->height || mx < 0)
		return ('1');
	if (mx >= ft_strlen(map->grid[my]))
		return ('1');
	return (map->grid[my][mx]);
}

/* ── draw_minimap ───────────────────────────────────────── */

void	draw_minimap(t_game *g)
{
	t_map	*map;
	int		map_w;
	int		map_h;
	int		step;
	int		draw_cols;
	int		draw_rows;
	int		origin_col;
	int		origin_row;
	int		my;
	int		mx;
	int		scol;
	int		srow;
	int		px_cell;
	int		py_cell;
	t_face	face;
	char	ch;

	map = &g->map;
	map_w = map->width;
	map_h = map->height;

	/*
	** Compute step: how many map cells one screen-cell-group represents.
	** We want:  draw_cols = map_w * MINI_CELL_W / step  ≤  MINI_MAX_COLS
	** So:       step = ceil(map_w * MINI_CELL_W / MINI_MAX_COLS)
	** But since cell height = MINI_CELL_H = 1, the row step is the same.
	** We use a single step for both axes so the map is not distorted.
	*/
	step = 1;
	while ((map_w / step) * MINI_CELL_W > MINI_MAX_COLS
		|| map_h / step > MINI_MAX_ROWS)
		step++;

	/* Screen dimensions of the rendered minimap (cells, not map cells) */
	draw_cols = (map_w / step) * MINI_CELL_W;
	draw_rows = map_h / step;

	/* Top-left origin on screen, including border */
	origin_col = MINI_BORDER;
	origin_row = MINI_BORDER;

	/* Player's map-cell coordinates */
	px_cell = (int)g->player.x;
	py_cell = (int)g->player.y;

	/* ── Draw border rows (blank, over-write raycaster output) ── */
	/*
	** We clear a border of 1 screen-row above/below and MINI_CELL_W
	** columns left/right so the minimap floats cleanly.
	** Since screen_clear_buf already set everything to ' '/FACE_NONE,
	** the border is implicitly blank — nothing extra needed.
	*/

	/* ── Draw map cells ──────────────────────────────────────── */
	my = 0;
	while (my * step < map_h)
	{
		srow = origin_row + my;
		if (srow >= g->screen.h)
			break ;
		mx = 0;
		while (mx * step < map_w)
		{
			scol = origin_col + mx * MINI_CELL_W;
			if (scol >= g->screen.w)
				break ;
			/* Player cell */
			if (mx * step == px_cell && my * step == py_cell)
			{
				face = FACE_MINI_PLAYER;
				ch = arrow_for_angle(g->player.angle);
			}
			else if (cell_at_map(map, mx * step, my * step) == CHAR_WALL)
			{
				face = FACE_MINI_WALL;
				ch = ' ';
			}
			else
			{
				face = FACE_MINI_FLOOR;
				ch = ' ';
			}
			write_cell(&g->screen, scol, srow, face, ch);
			mx++;
		}
		my++;
	}

	/* ── Draw a 1-cell blank border row above the map ─────────── */
	/* Already spaces from screen_clear_buf; mark explicitly clean */
	if (origin_row > 0)
	{
		scol = 0;
		while (scol < origin_col + draw_cols + MINI_BORDER
			&& scol < g->screen.w)
		{
			g->screen.face_buf[0 * g->screen.w + scol] = FACE_NONE;
			g->screen.buf[0 * g->screen.w + scol] = ' ';
			scol++;
		}
	}

	/* ── Overdraw the right-side border column ─────────────── */
	srow = 0;
	while (srow < origin_row + draw_rows + MINI_BORDER
		&& srow < g->screen.h)
	{
		scol = origin_col + draw_cols;
		if (scol < g->screen.w)
		{
			g->screen.face_buf[srow * g->screen.w + scol] = FACE_NONE;
			g->screen.buf[srow * g->screen.w + scol] = ' ';
		}
		srow++;
	}
	(void)draw_rows;
	(void)draw_cols;
}
