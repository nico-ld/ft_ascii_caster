#include "../includes_bonus/ft_caster_bonus.h"

/*
** term_set_raw – switches the terminal to raw mode:
**   - no echo, no canonical buffering (input char-by-char)
**   - VMIN=0, VTIME=0 → non-blocking reads
** Saves original settings in t->original for later restore.
*/
void	term_set_raw(t_term *t)
{
	struct termios	raw;

	if (tcgetattr(STDIN_FILENO, &t->original) < 0)
	{
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}
	raw = t->original;
	/* Input flags: disable break signal, CR→NL, parity, strip 8th bit, XON/XOFF */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/* Output flags: disable post-processing (\n → \r\n) */
	raw.c_oflag &= ~(OPOST);
	/* Control: 8-bit chars */
	raw.c_cflag |= (CS8);
	/* Local: no echo, no canonical, no signals, no extended processing */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* Non-blocking: return immediately even if no bytes available */
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
	{
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
	t->raw_active = 1;
	/* Hide cursor to avoid flicker */
	write(STDOUT_FILENO, ANSI_CURSOR_HIDE, sizeof(ANSI_CURSOR_HIDE) - 1);
}

/*
** term_restore – restores original terminal settings and shows cursor.
** Safe to call even if raw mode was never activated.
*/
void	term_restore(t_term *t)
{
	if (!t->raw_active)
		return ;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &t->original);
	t->raw_active = 0;
	write(STDOUT_FILENO, ANSI_CURSOR_SHOW, sizeof(ANSI_CURSOR_SHOW) - 1);
	write(STDOUT_FILENO, "\r\n", 2);
}
