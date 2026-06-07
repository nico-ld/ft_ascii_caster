#include "../includes/ft_caster.h"

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
	run_game(&game);
	return (EXIT_SUCCESS);
}
