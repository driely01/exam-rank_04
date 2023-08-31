#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

typedef enum s_type {
	CMD,
	PIPE,
	SEQ
}	t_type;

typedef struct s_tree {
	char 			**cmd;
	int				type;
	struct s_tree	*left_child;
	struct s_tree	*right_child;
}	t_tree;

void	executor(t_tree *root, char **env);

int	ft_strlen(char *str) {
	int i;

	i = 1;
	while (str[i])
		i++;
	return (i);
}

void	ft_putstr(char *str, int fd) {
	int i;

	i = 0;
	while (str[i]) {
		write(fd, &str[i], 1);
		i++;
	}
}

int ft_strcmp(char *s1, char *s2) {
	int i;
	
	i = 0;
	if (!s1 || !s2)
		return (-1);
	while (s1[i] || s2[i]) {
		if (s1[i] > s2[i] || s1[i] < s2[i])
			return (s2[i] - s1[i]);
		i++;
	}
	return (0);
}

void	skip_all_pipes_and_seqs(char **argv, int *i) {
	while (argv[*i] && ((!ft_strcmp(argv[*i], ";") && ft_strlen(argv[*i]) == 1) || (!ft_strcmp(argv[*i], "|") && ft_strlen(argv[*i]) == 1))) {
		(*i)++;
	}
}

char	*ft_strdup(char *str) {
	char	*tmp;
	int		i;
	int		lenght;

	i = 0;
	if (!str)
		return (NULL);
	lenght = ft_strlen(str);
	tmp = (char *)malloc(sizeof(char) * (lenght + 1));
	if (!tmp)
		return (NULL);
	while (str[i]) {
		tmp[i] = str[i];
		i++;
	}
	tmp[i] = 0;
	return (tmp);
}

void	free_tree(t_tree *root) {
	int i;
	 
	i = 0;
	if (root->type == CMD) {
		while (root->cmd && root->cmd[i]) {
			free(root->cmd[i++]);
		}
		free(root->cmd);
		return ;
	}
	free_tree(root->left_child);
	free_tree(root->right_child);
	free(root);
}

int	peek(char **argv, int i) {
	while (argv[i] && ((ft_strcmp(argv[i], "|") && ft_strlen(argv[i]) != 1) || (ft_strcmp(argv[i], ";") && ft_strlen(argv[i]) != 1))) {
		i++;
	}
	if (argv[i] && !ft_strcmp(argv[i], ";") && ft_strlen(argv[i]) == 1)
		return (SEQ);
	else if (argv[i] && !ft_strcmp(argv[i], "|") && ft_strlen(argv[i]) == 1)
		return (PIPE);
	else
		return (-1);
}

int	cmd_counter(char **argv, int i) {
	int counter;

	counter = 0;
	while (argv[i]) {
		if (!ft_strcmp(argv[i], ";") && ft_strlen(argv[i]) == 1)
			break;
		else if (!ft_strcmp(argv[i], "|") && ft_strlen(argv[i]) == 1)
			break;
		else {
			i++;
			counter++;
		}
	}
	return (counter);
}

t_tree	*new_command(char **argv, int *i) {
	int		j;
	int		lenght;
	t_tree	*head;
	
	j = 0;
	head = (t_tree *)malloc(sizeof(t_tree));
	lenght = cmd_counter(argv, *i);
	if (!head)
		exit(1);
	head->cmd = (char **)malloc(sizeof(char *) * (lenght + 1));
	if (!head->cmd)
		exit(1);
	while (j < lenght) {
		head->cmd[j] = ft_strdup(argv[*i]);
		(*i)++;
		j++;
	}
	head->cmd[j] = NULL;
	head->type = CMD;
	head->left_child = NULL;
	head->right_child = NULL;
	return (head);
}

t_tree	*new_pipe(t_tree **root, t_tree *right_cmd) {
	t_tree	*head;

	head = (t_tree *)malloc(sizeof(t_tree));
	if (!head)
		exit(1);
	head->cmd = NULL;
	head->type = PIPE;
	head->left_child = *root;
	head->right_child = right_cmd;
	return (head);
}

t_tree	*pipeline(t_tree **root, char **argv, int *i) {
	t_tree	*right_cmd;

	right_cmd = NULL;
	*root = new_command(argv, i);
	while (peek(argv, *i) == PIPE) {
		if (!(*root))
			*root = new_command(argv, i);
		skip_all_pipes_and_seqs(argv, i);
		right_cmd = new_command(argv, i);
		*root = new_pipe(root, right_cmd);
	}
	return (*root);
}

