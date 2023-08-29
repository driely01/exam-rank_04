#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>


typedef enum s_type {
	CMD,
	PIPE,
	SEQ,
	END
}	t_type;

typedef struct s_tree {
	char			**cmd;
	int				type;
	struct s_tree	*left_child;
	struct s_tree	*right_child;
}	t_tree;

int	execution(t_tree *root, char **env);

int ft_strlen(char *str)
{
	int i = 0;

	while(str && str[i]) {
		i++;
	}
	return (i);
}

int	ft_strcmp(char *s1, char *s2)
{
	size_t			i;

	i = 0;
	if (!s1 || !s2)
		return (-1);
	while ((s1[i] != '\0' || s2[i] != '\0')) {
		if (s1[i] > s2[i] || s1[i] < s2[i])
			return (s1[i] - s2[i]);
		i++;
	}
	return (0);
}

int	count_command(char **av, int i)
{
	int count;

	count = 0;
	while (av[i]) {
		if(!ft_strcmp(av[i], ";") && ft_strlen(av[i]) == 1)
			break;
		if(!ft_strcmp(av[i], "|") && ft_strlen(av[i]) == 1)
			break;
		else {
			i++;
			count++;
		}
	}
	return (count);
}

void	skip_pipes_seq(char **av, int *i)
{
	while (av[*i] && ((!ft_strcmp(av[*i], ";") && ft_strlen(av[*i]) == 1) || (!ft_strcmp(av[*i], "|") && ft_strlen(av[*i]) == 1)))
		(*i)++;
}

int	peek(char **av, int i)
{
	while (av[i] && ((ft_strcmp(av[i], ";") && ft_strlen(av[i]) != 1) || (ft_strcmp(av[i], "|") && ft_strlen(av[i]) != 1))) {

		i++;
	}
	if (av[i] && !ft_strcmp(av[i], ";") && ft_strlen(av[i]) == 1)
		return (SEQ);
	else if (av[i] && !ft_strcmp(av[i], "|") && ft_strlen(av[i]) == 1)
		return (PIPE);
	return (-1);
}

char	*ft_strdup(char *str)
{
	int		i;
	int		lenght;
	char	*tmp;

	i = 0;
	lenght = ft_strlen(str);
	tmp = malloc(sizeof(char) * (lenght + 1));
	if (!tmp)
		return (NULL);
	while (str[i]) {

		tmp[i] = str[i];
		i++;
	}
	tmp[i] = 0;
	return (tmp);
}


t_tree	*new_command(char **av, int *i)
{
	t_tree	*head;
	int		lenght;
	int		j;

	j = 0;
	head = malloc(sizeof(t_tree));
	lenght = count_command(av, *i);
	head->cmd = malloc(sizeof(char *) * (lenght + 1));
	if (!head->cmd) {
		perror("malloc");
		exit(1);
	}
	while (j < lenght) {
		head->cmd[j] = ft_strdup(av[*i]);
		j++;
		(*i)++;
	}
	head->cmd[j] = NULL;
	head->type = CMD;
	head->left_child = NULL;
	head->right_child = NULL;
	return (head);
}

t_tree	*new_pipe(t_tree **root, t_tree *right_cmd)
{
	t_tree	*head;

	head = malloc(sizeof(t_tree));
	head->cmd = NULL;
	head->type = PIPE;
	head->left_child = *root;
	head->right_child = right_cmd;
	return (head);
}

t_tree	*new_sequance(t_tree **root, t_tree *right_pipe)
{

	t_tree	*head;

	head = malloc(sizeof(t_tree));
	if (!head)
		exit(1);
	head->cmd = NULL;
	head->type = SEQ;
	head->left_child = *root;
	head->right_child = right_pipe;
	return (head);
}

