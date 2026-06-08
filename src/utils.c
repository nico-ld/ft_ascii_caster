#include "../includes/ft_caster.h"

/* ── ft_strlen ──────────────────────────────────────────── */
int	ft_strlen(const char *s)
{
	int	i;

	i = 0;
	while (s[i])
		i++;
	return (i);
}

/* ── ft_strdup ──────────────────────────────────────────── */
char	*ft_strdup(const char *s)
{
	int		len;
	char	*dup;

	len = ft_strlen(s);
	dup = malloc(len + 1);
	if (!dup)
		return (NULL);
	ft_memcpy(dup, s, len + 1);
	return (dup);
}

/*
** ft_strtrim_nl – removes a trailing '\n' (or '\r\n') in-place, returns s.
*/
char	*ft_strtrim_nl(char *s)
{
	int	len;

	len = ft_strlen(s);
	while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
	{
		s[len - 1] = '\0';
		len--;
	}
	return (s);
}

/* ── ft_free_strarr ─────────────────────────────────────── */
void	ft_free_strarr(char **arr)
{
	int	i;

	if (!arr)
		return ;
	i = 0;
	while (arr[i])
	{
		free(arr[i]);
		i++;
	}
	free(arr);
}

/* ── free_map ───────────────────────────────────────────── */
void	free_map(t_map *map)
{
	if (!map)
		return ;
	ft_free_strarr(map->grid);
	map->grid = NULL;
	map->height = 0;
	map->width = 0;
}

/* ── error_exit ─────────────────────────────────────────── */
void	error_exit(t_map *map, const char *msg)
{
	free_map(map);
	write(STDERR_FILENO, "Error\n", 6);
	if (msg)
	{
		write(STDERR_FILENO, msg, ft_strlen(msg));
		write(STDERR_FILENO, "\n", 1);
	}
	exit(EXIT_FAILURE);
}


void	*ft_memset(void *b, int c, int len)
{
	unsigned char *p;
	int i;
	p = (unsigned char *)b;
	i = 0;
	while (i < len)
		p[i++] = (unsigned char)c;
	return (b);
}

void	*ft_memcpy(void *dst, const void *src, int n)
{
	unsigned char *d=(unsigned char *)dst;
	const unsigned char *s=(const unsigned char *)src;
	int i=0;
	while (i<n)
	{
		d[i]=s[i];
		i++;
	}
	return (dst);
}

int	ft_strcmp(const char *s1, const char *s2)
{
	int i=0;
	while (s1[i] && s1[i]==s2[i])
		i++;
	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}
