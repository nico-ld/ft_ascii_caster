#include "../includes/ft_caster.h"

static void	print_map_debug(t_map *map)
{
	int	y;

	printf("Map loaded: %d rows x %d cols\n", map->height, map->width);
	y = 0;
	while (y < map->height)
	{
		printf("  %s\n", map->grid[y]);
		y++;
	}
	printf("Player at (%.1f, %.1f) facing %s\n",
		map->player.x, map->player.y,
		map->player.dir == DIR_NORTH ? "NORTH" :
		map->player.dir == DIR_SOUTH ? "SOUTH" :
		map->player.dir == DIR_EAST  ? "EAST"  : "WEST");
}

int	main(int argc, char **argv)
{
	t_game	game;

	if (argc != 2)
	{
		write(STDERR_FILENO, "Error\n", 6);
		write(STDERR_FILENO, "Usage: ./ft_ascii_caster maps/classic.map\n", 42);
		return (EXIT_FAILURE);
	}
	memset(&game, 0, sizeof(t_game));
	parse_map(argv[1], &game.map);

	/* Debug output – will be replaced by the rendering loop in later steps */
	print_map_debug(&game.map);

	free_map(&game.map);
	return (EXIT_SUCCESS);
}
