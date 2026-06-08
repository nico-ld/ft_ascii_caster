#include "../includes/ft_caster.h"

/* ────────────────────────────────────────────────────────────────────────────
** SECTION 1 – File reading
** We read the whole file into a single heap buffer, then split on newlines.
** ────────────────────────────────────────────────────────────────────────── */

/*
** read_file – reads the entire content of fd into a malloc'd buffer.
** Returns NULL on error (caller must handle).
** *out_len receives the number of bytes read.
*/
static char	*read_file(int fd, int *out_len)
{
	char	buf[READ_BUF_SIZE];
	char	*content;
	char	*tmp;
	int		total;
	int		n;

	content = malloc(1);
	if (!content)
		return (NULL);
	content[0] = '\0';
	total = 0;
	n = read(fd, buf, READ_BUF_SIZE);
	while (n > 0)
	{
		tmp = malloc(total + n + 1);
		if (!tmp)
		{
			free(content);
			return (NULL);
		}
		ft_memcpy(tmp, content, total);
		ft_memcpy(tmp + total, buf, n);
		total += n;
		tmp[total] = '\0';
		free(content);
		content = tmp;
		n = read(fd, buf, READ_BUF_SIZE);
	}
	if (n < 0)
	{
		free(content);
		return (NULL);
	}
	*out_len = total;
	return (content);
}

/*
** count_lines – counts non-empty lines delimited by '\n'.
** Empty trailing newline at end of file is not counted.
*/
static int	count_lines(const char *content)
{
	int	count;
	int	i;

	count = 0;
	i = 0;
	while (content[i])
	{
		if (content[i] == '\n')
			count++;
		i++;
	}
	/* last line without trailing newline */
	if (i > 0 && content[i - 1] != '\n')
		count++;
	return (count);
}

/*
** split_lines – splits content on '\n', returns NULL-terminated array of
** heap-allocated strings (newline stripped). Returns NULL on alloc failure.
*/
static char	**split_lines(const char *content, int nb_lines)
{
	char	**lines;
	char	*line;
	int		i;
	int		j;
	int		start;

	lines = malloc(sizeof(char *) * (nb_lines + 1));
	if (!lines)
		return (NULL);
	i = 0;
	start = 0;
	j = 0;
	while (content[i])
	{
		if (content[i] == '\n')
		{
			line = malloc(i - start + 1);
			if (!line)
			{
				lines[j] = NULL;
				ft_free_strarr(lines);
				return (NULL);
			}
			ft_memcpy(line, content + start, i - start);
			/* strip \r if present */
			if (i - start > 0 && line[i - start - 1] == '\r')
				line[i - start - 1] = '\0';
			else
				line[i - start] = '\0';
			lines[j++] = line;
			start = i + 1;
		}
		i++;
	}
	/* last line (no trailing newline) */
	if (i > start)
	{
		line = malloc(i - start + 1);
		if (!line)
		{
			lines[j] = NULL;
			ft_free_strarr(lines);
			return (NULL);
		}
		ft_memcpy(line, content + start, i - start);
		line[i - start] = '\0';
		lines[j++] = line;
	}
	lines[j] = NULL;
	return (lines);
}

/* ────────────────────────────────────────────────────────────────────────────
** SECTION 2 – Validation helpers
** ────────────────────────────────────────────────────────────────────────── */

static int	is_player_char(char c)
{
	return (c == 'N' || c == 'S' || c == 'E' || c == 'W');
}

static t_dir	player_dir(char c)
{
	if (c == 'N')
		return (DIR_NORTH);
	if (c == 'S')
		return (DIR_SOUTH);
	if (c == 'E')
		return (DIR_EAST);
	return (DIR_WEST);
}

/*
** validate_chars – checks every character is '0', '1', or a player letter.
** Also detects multiple players.
** Returns 0 on success, -1 on error (sets *err_msg).
*/
static int	validate_chars(t_map *map, const char **err_msg)
{
	int	y;
	int	x;
	char	c;

	y = 0;
	while (y < map->height)
	{
		x = 0;
		while (map->grid[y][x])
		{
			c = map->grid[y][x];
			if (c != CHAR_WALL && c != CHAR_FLOOR && !is_player_char(c))
			{
				*err_msg = "Invalid character in map.";
				return (-1);
			}
			if (is_player_char(c))
			{
				if (map->player_found)
				{
					*err_msg = "Multiple player starting positions found.";
					return (-1);
				}
				map->player_found = 1;
				map->player.x = (double)x + 0.5;
				map->player.y = (double)y + 0.5;
				map->player.dir = player_dir(c);
			}
			x++;
		}
		y++;
	}
	if (!map->player_found)
	{
		*err_msg = "No player starting position found (use N, S, E or W).";
		return (-1);
	}
	return (0);
}