t_tree	*pipe_line(t_tree **root, char **av, int *i)
{
	t_tree	*right_cmd;

	right_cmd = NULL;
	*root = new_command(av, i);
	while (peek(av, *i) == PIPE) {

		if (!(*root))
			*root = new_command(av, i);
		while (av[*i] && ((!ft_strcmp(av[*i], ";") && ft_strlen(av[*i]) == 1) || (!ft_strcmp(av[*i], "|") && ft_strlen(av[*i]) == 1)))
			(*i)++;
		right_cmd =  new_command(av, i);
		*root = new_pipe(root, right_cmd);
	}
	return (*root);
}

void	sequance(t_tree **root, char **av, int *i)
{
	t_tree	*right_pipe;

	right_pipe = NULL;
	while (peek(av, *i) == SEQ) {
		if (!*root)
			*root = pipe_line(root, av, i);
		while (av[*i] && ((!ft_strcmp(av[*i], ";") && ft_strlen(av[*i]) == 1) || (!ft_strcmp(av[*i], "|") && ft_strlen(av[*i]) == 1)))
			(*i)++;
		right_pipe = pipe_line(&right_pipe, av, i);
		*root = new_sequance(root, right_pipe);
	}
}

void	parser(t_tree **root, char **av)
{
	int	i;
	int	j;

	i = 1;
	j = 1;
	skip_pipes_seq(av, &i);
	while (av[i]) {
		if (peek(av, i) == PIPE)
			pipe_line(root, av, &i);
		else if (peek(av, i) == SEQ)
			sequance(root, av, &i);
		else
			pipe_line(root, av, &i);
	}
}

void	print_tree(t_tree *root)
{
	int i;
	 
	i = 0;
	if (root->type == CMD)
	{
		while (root->cmd[i])
		{
			printf("cmd: %s\n", root->cmd[i++]);
		}
		return ;
	}
	print_tree(root->left_child);
	print_tree(root->right_child);
}

int	execute_cd(t_tree *root)
{
	int i;

	i = 0;
	while (root->cmd && root->cmd[i])
		i++;
	if (i == 1 || i > 2) {
		printf("cd: more than one argument\n");
		return (-1);
	}
	if (chdir(root->cmd[1]) == -1) {
		perror("cd");
		return (-1);
	}
	return (0);
}

int execute_command(t_tree *root, char **env)
{
	int pid;

	if (!ft_strcmp(root->cmd[0], "cd") && ft_strlen(root->cmd[0]) == 2)
		return (execute_cd(root));
	pid = fork();
	if (pid == -1) {
		printf("error fork\n");
		return (-1);
	}
	if (pid == 0) {
		if (root->cmd && root->cmd[0]) {
			execve(root->cmd[0], root->cmd, env);
			printf("error execve\n");
		}
		exit(1);
	} else {
		waitpid(pid, NULL, 0);
	}
	return (0);
}

int execute_pipe(t_tree *root, char **env)
{
	int fd[2];
	int pid;

	if (pipe(fd) == -1) {
		printf("pipe error\n");
		return (-1);
	}
	pid = fork();
	if (pid == -1) {
		printf("fork error\n");
		return (-1);
	}
	if (pid == 0) {
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		execution(root->left_child, env);
		exit(1);
	}
	pid = fork();
	if (pid == -1) {
		printf("fork error\n");
		return (-1);
	}
	if (pid == 0) {
		dup2(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		execution(root->right_child, env);
		exit(1);
	}
	close(fd[0]);
	close(fd[1]);
	while (1) {

		if (waitpid(-1, NULL, 0) == -1)
			break;
	}
	return (0);
}

int execute_sequance(t_tree *root, char **env)
{
	return (execution(root->left_child, env));
	return (execution(root->right_child, env));
	return (0);
}

int	execution(t_tree *root, char **env)
{
	if (root->type == SEQ)
		return (execute_sequance(root, env));
	else if (root->type == PIPE)
		return (execute_pipe(root, env));
	else 
		return (execute_command(root, env));
	return (0);
}

int main(int ac, char **av, char **env)
{
	t_tree	*root;
	int		i;

	i = 0;
	root = NULL;
	if (ac > 1) {
		parser(&root, av);
		if (execution(root, env) == -1)
			return (1);
	} else 
		return (1);
	return (0);
}