t_tree	*new_sequance(t_tree **root, t_tree *right_pipe) {
	t_tree	*head;

	head = (t_tree *)malloc(sizeof(t_tree));
	if (!head)
		exit(1);
	head->cmd = NULL;
	head->type = SEQ;
	head->left_child = *root;
	head->right_child = right_pipe;
	return (head);
}

t_tree	*sequance(t_tree **root, char **argv, int *i) {
	t_tree	*right_pipe;

	right_pipe = NULL;
	while (peek(argv, *i) == SEQ) {
		if (!(*root))
			*root = pipeline(root, argv, i);
		skip_all_pipes_and_seqs(argv, i);
		right_pipe = pipeline(&right_pipe, argv, i);
		*root = new_sequance(root, right_pipe);
	}
	return (*root);
}

void	parser(t_tree **root, char **argv) {
	int i;

	i = 1;
	skip_all_pipes_and_seqs(argv, &i);
	while (argv[i]) {
		if (peek(argv, i) == PIPE)
			pipeline(root, argv, &i);
		else if (peek(argv, i) == SEQ)
			sequance(root, argv, &i);
		else
			pipeline(root, argv, &i);
	}
}


void	execute_cd(t_tree *root) {
	int lenght;

	lenght = cmd_counter(root->cmd, 0);
	if (lenght == 1 || lenght > 2) {
		ft_putstr("error: cd: bad arguments\n", 2);
	}
	else if (chdir(root->cmd[1]) == -1) {
		ft_putstr("error: cd: cannot change directory to ", 2);
		ft_putstr(root->cmd[1], 2);
		ft_putstr("\n", 2);
	}
}

void	execute_command(t_tree *root, char **env) {
	int pid;

	if (root->cmd && !ft_strcmp(root->cmd[0], "cd") && ft_strlen(root->cmd[0]) == 2) {
			execute_cd(root);
			return ;
	}
	pid = fork();
	if (pid == -1) {
		ft_putstr("error: fatal\n", 2);
		exit(1);
	}
	if (pid == 0) {
		if (root->cmd && root->cmd[0]) {
			if (execve(root->cmd[0], root->cmd, env) == -1) {
				ft_putstr("error: cannot execute ", 2);
				ft_putstr(root->cmd[0], 2);
				ft_putstr("\n", 2);
			}
		}
		exit(1);
	} else {
		waitpid(pid, NULL, 0);
	}
}

void	execute_pipe(t_tree *root, char **env) {
	int pid;
	int fd[2];

	if (pipe(fd) == -1) {
		ft_putstr("error: fatal\n", 2);
		exit(1);
	}
	pid = fork();
	if (pid == -1) {
		ft_putstr("error: fatal\n", 2);
		exit(1);
	}
	if (pid == 0) {
		if (dup2(fd[1], 1) == -1) {
			ft_putstr("error: fatal\n", 2);
			exit(1);
		}
		if (close(fd[0]) == -1) {
			ft_putstr("error: fatal\n", 2);
			exit(1);
		}
		if (close(fd[1]) == -1) {
			ft_putstr("error: fatal\n", 2);
			exit(1);
		}
		executor(root->left_child, env);
		exit(1);
	}
	pid = fork();
	if (pid == -1) {
		ft_putstr("error: fatal\n", 2);
		exit(1);
	}
	if (pid == 0) {
		if (dup2(fd[0], 0) == -1) {
			ft_putstr("error: fatal\n", 2);
			exit(1);
		}
		if (close(fd[1]) == -1) {
			ft_putstr("error: fatal\n", 2);
			exit(1);
		}
		if (close(fd[0]) == -1) {
			ft_putstr("error: fatal\n", 2);
			exit(1);
		}
		executor(root->right_child, env);
		exit(1);
	}
	if (close(fd[0]) == -1) {
		ft_putstr("error: fatal\n", 2);
		exit(1);
	}
	if (close(fd[1]) == -1) {
		ft_putstr("error: fatal\n", 2);;
		exit(1);
	}
	while (1) {
		if (waitpid(-1, NULL, 0) == -1)
			break;
	}
}

void	execute_sequance(t_tree *root, char **env) {
	executor(root->left_child, env);
	executor(root->right_child, env);
}

void	executor(t_tree *root, char **env) {
	if (root->type == SEQ)
		execute_sequance(root, env);
	else if (root->type == PIPE)
		execute_pipe(root, env);
	else
		execute_command(root, env);
}

int main(int argc, char **argv, char **env) {
	t_tree	*root;

	root = NULL;
	if (argc > 1) {
		parser(&root, argv);
		if (root) {
			executor(root, env);
			free_tree(root);
		}
	} else {
		return (0);
	}
	return (0);
}
