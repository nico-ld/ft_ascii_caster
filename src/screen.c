#include "../includes/ft_caster.h"


/*
** parse_cpr – parses the terminal's CPR response: ESC [ rows ; cols R
** Reads decimal digits for rows, then cols, from buf[i] onward.
** Returns the number of bytes consumed.
*/
static int	parse_cpr(const char *buf, int n, int *rows, int *cols)
{
	int	i;

	*rows = 0;
	*cols = 0;
	i = 0;
	/* expect ESC [ */
	if (i + 1 >= n || buf[i] != '\033' || buf[i + 1] != '[')
		return (0);
	i += 2;
	/* parse rows */
	while (i < n && buf[i] >= '0' && buf[i] <= '9')
		*rows = *rows * 10 + (buf[i++] - '0');
	/* expect ';' */
	if (i >= n || buf[i] != ';')
		return (0);
	i++;
	/* parse cols */
	while (i < n && buf[i] >= '0' && buf[i] <= '9')
		*cols = *cols * 10 + (buf[i++] - '0');
	/* expect 'R' */
	if (i >= n || buf[i] != 'R')
		return (0);
	return (i + 1);
}

/*
** get_terminal_size – queries the real terminal dimensions using the
** ANSI Cursor Position Report (CPR) trick:
**
**   1. Open /dev/tty for read+write (works even when stdin/stdout
**      are redirected, because /dev/tty is always the controlling terminal).
**   2. Put /dev/tty into raw mode (VMIN=1, VTIME=1) so we can read
**      the response byte-by-byte without waiting for Enter.
**   3. Write  ESC[999;999H  – move cursor to an absurdly large position;
**      the terminal clamps it to the actual bottom-right corner.
**   4. Write  ESC[6n        – Device Status Report: "where is the cursor?"
**   5. Read back  ESC[rows;colsR  and parse it.
**   6. Restore /dev/tty's original settings and close it.
**
** Falls back to DEFAULT_SCREEN_W x DEFAULT_SCREEN_H on any error.
** Uses only open/read/write/close and termios – no ioctl, no signal.h.
*/
static void	get_terminal_size(int *w, int *h)
{
	int				tty;
	struct termios	orig;
	struct termios	raw;
	char			buf[32];
	int				n;
	int				rows;
	int				cols;

	*w = DEFAULT_SCREEN_W;
	*h = DEFAULT_SCREEN_H;
	tty = open("/dev/tty", O_RDWR);
	if (tty < 0)
		return ;
	if (tcgetattr(tty, &orig) < 0)
	{
		close(tty);
		return ;
	}
	raw = orig;
	raw.c_lflag &= ~(unsigned int)(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(tty, TCSANOW, &raw) < 0)
	{
		close(tty);
		return ;
	}
	/* Move to bottom-right then ask "where am I?" */
	write(tty, "\033[999;999H\033[6n", 14);
	n = (int)read(tty, buf, sizeof(buf) - 1);
	tcsetattr(tty, TCSANOW, &orig);
	close(tty);
	if (n <= 0)
		return ;
	buf[n] = '\0';
	rows = 0;
	cols = 0;
	if (parse_cpr(buf, n, &rows, &cols) > 0 && rows > 0 && cols > 0)
	{
		*h = rows;
		*w = cols;
	}
}

/*
** screen_init – allocates the framebuffer.
**
** Layout: h lines of w characters each, terminated by '\r\n' (2 bytes).
** The extra '\r' is required because we disabled OPOST in raw mode, which
** means the terminal no longer translates '\n' → '\r\n' for us.
** Total size: h * (w + 2) + 1 (NUL sentinel).
*/
int	screen_init(t_screen *sc)
{
	get_terminal_size(&sc->w, &sc->h);
	sc->buf_size = sc->h * (sc->w + 2) + 1;
	sc->buf = malloc(sc->buf_size);
	if (!sc->buf)
		return (-1);
	ft_memset(sc->buf, ' ', sc->buf_size - 1);
	sc->buf[sc->buf_size - 1] = '\0';
	return (0);
}

void	screen_free(t_screen *sc)
{
	free(sc->buf);
	sc->buf = NULL;
}

/*
** screen_clear_buf – fills every cell with space, re-inserts \r\n line ends.
** Called at the start of each frame before render_frame writes wall slices.
*/
void	screen_clear_buf(t_screen *sc)
{
	int	y;
	int	row_stride;

	row_stride = sc->w + 2;
	y = 0;
	while (y < sc->h)
	{
		ft_memset(sc->buf + y * row_stride, ' ', sc->w);
		sc->buf[y * row_stride + sc->w]     = '\r';
		sc->buf[y * row_stride + sc->w + 1] = '\n';
		y++;
	}
	sc->buf[sc->h * row_stride] = '\0';
}

/*
** screen_flush – moves cursor to (0,0) and writes the entire framebuffer
** in one write() call to minimise flicker.
*/
void	screen_flush(t_screen *sc)
{
	write(STDOUT_FILENO, ANSI_HOME, sizeof(ANSI_HOME) - 1);
	write(STDOUT_FILENO, sc->buf, sc->h * (sc->w + 2));
}
