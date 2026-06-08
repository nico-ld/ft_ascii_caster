#include "../includes_bonus/ft_caster_bonus.h"

/* ── CPR terminal size query (unchanged from mandatory) ─── */

static int	parse_cpr(const char *buf, int n, int *rows, int *cols)
{
	int	i;

	*rows = 0;
	*cols = 0;
	i = 0;
	if (i + 1 >= n || buf[i] != '\033' || buf[i + 1] != '[')
		return (0);
	i += 2;
	while (i < n && buf[i] >= '0' && buf[i] <= '9')
		*rows = *rows * 10 + (buf[i++] - '0');
	if (i >= n || buf[i] != ';')
		return (0);
	i++;
	while (i < n && buf[i] >= '0' && buf[i] <= '9')
		*cols = *cols * 10 + (buf[i++] - '0');
	if (i >= n || buf[i] != 'R')
		return (0);
	return (i + 1);
}

static void	get_terminal_size(int *w, int *h)
{
	int				tty;
	struct termios	orig;
	struct termios	raw;
	char			qbuf[32];
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
	write(tty, "\033[999;999H\033[6n", 14);
	n = (int)read(tty, qbuf, sizeof(qbuf) - 1);
	tcsetattr(tty, TCSANOW, &orig);
	close(tty);
	if (n <= 0)
		return ;
	qbuf[n] = '\0';
	rows = 0;
	cols = 0;
	if (parse_cpr(qbuf, n, &rows, &cols) > 0 && rows > 0 && cols > 0)
	{
		*h = rows;
		*w = cols;
	}
}

/* ── screen_init ────────────────────────────────────────── */

int	screen_init(t_screen *sc)
{
	get_terminal_size(&sc->w, &sc->h);
	sc->buf_size = sc->h * sc->w;
	sc->buf = malloc(sc->buf_size);
	if (!sc->buf)
		return (-1);
	sc->face_buf = malloc(sc->buf_size * sizeof(t_face));
	if (!sc->face_buf)
	{
		free(sc->buf);
		sc->buf = NULL;
		return (-1);
	}
	return (0);
}

void	screen_free(t_screen *sc)
{
	free(sc->buf);
	free(sc->face_buf);
	sc->buf = NULL;
	sc->face_buf = NULL;
}

/* ── screen_clear_buf ───────────────────────────────────── */

void	screen_clear_buf(t_screen *sc)
{
	int	i;

	i = 0;
	while (i < sc->h * sc->w)
	{
		sc->buf[i] = ' ';
		sc->face_buf[i] = FACE_NONE;
		i++;
	}
}

/* ── colour_for_face ────────────────────────────────────── */

/*
** Returns the ANSI colour escape string for the given wall face.
** Returns NULL for FACE_NONE (ceiling/floor – no colour needed).
*/
static const char	*colour_for_face(t_face face)
{
	if (face == FACE_NORTH)
		return (COLOR_NORTH);
	if (face == FACE_SOUTH)
		return (COLOR_SOUTH);
	if (face == FACE_EAST)
		return (COLOR_EAST);
	if (face == FACE_WEST)
		return (COLOR_WEST);
	return (NULL);
}

/* ── screen_flush ───────────────────────────────────────── */

/*
** screen_flush – moves cursor to home and renders the framebuffer.
**
** For the bonus, every wall cell is wrapped in:
**     <colour_escape> shade_char \033[0m
** Ceiling/floor cells (' ', FACE_NONE) are emitted as plain spaces.
**
** We build output row-by-row:
**   - For each column, check face_buf to decide whether to emit colour.
**   - At the end of each row, write '\r\n' (raw mode: no OPOST).
**
** We avoid a giant pre-built buffer because ANSI codes make the byte
** count per row variable.  One write() per single character would be
** too slow; instead we accumulate each row in a stack-allocated line
** buffer (max width ~300 bytes with escape codes) and flush per row.
** At 30 fps on an 80-column terminal that's 80*24 = 1920 write calls –
** in practice we batch the whole row so it's just sc->h writes/frame.
**
** Line buffer sizing:
**   worst case per cell = len(COLOR_NORTH) + 1 + len(COLOR_RESET)
**                       = 11 + 1 + 4 = 16 bytes
**   max cols = 500 (generous upper bound)
**   per row  = 500 * 16 + 2 (\r\n) = 8002 bytes → we use 8192
*/
# define LINE_BUF_SIZE 8192

void	screen_flush(t_screen *sc)
{
	char		line[LINE_BUF_SIZE];
	int			x;
	int			y;
	int			pos;
	int			cell;
	t_face		face;
	const char	*col;
	const char	*reset;
	int			clen;
	int			rlen;

	reset = COLOR_RESET;
	rlen = 0;
	while (reset[rlen])
		rlen++;
	write(STDOUT_FILENO, ANSI_HOME, sizeof(ANSI_HOME) - 1);
	y = 0;
	while (y < sc->h)
	{
		pos = 0;
		x = 0;
		while (x < sc->w)
		{
			cell = y * sc->w + x;
			face = sc->face_buf[cell];
			col = colour_for_face(face);
			if (col)
			{
				/* wall cell: colour + shade char + reset */
				clen = 0;
				while (col[clen])
					clen++;
				if (pos + clen + 1 + rlen + 2 < LINE_BUF_SIZE)
				{
					ft_memcpy(line + pos, col, clen);
					pos += clen;
					line[pos++] = sc->buf[cell];
					ft_memcpy(line + pos, reset, rlen);
					pos += rlen;
				}
			}
			else
			{
				/* ceiling or floor: plain space */
				if (pos + 1 < LINE_BUF_SIZE)
					line[pos++] = sc->buf[cell];
			}
			x++;
		}
		/* end of row: carriage-return + newline (raw mode) */
		if (pos + 2 <= LINE_BUF_SIZE)
		{
			line[pos++] = '\r';
			line[pos++] = '\n';
		}
		write(STDOUT_FILENO, line, pos);
		y++;
	}
}
