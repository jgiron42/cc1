void		*malloc(unsigned long int size);
void        free(void *ptr);
int			puts(const char *s);
int			putchar(int);
int			printf(const char *format, ...);
int			strcmp(const char *s1, const char *s2);
void		exit(int status);
long int	write(int fd, const void *buf, unsigned long int count);

typedef unsigned long int size_t;


size_t	ft_strlen(const char *str)
{
	size_t	a;
	const int b = 43;

	a = 0;
	while (str[a] != '\0')
		a++;
	return (a);
}

void	*ft_memcpy(void *dst, const void *src, size_t n)
{
	size_t	i;

	if (!dst && !src)
		return (0);
	i = 0;
	while (i < n / sizeof(long int))
	{
		((long int *)dst)[i] = ((long int *)src)[i];
		i++;
	}
	i *= sizeof(long int);
	while (i < n)
	{
		((char *)dst)[i] = ((char *)src)[i];
		i++;
	}
	return (dst);
}

char	*ft_strdup(const char *s1)
{
	char	*dest;
	size_t	len = ft_strlen(s1);

	dest = malloc(len + 1);
	if (dest)
		dest = ft_memcpy(dest, s1, len + 1);
	return (dest);
}

char	*ft_strmapi(char const *s, char (*f)(unsigned int, char))
{
	size_t	i;
	char	*dst;

	if (!s || !f)
		return (0);
	i = -1;
	dst = ft_strdup(s);
	if (!dst)
		return (0);
	while (dst[++i])
	{
		dst[i] = (*f)(i, dst[i]);
	}
	return (dst);
}

int factorial(unsigned int n)
{
	if (n)
		return n * factorial(n - 1);
	return 1;
}


int	ft_isalpha(int c)
{
	c |= 32;
	return (c >= 'a' && c <= 'z');
}

char rot(unsigned int n, char c)
{
	if (!ft_isalpha(c))
		return c;
	if (c <= 'Z')
		return 'A' + ((c - (unsigned int)'A') + factorial(n % 10) + n / 10) % 26;
	else
		return 'a' + ((c - (unsigned int)'a') + factorial(n % 10) + n / 10) % 26;
}

int main(int ac, char **av)
{
	int i = 1;

	while (i < ac)
	{
		char *s = ft_strmapi(av[i], &rot);

		puts(s);
		free(s);
		i++;
	}
}