#include "../includes/ft_caster.h"

/*
** read_input – non-blocking read of stdin in raw mode.
** Drains all available bytes (up to 16) and sets flags in *in.
** Key bindings:
**   W / Arrow-up    → forward
**   S / Arrow-down  → backward
**   A / Arrow-left  → rotate left
**   D / Arrow-right → rotate right
**   Q / ESC         → quit
*/
void	read_input(t_input *in)
{
	char	buf[16];
	int		n;
	int		i;

	memset(in, 0, sizeof(t_input));
	n = (int)read(STDIN_FILENO, buf, sizeof(buf));
	if (n <= 0)
		return ;
	i = 0;
	while (i < n)
	{
		if (buf[i] == 'q' || buf[i] == 'Q' || buf[i] == 27)
		{
			/* ESC alone → quit; ESC [ A/B/C/D → arrow keys */
			if (buf[i] == 27 && i + 2 < n && buf[i + 1] == '[')
			{
				if (buf[i + 2] == 'A')
					in->forward = 1;
				else if (buf[i + 2] == 'B')
					in->backward = 1;
				else if (buf[i + 2] == 'C')
					in->right = 1;
				else if (buf[i + 2] == 'D')
					in->left = 1;
				else
					in->quit = 1;
				i += 3;
				continue ;
			}
			else if (buf[i] == 27)
				in->quit = 1;
			else
				in->quit = 1;
		}
		else if (buf[i] == 'w' || buf[i] == 'W')
			in->forward = 1;
		else if (buf[i] == 's' || buf[i] == 'S')
			in->backward = 1;
		else if (buf[i] == 'a' || buf[i] == 'A')
			in->left = 1;
		else if (buf[i] == 'd' || buf[i] == 'D')
			in->right = 1;
		i++;
	}
}
