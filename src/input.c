#include "../includes/ft_caster.h"

/*
** read_input – non-blocking drain of stdin in raw mode (VMIN=0, VTIME=0).
**
** Key bindings (per spec):
**   W            → move forward
**   S            → move backward
**   A            → strafe left
**   D            → strafe right
**   Left  arrow  → rotate left   (ESC [ C)
**   Right arrow  → rotate right  (ESC [ D)
**   ESC alone    → quit
**
** Arrow keys arrive as a 3-byte escape sequence: 0x1B 0x5B 0x41-0x44.
** We detect ESC-alone vs ESC-sequence by checking whether the next two
** bytes are already in the buffer (they always are if an arrow was pressed,
** because the kernel delivers the whole sequence in one read()).
**
** VMIN=0 / VTIME=0 means read() returns 0 immediately when no key is
** pressed – the caller just sees an empty t_input and moves on.
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
		if (buf[i] == '\033')
		{
			/* Check for ESC [ X  arrow-key sequence */
			if (i + 2 < n && buf[i + 1] == '[')
			{
				if (buf[i + 2] == 'C')
					in->right = 1;
				else if (buf[i + 2] == 'D')
					in->left = 1;
				/* Up (A) and Down (B) arrows are intentionally ignored */
				i += 3;
				continue ;
			}
			/* ESC alone (no following bytes in this read) → quit */
			in->quit = 1;
			i++;
			continue ;
		}
		if (buf[i] == 'w' || buf[i] == 'W')
			in->forward = 1;
		else if (buf[i] == 's' || buf[i] == 'S')
			in->backward = 1;
		else if (buf[i] == 'a' || buf[i] == 'A')
			in->strafe_left = 1;
		else if (buf[i] == 'd' || buf[i] == 'D')
			in->strafe_right = 1;
		i++;
	}
}
