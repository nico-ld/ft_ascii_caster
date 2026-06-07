#include "../includes/ft_caster.h"

/*
** ─────────────────────────────────────────────────────────────────────────────
**  RAYCASTING ENGINE  –  DDA algorithm, one ray per screen column
**
**  Coordinate system
**  -----------------
**  grid[row][col]  →  world (x, y):  x = col,  y = row
**  Player angle 0 = facing +x (East).  Angles increase counter-clockwise.
**  North on map = decreasing y  (angle = π/2).
**
**  DDA overview
**  ------------
**  For each screen column col ∈ [0, screen_width):
**    1. Compute the ray direction from the player angle and FOV.
**    2. Determine which map cell the player is in.
**    3. Compute step sizes along x and y (delta_dist).
**    4. Step through the grid cell by cell until a wall is hit.
**    5. Compute perpendicular distance to eliminate fisheye.
**    6. Project the wall slice height onto the screen.
**    7. Fill the framebuffer column: ceiling | wall | floor.
** ─────────────────────────────────────────────────────────────────────────────
*/

/* ── Angle helpers ──────────────────────────────────────── */

/*
** angle_for_col – ray angle for screen column col.
** Rays sweep from (player_angle - HALF_FOV) on the left to
** (player_angle + HALF_FOV) on the right.
*/
static double	angle_for_col(int col, int screen_w, double player_angle)
{
	double	frac;

	frac = (double)col / (double)(screen_w - 1);
	return (player_angle - HALF_FOV + frac * FOV);
}

/* ── DDA ────────────────────────────────────────────────── */

/*
** cast_ray – runs DDA for one ray direction (ray_dx, ray_dy).
** Returns the perpendicular wall distance in world units.
** Populates no extra state: callers only need the distance.
**
** DDA logic:
**   delta_dist_x = |1 / ray_dx|   distance the ray travels in x to cross 1 tile
**   delta_dist_y = |1 / ray_dy|   same for y
**   side_dist_x  = distance from player to first x grid line in ray direction
**   side_dist_y  = same for y
**   At each step we advance whichever side_dist is smaller, until we hit a wall.
**   Perpendicular distance avoids fisheye:
**     if x-side was hit: perp_dist = (map_x - px + (1 - step_x)/2) / ray_dx
**     if y-side was hit: perp_dist = (map_y - py + (1 - step_y)/2) / ray_dy
*/
static double	cast_ray(t_map *map, double px, double py,
	double ray_dx, double ray_dy)
{
	int		map_x;
	int		map_y;
	int		step_x;
	int		step_y;
	double	delta_dist_x;
	double	delta_dist_y;
	double	side_dist_x;
	double	side_dist_y;
	int		hit;
	int		side;       /* 0 = x-side wall, 1 = y-side wall */
	double	perp_dist;

	map_x = (int)px;
	map_y = (int)py;

	/* Avoid division by zero with a tiny epsilon */
	delta_dist_x = (ray_dx == 0.0) ? 1e30 : fabs(1.0 / ray_dx);
	delta_dist_y = (ray_dy == 0.0) ? 1e30 : fabs(1.0 / ray_dy);

	if (ray_dx < 0)
	{
		step_x = -1;
		side_dist_x = (px - map_x) * delta_dist_x;
	}
	else
	{
		step_x = 1;
		side_dist_x = (map_x + 1.0 - px) * delta_dist_x;
	}
	if (ray_dy < 0)
	{
		step_y = -1;
		side_dist_y = (py - map_y) * delta_dist_y;
	}
	else
	{
		step_y = 1;
		side_dist_y = (map_y + 1.0 - py) * delta_dist_y;
	}

	/* DDA loop */
	hit = 0;
	side = 0;
	while (!hit)
	{
		if (side_dist_x < side_dist_y)
		{
			side_dist_x += delta_dist_x;
			map_x += step_x;
			side = 0;
		}
		else
		{
			side_dist_y += delta_dist_y;
			map_y += step_y;
			side = 1;
		}
		/* Bounds check – treat out-of-bounds as wall to avoid infinite loop */
		if (map_y < 0 || map_y >= map->height || map_x < 0)
			break ;
		if (map_x >= ft_strlen(map->grid[map_y]))
			break ;
		if (map->grid[map_y][map_x] == CHAR_WALL)
			hit = 1;
	}

	/* Perpendicular distance (fisheye correction) */
	if (side == 0)
		perp_dist = side_dist_x - delta_dist_x;
	else
		perp_dist = side_dist_y - delta_dist_y;

	if (perp_dist < 0.0001)
		perp_dist = 0.0001;
	return (perp_dist);
}