/*
** validate_no_empty_lines – an empty line inside the map is forbidden.
*/
static int	validate_no_empty_lines(t_map *map, const char **err_msg)
{
	int	y;

	y = 0;
	while (y < map->height)
	{
		if (map->grid[y][0] == '\0')
		{
			*err_msg = "Empty line found inside the map.";
			return (-1);
		}
		y++;
	}
	return (0);
}

/*
** cell_at – returns the character at (x, y).
** Returns VOID_CELL (' ') for anything outside the grid or beyond the end
** of a shorter row.  Space is never a valid map character, so callers can
** treat it unambiguously as "the void".
*/
# define VOID_CELL ' '

static char	cell_at(t_map *map, int x, int y)
{
	if (y < 0 || y >= map->height)
		return (VOID_CELL);
	if (x < 0 || x >= ft_strlen(map->grid[y]))
		return (VOID_CELL);
	return (map->grid[y][x]);
}

/*
** is_void – returns 1 if c represents empty space outside the map boundary.
*/
static int	is_void(char c)
{
	return (c == VOID_CELL);
}

/*
** validate_closed – every non-wall cell must have all 4 orthogonal neighbours
** be a valid map character ('0', '1', or player).  A VOID_CELL neighbour means
** the cell is exposed to the outside of the map, which is forbidden.
**
** This correctly handles jagged rows: if row y has length 8 but a floor cell
** on row y-1 is at column 9, cell_at returns VOID_CELL for that neighbour.
*/
static int	validate_closed(t_map *map, const char **err_msg)
{
	int		y;
	int		x;
	char	c;
	int		row_len;

	y = 0;
	while (y < map->height)
	{
		row_len = ft_strlen(map->grid[y]);
		x = 0;
		while (x < row_len)
		{
			c = map->grid[y][x];
			if (c != CHAR_WALL)
			{
				if (is_void(cell_at(map, x, y - 1))
					|| is_void(cell_at(map, x, y + 1))
					|| is_void(cell_at(map, x - 1, y))
					|| is_void(cell_at(map, x + 1, y)))
				{
					*err_msg = "Map is not closed: floor cell exposed to void.";
					return (-1);
				}
			}
			x++;
		}
		y++;
	}
	return (0);
}

/* ────────────────────────────────────────────────────────────────────────────
** SECTION 3 – Public entry points
** ────────────────────────────────────────────────────────────────────────── */

/*
** check_extension – .map files only.
*/
static void	check_extension(const char *path, t_map *map)
{
	int	len;

	len = ft_strlen(path);
	if (len < 5 || ft_strcmp(path + len - 4, ".map") != 0)
		error_exit(map, "File must have a .map extension.");
}

/*
** parse_map – public function called from main.
** Fills *map or calls error_exit (which frees and exits).
*/
int	parse_map(const char *path, t_map *map)
{
	int		fd;
	char	*content;
	char	**lines;
	int		nb_lines;
	int		file_len;
	int		y;
	const char	*err_msg;

	ft_memset(map, 0, sizeof(t_map));
	check_extension(path, map);

	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror("open");
		error_exit(map, "Cannot open map file.");
	}

	file_len = 0;
	content = read_file(fd, &file_len);
	close(fd);
	if (!content)
		error_exit(map, "Failed to read map file (memory or I/O error).");

	if (file_len == 0)
	{
		free(content);
		error_exit(map, "Map file is empty.");
	}

	nb_lines = count_lines(content);
	lines = split_lines(content, nb_lines);
	free(content);
	if (!lines)
		error_exit(map, "Memory allocation failure while parsing map.");

	/* Store into map struct so error_exit can free on any later failure */
	map->grid = lines;
	map->height = nb_lines;

	/* Compute widest row */
	y = 0;
	while (y < map->height)
	{
		int w = ft_strlen(map->grid[y]);
		if (w > map->width)
			map->width = w;
		y++;
	}

	if (map->height < 3 || map->width < 3)
		error_exit(map, "Map is too small (minimum 3x3).");

	err_msg = NULL;
	if (validate_no_empty_lines(map, &err_msg) < 0)
		error_exit(map, err_msg);
	if (validate_chars(map, &err_msg) < 0)
		error_exit(map, err_msg);
	if (validate_closed(map, &err_msg) < 0)
		error_exit(map, err_msg);

	return (0);
}