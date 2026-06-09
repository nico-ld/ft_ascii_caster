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

/* ── Colour helpers ─────────────────────────────────────── */

static const char	*fg_for_face(t_face face)
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

static const char	*bg_for_face(t_face face)
{
	if (face == FACE_NORTH)
		return (BGCOL_NORTH);
	if (face == FACE_SOUTH)
		return (BGCOL_SOUTH);
	if (face == FACE_EAST)
		return (BGCOL_EAST);
	if (face == FACE_WEST)
		return (BGCOL_WEST);
	return (NULL);
}

static const char	*minimap_colour(t_face face)
{
	if (face == FACE_MINI_WALL)
		return (COLOR_MINI_WALL);
	if (face == FACE_MINI_FLOOR)
		return (COLOR_MINI_FLOOR);
	if (face == FACE_MINI_PLAYER)
		return (COLOR_MINI_PLAYER);
	return (NULL);
}

/*
** cell_escape – writes the ANSI prefix for one cell into dst (≤ 64 bytes).
** Returns the number of bytes written (no NUL terminator added).
**
** render_mode for wall faces:
**   0  – foreground only  → fg colour escape
**   1  – background only  → bg colour escape
**   2  – fg + bg          → bg escape concatenated with fg escape
**
** Minimap faces always use their background colour (mode-independent).
** FACE_NONE returns 0: no bytes written, no escape emitted.
*/
static int	cell_escape(t_face face, int mode, char *dst)
{
	const char	*fg;
	const char	*bg;
	const char	*mm;
	int			len;
	int			i;

	mm = minimap_colour(face);
	if (mm)
	{
		len = 0;
		while (mm[len])
		{
			dst[len] = mm[len];
			len++;
		}
		return (len);
	}
	fg = fg_for_face(face);
	bg = bg_for_face(face);
	if (!fg)
		return (0);
	len = 0;
	if (mode == 0)
	{
		while (fg[len])
		{
			dst[len] = fg[len];
			len++;
		}
		return (len);
	}
	if (mode == 1)
	{
		while (bg[len])
		{
			dst[len] = bg[len];
			len++;
		}
		return (len);
	}
	/* mode 2: bg first so fg overrides the foreground colour */
	i = 0;
	while (bg[i])
		dst[len++] = bg[i++];
	i = 0;
	while (fg[i])
		dst[len++] = fg[i++];
	return (len);
}

/*
** display_char – returns the character to draw for a cell.
** In bg-only mode (1) wall cells become spaces so only the colour block
** is visible. Minimap cells and all other modes keep their stored char.
*/
static char	display_char(t_screen *sc, int cell)
{
	t_face	face;

	face = sc->face_buf[cell];
	if (face == FACE_MINI_WALL || face == FACE_MINI_FLOOR
		|| face == FACE_MINI_PLAYER)
		return (sc->buf[cell]);
	if (sc->render_mode == 1)
		return (' ');
	return (sc->buf[cell]);
}

/* ── screen_flush ───────────────────────────────────────── */

/*
** Renders the framebuffer to stdout one row at a time.
**
** Per wall cell: <ANSI_prefix> <char> \033[0m
** Per ceiling/floor cell: plain space (no escape codes).
** Each row ends with \r\n (raw mode: OPOST is off).
**
** Line buffer sizing (worst case – mode 2 with widest escape strings):
**   BGCOL (12 bytes) + COLOR (12 bytes) + char (1) + RESET (4) = 29 bytes
**   500 columns × 29 + 2 (\r\n) = 14502 → rounded up to 16384.
*/
# define LINE_BUF_SIZE 16384

void	screen_flush(t_screen *sc)
{
	char		line[LINE_BUF_SIZE];
	char		esc[64];
	const char	*reset;
	int			rlen;
	int			elen;
	int			x;
	int			y;
	int			pos;
	int			cell;

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
			elen = cell_escape(sc->face_buf[cell], sc->render_mode, esc);
			if (elen > 0)
			{
				if (pos + elen + 1 + rlen + 2 < LINE_BUF_SIZE)
				{
					ft_memcpy(line + pos, esc, elen);
					pos += elen;
					line[pos++] = display_char(sc, cell);
					ft_memcpy(line + pos, reset, rlen);
					pos += rlen;
				}
			}
			else
			{
				if (pos + 1 < LINE_BUF_SIZE)
					line[pos++] = sc->buf[cell];
			}
			x++;
		}
		if (pos + 2 <= LINE_BUF_SIZE)
		{
			line[pos++] = '\r';
			line[pos++] = '\n';
		}
		write(STDOUT_FILENO, line, pos);
		y++;
	}
}