/* ── Shade selection ────────────────────────────────────── */

/*
** shade_for_dist – returns the ASCII character representing the wall density
** at the given perpendicular distance.
** Thresholds: SHADE_D0 … SHADE_D3 are defined in the header.
** Palette:    '@' '#' 'O' 'x' '.'   (index 0 = nearest)
*/
static char	shade_for_dist(double dist)
{
	const char	palette[] = SHADE_CHARS;

	if (dist < SHADE_D0)
		return (palette[0]);
	if (dist < SHADE_D1)
		return (palette[1]);
	if (dist < SHADE_D2)
		return (palette[2]);
	if (dist < SHADE_D3)
		return (palette[3]);
	return (palette[4]);
}

/* ── Column projection ──────────────────────────────────── */

/*
** project_column – given perpendicular distance, fills a t_ray with
** the screen-row span for the wall slice and its shade character.
**
** Wall height in pixels:  wall_h = screen_h / perp_dist
** (a wall exactly 1 unit away fills the whole screen height)
*/
static t_ray	project_column(double dist, int screen_h)
{
	t_ray	ray;
	int		wall_h;

	ray.dist = dist;
	wall_h = (int)((double)screen_h / dist);
	if (wall_h > screen_h)
		wall_h = screen_h;
	ray.wall_h   = wall_h;
	ray.wall_top = (screen_h - wall_h) / 2;
	ray.wall_bot = ray.wall_top + wall_h - 1;
	ray.shade    = shade_for_dist(dist);
	return (ray);
}

/* ── Framebuffer write ──────────────────────────────────── */

/*
** draw_column – writes a vertical slice into sc->buf at column x.
**
** Framebuffer layout (set by screen_clear_buf):
**   Row y starts at buf[y * (sc->w + 2)].
**   Column x within that row is offset +x.
**   Bytes sc->w and sc->w+1 are '\r' and '\n' (not touched here).
**
** We write:
**   rows [0,        wall_top-1]   → ' ' (ceiling, already spaces)
**   rows [wall_top, wall_bot  ]   → shade character
**   rows [wall_bot+1, sc->h-1]   → ' ' (floor, already spaces)
** Since screen_clear_buf pre-fills with spaces, we only write wall rows.
*/
static void	draw_column(t_screen *sc, int col, t_ray *ray)
{
	int	row_stride;
	int	y;

	row_stride = sc->w + 2;
	y = ray->wall_top;
	while (y <= ray->wall_bot && y < sc->h)
	{
		if (y >= 0)
			sc->buf[y * row_stride + col] = ray->shade;
		y++;
	}
}

/* ── Public entry point ─────────────────────────────────── */

/*
** render_frame – for each screen column, cast a ray and draw the wall slice.
** Called once per game loop iteration after screen_clear_buf.
*/
void	render_frame(t_game *g)
{
	int		col;
	double	ray_angle;
	double	ray_dx;
	double	ray_dy;
	double	dist;
	t_ray	ray;

	col = 0;
	while (col < g->screen.w)
	{
		ray_angle = angle_for_col(col, g->screen.w, g->player.angle);
		ray_dx = cos(ray_angle);
		ray_dy = -sin(ray_angle);   /* y axis is inverted: row 0 = top of screen */
		dist   = cast_ray(&g->map, g->player.x, g->player.y, ray_dx, ray_dy);
		ray    = project_column(dist, g->screen.h);
		draw_column(&g->screen, col, &ray);
		col++;
	}
}
