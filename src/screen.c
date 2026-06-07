#include "../includes/ft_caster.h"

/*
** get_terminal_size – queries the terminal dimensions via TIOCGWINSZ.
** Falls back to DEFAULT_SCREEN_W x DEFAULT_SCREEN_H if the ioctl fails
** or returns zero (e.g. when stdout is redirected).
*/
static void	get_terminal_size(int *w, int *h)
{
	struct winsize	ws;

	*w = DEFAULT_SCREEN_W;
	*h = DEFAULT_SCREEN_H;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0
		&& ws.ws_col > 0 && ws.ws_row > 0)
	{
		*w = (int)ws.ws_col;
		*h = (int)ws.ws_row;
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
	memset(sc->buf, ' ', sc->buf_size - 1);
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
		memset(sc->buf + y * row_stride, ' ', sc->w);
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
