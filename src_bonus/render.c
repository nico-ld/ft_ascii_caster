#include "../includes_bonus/ft_caster_bonus.h"

/*
** ─────────────────────────────────────────────────────────────────────────────
**  RAYCASTING ENGINE – BONUS
**
**  Extends the mandatory DDA with wall-face detection:
**
**  Which face of a wall was hit?
**  ┌──────────────────────────────────────────────────────┐
**  │  DDA side │ step direction │  face seen by the ray  │
**  │───────────┼────────────────┼────────────────────────│
**  │  X-side   │  step_x = +1  │  WEST  (ray came from W) │
**  │  X-side   │  step_x = -1  │  EAST  (ray came from E) │
**  │  Y-side   │  step_y = +1  │  NORTH (ray came from N) │
**  │  Y-side   │  step_y = -1  │  SOUTH (ray came from S) │
**  └──────────────────────────────────────────────────────┘
**  Remember: in our coordinate system row 0 = top of screen,
**  so ray_dy = -sin(angle) and step_y > 0 means moving downward
**  on screen = moving toward the south wall of the cell below,
**  which is the NORTH face of the wall hit.
**
**  The face is stored in t_ray.face and written into screen.face_buf
**  so screen_flush can emit the right ANSI colour per cell.
** ─────────────────────────────────────────────────────────────────────────────
*/

/* ── t_hit: raw DDA output ──────────────────────────────── */

typedef struct s_hit
{
	double	dist;
	t_face	face;
}	t_hit;

/* ── Angle helper ───────────────────────────────────────── */

static double	angle_for_col(int col, int screen_w, double player_angle)
{
	double	frac;

	frac = (double)col / (double)(screen_w - 1);
	return (player_angle - HALF_FOV + frac * FOV);
}

/* ── DDA ────────────────────────────────────────────────── */

static t_hit	cast_ray(t_map *map, double px, double py,
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
	int		side;
	double	perp_dist;
	t_hit	result;

	map_x = (int)px;
	map_y = (int)py;
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
		if (map_y < 0 || map_y >= map->height || map_x < 0)
			break ;
		if (map_x >= ft_strlen(map->grid[map_y]))
			break ;
		if (map->grid[map_y][map_x] == CHAR_WALL)
			hit = 1;
	}
	/* Perpendicular distance */
	if (side == 0)
		perp_dist = side_dist_x - delta_dist_x;
	else
		perp_dist = side_dist_y - delta_dist_y;
	if (perp_dist < 0.0001)
		perp_dist = 0.0001;
	/* Determine face from side + step direction */
	if (side == 0)
		result.face = (step_x > 0) ? FACE_WEST : FACE_EAST;
	else
		result.face = (step_y > 0) ? FACE_NORTH : FACE_SOUTH;
	result.dist = perp_dist;
	return (result);
}

/* ── Shade selection ────────────────────────────────────── */

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

static t_ray	project_column(t_hit hit, int screen_h)
{
	t_ray	ray;
	int		wall_h;

	ray.dist = hit.dist;
	ray.face = hit.face;
	wall_h = (int)((double)screen_h / hit.dist);
	if (wall_h > screen_h)
		wall_h = screen_h;
	ray.wall_h   = wall_h;
	ray.wall_top = (screen_h - wall_h) / 2;
	ray.wall_bot = ray.wall_top + wall_h - 1;
	ray.shade    = shade_for_dist(hit.dist);
	return (ray);
}

/* ── Framebuffer write ──────────────────────────────────── */

/*
** draw_column – writes shade char AND face into the parallel buffers.
** buf[y * w + x]      = shade character
** face_buf[y * w + x] = wall face (or FACE_NONE for ceiling/floor)
*/
static void	draw_column(t_screen *sc, int col, t_ray *ray)
{
	int	y;

	y = ray->wall_top;
	while (y <= ray->wall_bot && y < sc->h)
	{
		if (y >= 0)
		{
			sc->buf[y * sc->w + col] = ray->shade;
			sc->face_buf[y * sc->w + col] = ray->face;
		}
		y++;
	}
}

/* ── Public entry point ─────────────────────────────────── */

void	render_frame(t_game *g)
{
	int		col;
	double	ray_angle;
	double	ray_dx;
	double	ray_dy;
	t_hit	hit;
	t_ray	ray;

	col = 0;
	while (col < g->screen.w)
	{
		ray_angle = angle_for_col(col, g->screen.w, g->player.angle);
		ray_dx = cos(ray_angle);
		ray_dy = -sin(ray_angle);
		hit    = cast_ray(&g->map, g->player.x, g->player.y, ray_dx, ray_dy);
		ray    = project_column(hit, g->screen.h);
		draw_column(&g->screen, col, &ray);
		col++;
	}
}
